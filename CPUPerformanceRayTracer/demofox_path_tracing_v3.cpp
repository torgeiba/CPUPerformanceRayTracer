#include "demofox_path_tracing_v2.h"

//#include <thread>

#include "work_queue.h"

// The minimunm distance a ray must travel before we consider an intersection.
// This is to prevent a ray from intersecting a surface it just bounced off of.
const f32 c_minimumRayHitTime = 0.01f;

// after a hit, it moves the ray this far along the normal away from a surface.
// Helps prevent incorrect intersections when rays bounce off of objects.
const f32 c_rayPosNormalNudge = 0.01f;

// the farthest we look for ray hits
const f32 c_superFar = 10000.0f;

// camera FOV
const f32 c_FOVDegrees = 90.0f;

// number of ray bounces allowed
const int c_numBounces = 4; //8

// how many renders per frame - to get around the vsync limitation.
const int c_numRendersPerFrame = 1;

const f32 c_pi = 3.14159265359f;
const f32 c_twopi = 2.0f * c_pi;

static const __m256 MaskFalse{ set1_ps(0.f) };
static const __m256 MaskTrue{ !MaskFalse };
static const __m256 ConstOne{ set1_ps(1.f) };
static const __m256 ConstZero{ set1_ps(0.f) };

static f32 iFrame = 0.f;

static
__m256i wang_hash_ps(__m256i& seeds)
{
    seeds = _mm256_xor_si256(_mm256_xor_si256(seeds, _mm256_set1_epi32(61)), (_mm256_srli_epi32(seeds, 16)));
    seeds = _mm256_mullo_epi32(seeds, _mm256_set1_epi32(9));
    seeds = _mm256_xor_si256(seeds, _mm256_srli_epi32(seeds, 4));
    seeds = _mm256_mullo_epi32(seeds, _mm256_set1_epi32(0x27d4eb2d));
    seeds = _mm256_xor_si256(seeds, _mm256_srli_epi32(seeds, 15));
    return seeds;
}

// packed unsigned int32 to packed 32 bit float conversion. Adapted from 
// https://stackoverflow.com/questions/34066228/how-to-perform-uint32-float-conversion-with-sse
static inline __m256 _mm_cvtepu32_ps(const __m256i v)
{
    __m256i v2 = _mm256_srli_epi32(v, 1);                    // v2 = v / 2
    __m256i v1 = _mm256_and_si256(v, _mm256_set1_epi32(1));  // v1 = v & 1
    __m256 v2f = _mm256_cvtepi32_ps(v2);
    __m256 v1f = _mm256_cvtepi32_ps(v1);
    return _mm256_add_ps(_mm256_add_ps(v2f, v2f), v1f);      // return 2 * v2 + v1
}

static
__m256 Randomf3201_ps(__m256i& state)
{
    // _mm256_cvtepi32_ps does not work here, since it converts from signed integer
    //return _mm256_cvtepi32_ps(wang_hash_ps(state)) / 4294967296.0f;
    //return _mm_cvtepu32_ps(wang_hash_ps(state)) / 4294967296.0f;

    // faster but possibly fewer states version, the point is to not have 1 integer codepoint of bias due to there being more negative numbers than positive in signed integer
    return _mm256_cvtepi32_ps(_mm256_and_si256(_mm256_set1_epi32(0x7FFFFFFF), wang_hash_ps(state))) / 2147483648.0f;
}

static
__m256 SignedRandomf3201_ps(__m256i& state)
{
    return _mm256_cvtepi32_ps(wang_hash_ps(state)) / 2147483648.0f;
}

static
m256x3 RandomUnitVector_ps(__m256i& state)
{
    __m256 wide_z = Randomf3201_ps(state);
    __m256 wide_a = Randomf3201_ps(state);
    __m256 z = wide_z * set1_ps(2.f) - ConstOne;

    __m256 a = wide_a * set1_ps(c_twopi);
    __m256 r = sroot(ConstOne - z * z);
    __m256 x = r * cos_ps(a);
    __m256 y = r * sin_ps(a);
    return m256x3{ x, y, z };
}

//static
//m256x3 RandomUnitVector_ps(__m256i& state)
//{
//    __m256 z = SignedRandomf3201_ps(state);
//    __m256 wide_a = SignedRandomf3201_ps(state);
//
//    __m256 a = (wide_a + ConstOne) * .5f * set1_ps(c_twopi);
//    __m256 r = sroot(ConstOne - z * z);
//    __m256 x = r * cos_ps(a);
//    __m256 y = r * sin_ps(a);
//    return m256x3{ x, y, z };
//}

// ACES tone mapping curve fit to go from HDR to LDR
//https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
//m256x3 ACESFilm(m256x3 X)
//{
//    f32 a = 2.51f;
//    f32 b = 0.03f;
//    f32 c = 2.43f;
//    f32 d = 0.59f;
//    f32 e = 0.14f;
//    return clamp((X * (a * X + b)) / (X * (c * X + d) + e), set1x3_ps(0.f, 0.f, 0.f), set1x3_ps(1.f, 1.f, 1.f));
//}

m256x3 ACESFilm(m256x3 X)
{
    f32 a = 2.51f;
    f32 b = 0.03f;
    f32 c = 2.43f;
    f32 d = 0.59f;
    f32 e = 0.14f;
    return saturate((X * (a * X + b)) / (X * (c * X + d) + e));
}

m256x3 LinearToSRGB(m256x3 rgb)
{
    rgb = saturate(rgb);

    return m256x3{
        blend_ps(pow_ps(rgb.x, ConstOne / 2.4f) * 1.055f - 0.055f, rgb.x * 12.92f, rgb.x < set1_ps(0.0031308f)),
        blend_ps(pow_ps(rgb.y, ConstOne / 2.4f) * 1.055f - 0.055f, rgb.y * 12.92f, rgb.y < set1_ps(0.0031308f)),
        blend_ps(pow_ps(rgb.z, ConstOne / 2.4f) * 1.055f - 0.055f, rgb.z * 12.92f, rgb.z < set1_ps(0.0031308f))
    };
}

m256x3 SRGBToLinear(m256x3 rgb)
{
    rgb = saturate(rgb);

    return m256x3{
        blend_ps(pow_ps((rgb.x + 0.055f) / 1.055f, set1_ps(2.4f)), rgb.x / 12.92f, rgb.x < set1_ps(0.04045f)),
        blend_ps(pow_ps((rgb.y + 0.055f) / 1.055f, set1_ps(2.4f)), rgb.y / 12.92f, rgb.y < set1_ps(0.04045f)),
        blend_ps(pow_ps((rgb.z + 0.055f) / 1.055f, set1_ps(2.4f)), rgb.z / 12.92f, rgb.z < set1_ps(0.04045f))
    };
}

struct SMaterialInfo
{
    m256x3 albedo;
    m256x3 emissive;
    __m256 specularChance;
    __m256 specularRoughness;
    m256x3 specularColor;
    __m256 IOR;
    __m256 refractionChance;
    __m256 refractionRoughness;
    m256x3 refractionColor;
    //__m256 percentSpecular;
    //__m256 roughness;
};

struct SRayHitInfo
{
    __m256 fromInside;
    __m256 dist;
    m256x3 normal;
    SMaterialInfo material;
};

SMaterialInfo GetZeroedMaterial()
{
    SMaterialInfo ret;
    ret.albedo = set1x3_ps( 0.0f, 0.0f, 0.0f );
    ret.emissive = set1x3_ps(0.0f, 0.0f, 0.0f);
    ret.specularChance = ConstZero;
    ret.specularRoughness = ConstZero;
    ret.specularColor = set1x3_ps(0.0f, 0.0f, 0.0f);
    ret.IOR = ConstOne;
    ret.refractionChance = ConstZero;
    ret.refractionRoughness = ConstZero;
    ret.refractionColor = set1x3_ps(0.0f, 0.0f, 0.0f);
    return ret;
}

SMaterialInfo ConditionalWriteMaterial(SMaterialInfo OldMaterial, SMaterialInfo NewMaterial, __m256 cond)
{
    SMaterialInfo ret = OldMaterial;
    ret.albedo              = blend3_ps(OldMaterial.albedo             , NewMaterial.albedo             , cond);
    ret.emissive            = blend3_ps(OldMaterial.emissive           , NewMaterial.emissive           , cond);
    ret.specularChance      =  blend_ps(OldMaterial.specularChance     , NewMaterial.specularChance     , cond);
    ret.specularRoughness   =  blend_ps(OldMaterial.specularRoughness  , NewMaterial.specularRoughness  , cond);
    ret.specularColor       = blend3_ps(OldMaterial.specularColor      , NewMaterial.specularColor      , cond);
    ret.IOR                 =  blend_ps(OldMaterial.IOR                , NewMaterial.IOR                , cond);
    ret.refractionChance    =  blend_ps(OldMaterial.refractionChance   , NewMaterial.refractionChance   , cond);
    ret.refractionRoughness =  blend_ps(OldMaterial.refractionRoughness, NewMaterial.refractionRoughness, cond);
    ret.refractionColor     = blend3_ps(OldMaterial.refractionColor    , NewMaterial.refractionColor    , cond);
    return ret;
}

static
__m256 ScalarTriple(m256x3 u, m256x3 v, m256x3 w)
{
    return dot(cross(u, v), w);
}

// Index Of Refraction 1 : n1
// Index of Refraction 2 : n2
// f0 is the minimum reflection (angle = 0 degrees)
// f0 is the maximum reflection (angle = 90 degrees)
__m256 FresnelReflectAmount(__m256 n1, __m256 n2, m256x3 normal, m256x3 incident, __m256 f0, __m256 f90)
{
    // Schlick approximation
    __m256 r0 = (n1 - n2) / (n1 + n2);
    r0 *= r0;

    __m256 cosX = -dot(normal, incident);
    __m256 cond = (n1 > n2);
    
        __m256 n = n1 / n2;
        __m256 sinT2 = n * n *(ConstOne - cosX * cosX);
        __m256 newCosX = sroot(ConstOne - sinT2);
        __m256 totalInternalReflection = (sinT2 > ConstOne);
        cosX = blend_ps( cosX, newCosX, cond & !totalInternalReflection);
    
    __m256 x = ConstOne - cosX;
    __m256 x2 = x * x;
    __m256 ret = r0 + (ConstOne - r0) * x2 * x2 * x;
    ret = blend_ps(ret, ConstOne, cond & totalInternalReflection);
    return lerp(f0, f90, ret);
}

static
__m256 TestQuadTrace(m256x3 rayPos, m256x3 rayDir, SRayHitInfo& info, m256x3 a, m256x3 b, m256x3 c, m256x3 d)
{
    __m256 result = MaskFalse;
    __m256 early_return = MaskFalse;

    // calculate normal and flip vertices order if needed
    m256x3 normal = normalize(cross(c - a, c - b));
    {
        __m256 cond = (dot(normal, rayDir) > ConstZero);
        normal = blend3_ps(normal, (normal * (-1.0f)), cond);

        // Swap a and d
        m256x3 temp = d;
        d = blend3_ps(d, a, cond); //d = cond ? a : d;

        a = blend3_ps(a, temp, cond); //a = cond ? temp : a;

        // swap b and c
        temp = b;
        b = blend3_ps(b, c, cond);
        c = blend3_ps(c, temp, cond);
    }

    m256x3 p = rayPos;
    m256x3 q = rayPos + rayDir;
    m256x3 pq = q - p;
    m256x3 pa = a - p;
    m256x3 pb = b - p;
    m256x3 pc = c - p;

    // determine which triangle to test against by testing against diagonal first
    m256x3 intersectPos;
    {
        m256x3 m = cross(pc, pq);
        __m256 v = dot(pa, m);

        __m256 v_nonnegative_cond = v >= ConstZero;
        {
            // test against triangle a,b,c
            __m256 u = -dot(pb, m);
            {
                __m256 cond = (v_nonnegative_cond && u < ConstZero);
                result = blend_ps(result, MaskFalse, cond);
                early_return = blend_ps(early_return, MaskTrue, cond);
            }
            __m256 w = ScalarTriple(pq, pb, pa);
            {
                __m256 cond = (v_nonnegative_cond && w < ConstZero);
                result = blend_ps(result, MaskFalse, cond);
                early_return = blend_ps(early_return, MaskTrue, cond);
            }
            __m256 denom = 1.0f / (u + v + w);
            u = u * denom;
            v = blend_ps(v, v * denom, v_nonnegative_cond);
            w = w * denom;
            intersectPos = (u * a) + (v * b) + (w * c);
        }
        {
            m256x3 pd = d - p;
            __m256 u = dot(pd, m);
            {
                __m256 cond = (!(v_nonnegative_cond) && u < ConstZero);
                result = blend_ps(result, MaskFalse, cond);
                early_return = blend_ps(early_return, MaskTrue, cond);
            }
            __m256 w = ScalarTriple(pq, pa, pd);
            {
                __m256 cond = (!(v_nonnegative_cond) && w < ConstZero);
                result = blend_ps(result, MaskFalse, cond);
                early_return = blend_ps(early_return, MaskTrue, cond);

            }
            v = -v;
            __m256 denom = 1.0f / (u + v + w);
            u = u * denom;
            v = v * denom;
            w = w * denom;
            intersectPos = blend3_ps(intersectPos, (u * a) + (v * d) + (w * c), !(v_nonnegative_cond));
        }
    }

    __m256 dist;
    {
        __m256 cond1 = abs_ps(rayDir.x) > ConstZero;
        __m256 cond2 = abs_ps(rayDir.y) > ConstZero;

        dist = (intersectPos.z - rayPos.z) / rayDir.z;
        dist = blend_ps(dist, (intersectPos.y - rayPos.y) / rayDir.y, cond2);
        dist = blend_ps(dist, (intersectPos.x - rayPos.x) / rayDir.x, cond1);
    }

    {
        __m256 cond = (dist > set1_ps(c_minimumRayHitTime) && dist < info.dist);
        cond = (!(early_return) && cond);
        {
            info.fromInside = blend_ps(info.fromInside, ConstZero, cond);
            info.dist = blend_ps(info.dist, dist, cond);
            info.normal = blend3_ps(info.normal, normal, cond);
            result = blend_ps(result, MaskTrue, cond);
        }
    }

    return result;
}

static
__m256 TestSphereTrace(m256x3 rayPos, m256x3 rayDir, SRayHitInfo& info, m256x4 sphere)
{
    __m256 result = MaskFalse;
    __m256 early_return = MaskFalse;

    //get the vector from the center of this sphere to where the ray begins.
    m256x3 spherexyz = m256x3{ sphere.x, sphere.y, sphere.z };
    m256x3 m = rayPos - spherexyz;

    //get the dot product of the above vector and the ray's vector
    __m256 b = dot(m, rayDir);
    __m256 c = dot(m, m) - sphere.w * sphere.w;

    //exit if r's origin outside s (c > 0) and r pointing away from s (b > 0)
    {
        __m256 cond = c > ConstZero && b > ConstZero;
        result = blend_ps(result, MaskFalse, cond);
        early_return = blend_ps(early_return, MaskTrue, cond);
    }

    //calculate discriminant
    __m256 discr = b * b - c;

    //a negative discriminant corresponds to ray missing sphere
    {
        __m256 cond = (discr < ConstZero);
        result = blend_ps(result, MaskFalse, cond);
        early_return = blend_ps(early_return, MaskTrue, cond);
    }


    //ray now found to intersect sphere, compute smallest t value of intersection
    __m256 fromInside = MaskFalse;
    __m256 dist = -b - sroot(discr);

    {
        fromInside = (dist < ConstZero);
        dist = blend_ps(dist, -b + sroot(discr), fromInside);
    }

    {
        __m256 distCheck = (dist > set1_ps(c_minimumRayHitTime) && dist < info.dist);
        __m256 check = (!(early_return) && distCheck);
        info.fromInside = blend_ps(info.fromInside, fromInside, check);
        info.dist = blend_ps(info.dist, dist, check);
        info.normal = blend3_ps(
            info.normal,
            (normalize((rayPos + (rayDir * dist)) - spherexyz) * blend_ps(ConstOne, -ConstOne, fromInside)),
            check);
        result = blend_ps(result, MaskTrue, check);
    }

    return result;
}

static
void TestSceneTrace(m256x3 rayPos, m256x3 rayDir, SRayHitInfo& hitInfo, __m256 hasTraceTerminated)
{
    // to move the scene around, since we can't move the camera yet
    m256x3 sceneTranslation = set1x3_ps(0.0f, 0.0f, 10.0f);
    m256x4 sceneTranslation4 = m256x4{ sceneTranslation.x, sceneTranslation.y, sceneTranslation.z, ConstZero };

    //// back wall
    //{
    //    m256x3 A = set1x3_ps(-12.6f, -12.6f, 25.0f) + sceneTranslation;
    //    m256x3 B = set1x3_ps(12.6f, -12.6f, 25.0f) + sceneTranslation;
    //    m256x3 C = set1x3_ps(12.6f, 12.6f, 25.0f) + sceneTranslation;
    //    m256x3 D = set1x3_ps(-12.6f, 12.6f, 25.0f) + sceneTranslation;

    //    __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
    //    {
    //        SMaterialInfo NewMaterial = GetZeroedMaterial();
    //        NewMaterial.albedo = set1x3_ps(0.7f, 0.7f, 0.7f);
    //        NewMaterial.IOR = set1_ps(1.1f);
    //        hitInfo.material = ConditionalWriteMaterial(hitInfo.material, NewMaterial, cond);
    //    }
    //}

    // floor
    {
        m256x3 A = set1x3_ps(-25.0f, -12.5f, 5.0f) + sceneTranslation;
        m256x3 B = set1x3_ps( 25.0f, -12.5f, 5.0f) + sceneTranslation;
        m256x3 C = set1x3_ps( 25.0f, -12.5f, -5.0f) + sceneTranslation;
        m256x3 D = set1x3_ps(-25.0f, -12.5f, -5.0f) + sceneTranslation;

        __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
        {
            SMaterialInfo NewMaterial = GetZeroedMaterial();
            NewMaterial.albedo = set1x3_ps(0.7f, 0.7f, 0.7f);
            hitInfo.material = ConditionalWriteMaterial(hitInfo.material, NewMaterial, cond);
        }
    }

    // striped background
    {
        m256x3 A = set1x3_ps(-25.0f, -1.5f, 5.0f);
        m256x3 B = set1x3_ps(25.0f, -1.5f, 5.0f);
        m256x3 C = set1x3_ps(25.0f, -10.5f, 5.0f);
        m256x3 D = set1x3_ps(-25.0f, -10.5f, 5.0f);
        {
            __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
            SMaterialInfo NewMaterial = GetZeroedMaterial();
            m256x3 hitPos = rayPos + rayDir * hitInfo.dist;
            __m256 shade = round_floor(fract(hitPos.x) * 2.0f);
            NewMaterial.albedo = m256x3{ shade, shade, shade };
            hitInfo.material = ConditionalWriteMaterial(hitInfo.material, NewMaterial, cond);
        }
    }

    // cieling
    {
        m256x3 A = set1x3_ps(-7.5f, 12.5f, 5.0f) + sceneTranslation;
        m256x3 B = set1x3_ps( 7.5f, 12.5f, 5.0f) + sceneTranslation;
        m256x3 C = set1x3_ps( 7.5f, 12.5f, -5.0f) + sceneTranslation;
        m256x3 D = set1x3_ps(-7.5f, 12.5f, -5.0f) + sceneTranslation;

        {
            __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
            SMaterialInfo NewMaterial = GetZeroedMaterial();
            NewMaterial.albedo = set1x3_ps(0.7f, 0.7f, 0.7f);
            hitInfo.material = ConditionalWriteMaterial(hitInfo.material, NewMaterial, cond);
        }
    }

    //// left wall
    //{
    //    m256x3 A = set1x3_ps(-12.5f, -12.6f, 25.0f) + sceneTranslation;
    //    m256x3 B = set1x3_ps(-12.5f, -12.6f, 15.0f) + sceneTranslation;
    //    m256x3 C = set1x3_ps(-12.5f, 12.6f, 15.0f) + sceneTranslation;
    //    m256x3 D = set1x3_ps(-12.5f, 12.6f, 25.0f) + sceneTranslation;

    //    {
    //        __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
    //        SMaterialInfo NewMaterial = GetZeroedMaterial();
    //        NewMaterial.albedo = set1x3_ps(0.7f, 0.1f, 0.1f);
    //        NewMaterial.IOR = set1_ps(1.1f);
    //        hitInfo.material = ConditionalWriteMaterial(hitInfo.material, NewMaterial, cond);
    //    }
    //}

    //// right wall 
    //{
    //    m256x3 A = set1x3_ps(12.5f, -12.6f, 25.0f) + sceneTranslation;
    //    m256x3 B = set1x3_ps(12.5f, -12.6f, 15.0f) + sceneTranslation;
    //    m256x3 C = set1x3_ps(12.5f, 12.6f, 15.0f) + sceneTranslation;
    //    m256x3 D = set1x3_ps(12.5f, 12.6f, 25.0f) + sceneTranslation;

    //    {
    //        __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
    //        SMaterialInfo NewMaterial = GetZeroedMaterial();
    //        NewMaterial.albedo = set1x3_ps(0.1f, 0.7f, 0.1f);
    //        NewMaterial.IOR = set1_ps(1.1f);
    //        hitInfo.material = ConditionalWriteMaterial(hitInfo.material, NewMaterial, cond);
    //    }
    //}

    // light
    {
        m256x3 A = set1x3_ps(-5.0f, 12.4f, 2.5f) + sceneTranslation;
        m256x3 B = set1x3_ps( 5.0f, 12.4f, 2.5f) + sceneTranslation;
        m256x3 C = set1x3_ps( 5.0f, 12.4f, -2.5f) + sceneTranslation;
        m256x3 D = set1x3_ps(-5.0f, 12.4f, -2.5f) + sceneTranslation;

        {
            __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
            SMaterialInfo NewMaterial = GetZeroedMaterial();
            NewMaterial.emissive = (set1x3_ps(1.0f, 0.9f, 0.7f) * set1_ps(20.0f));
            hitInfo.material = ConditionalWriteMaterial(hitInfo.material, NewMaterial, cond);
        }
    }

    //{
    //    __m256 cond = (!(hasTraceTerminated) && (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps(-9.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4)));
    //    SMaterialInfo NewMaterial = GetZeroedMaterial();
    //    NewMaterial.albedo = set1x3_ps(0.9f, 0.9f, 0.5f);
    //    NewMaterial.specularColor = set1x3_ps(0.9f, 0.9f, 0.9f);
    //    NewMaterial.specularChance = set1_ps(0.1f);
    //    NewMaterial.specularRoughness = set1_ps(0.2f);
    //    NewMaterial.IOR = set1_ps(1.1f);
    //    hitInfo.material = ConditionalWriteMaterial(hitInfo.material, NewMaterial, cond);
    //}

    //{
    //    __m256 cond = (!(hasTraceTerminated) && (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps(0.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4)));
    //    SMaterialInfo NewMaterial = GetZeroedMaterial();
    //    NewMaterial.albedo = set1x3_ps(0.9f, 0.5f, 0.9f);
    //    NewMaterial.specularColor = set1x3_ps(0.9f, 0.9f, 0.9f);
    //    NewMaterial.specularChance = set1_ps(0.3f);
    //    NewMaterial.specularRoughness = set1_ps(0.2f);
    //    NewMaterial.IOR = set1_ps(1.1f);
    //    hitInfo.material = ConditionalWriteMaterial(hitInfo.material, NewMaterial, cond);
    //}

    //{
    //    __m256 cond = (!(hasTraceTerminated) && (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps(9.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4)));
    //    SMaterialInfo NewMaterial = GetZeroedMaterial();
    //    NewMaterial.albedo = set1x3_ps(0.f, 0.f, 1.f);
    //    NewMaterial.specularColor   = set1x3_ps(1.f, 0.f, 0.f);
    //    NewMaterial.specularChance  =set1_ps(0.5f);
    //    NewMaterial.specularRoughness = set1_ps(0.4f);
    //    NewMaterial.IOR = set1_ps(1.1f);
    //    hitInfo.material = ConditionalWriteMaterial(hitInfo.material, NewMaterial, cond);
    //}

    const i32 c_numSpheres = 7;
    for (i32 sphereIndex = 0; sphereIndex < c_numSpheres; sphereIndex += 1)
    {

        __m256 cond = (!(hasTraceTerminated) && (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps(-18.0f + 6.0f * (f32)(sphereIndex), -8.0f, 0.0f, 2.8f) + sceneTranslation4)));
        {
            SMaterialInfo NewMaterial = GetZeroedMaterial();

            __m256 r = set1_ps((f32)sphereIndex) / set1_ps(c_numSpheres - 1) * 0.5f;

            NewMaterial.albedo = set1x3_ps(0.9f, 0.25f, 0.25f);
            NewMaterial.emissive = set1x3_ps(0.0f, 0.0f, 0.0f);
            NewMaterial.specularChance = set1_ps(0.02f);
            NewMaterial.specularRoughness = r;
            NewMaterial.specularColor = set1x3_ps(1.0f, 1.0f, 1.0f) * 0.8f;
            NewMaterial.IOR = set1_ps(1.1f);
            NewMaterial.refractionChance = set1_ps(1.0f);
            NewMaterial.refractionRoughness = r;
            NewMaterial.refractionColor = set1x3_ps(0.0f, 0.5f, 1.0f);

            hitInfo.material = ConditionalWriteMaterial(hitInfo.material, NewMaterial, cond);
        }
    }
}

static
m256x3 GetColorForRay(m256x3 startRayPos, m256x3 startRayDir, __m256i& rngState /*u32& rngState*/, texture& Texture)
{
    // initialize
    m256x3 ret = set1x3_ps(0.0f, 0.0f, 0.0f);
    m256x3 throughput = set1x3_ps(1.0f, 1.0f, 1.0f);
    m256x3 rayPos = startRayPos;
    m256x3 rayDir = startRayDir;

    __m256 shouldBreak = MaskFalse;
    for (i32 bounceIndex = 0; (bounceIndex <= c_numBounces); ++bounceIndex)
    {
        // shoot a ray out into the world
        SRayHitInfo hitInfo{ 0.f };
        hitInfo.material = GetZeroedMaterial();
        hitInfo.dist = set1_ps(c_superFar);
        hitInfo.fromInside = MaskFalse;
        TestSceneTrace(rayPos, rayDir, hitInfo, shouldBreak);

        m256x3 newRet = ret;
        m256x3 newThroughput = throughput;
        m256x3 newRayPos = rayPos;
        m256x3 newRayDir = rayDir;

        // if the ray missed, we are done
        __m256 prevShouldBreak = shouldBreak;
        shouldBreak = (hitInfo.dist == set1_ps(c_superFar));
        {
            m256x3 SampleDir = newRayDir;
            SampleDir.x = -newRayDir.x;
            SampleDir.z = -newRayDir.z;
            //m256x3 ambient = EquirectangularTextureSample(Texture, SampleDir);
            m256x3 ambient = set1x3_ps(.11f, .1f, .15f);
            ambient *= throughput;
            __m256 cond = (!(prevShouldBreak) && shouldBreak);
            // if this is the fist time we hit this case, we add the ambient term once
            newRet = blend3_ps(newRet, newRet + ambient, cond);
        }

        // if (hitInfo.fromInside)
        newThroughput.x *= blend_ps(ConstOne, exp_ps(-hitInfo.material.refractionColor.x * hitInfo.dist), hitInfo.fromInside);
        newThroughput.y *= blend_ps(ConstOne, exp_ps(-hitInfo.material.refractionColor.y * hitInfo.dist), hitInfo.fromInside);
        newThroughput.z *= blend_ps(ConstOne, exp_ps(-hitInfo.material.refractionColor.z * hitInfo.dist), hitInfo.fromInside);

        // apply fresnel
        __m256 specularChance = hitInfo.material.specularChance;
        __m256 refractionChance = hitInfo.material.refractionChance;

        {
            __m256 hasSpecularChance = specularChance > ConstZero;
            __m256 n1  = blend_ps(ConstOne, hitInfo.material.IOR, hitInfo.fromInside);
            __m256 n2  = blend_ps(hitInfo.material.IOR, ConstOne, hitInfo.fromInside);
            __m256 f0  = hitInfo.material.specularChance;
            __m256 f90 = ConstOne;
            //__m256 newSpecularChance = FresnelReflectAmount(n1, n2, hitInfo.normal, rayDir, f0, f90);

            // Is hitInfo.normal and rayDir flipped?
            __m256 newSpecularChance = FresnelReflectAmount(n1, n2, newRayDir, hitInfo.normal, f0, f90);

            specularChance = blend_ps(specularChance, newSpecularChance, hasSpecularChance);

            __m256 chanceMultiplier = (1.f - specularChance) / (1.f - hitInfo.material.specularChance);
            refractionChance = blend_ps(refractionChance, refractionChance * chanceMultiplier, hasSpecularChance);
        }

        __m256 raySelectRoll = Randomf3201_ps(rngState);
        __m256 doSpecularMask = (specularChance > ConstZero) & (specularChance > raySelectRoll);
        __m256 doSpecular = ConstOne & doSpecularMask;
        __m256 doRefractionMask = !doSpecularMask & (refractionChance > ConstZero) & (raySelectRoll < (specularChance + refractionChance));
        __m256 doRefraction = ConstOne & doRefractionMask;

        __m256 rayProbability = 1.0f - (specularChance + refractionChance);
        rayProbability = blend_ps(rayProbability, refractionChance, doRefractionMask);
        rayProbability = blend_ps(rayProbability, specularChance, doSpecularMask);
        rayProbability = max_ps(rayProbability, set1_ps(0.001f));

        // update the ray position
        __m256 doRefractionSign = blend_ps(ConstOne, -ConstOne, doRefractionMask);
        newRayPos = (newRayPos + newRayDir * hitInfo.dist) + doRefractionSign * hitInfo.normal * c_rayPosNormalNudge;

        // calculate new ray direction, in a cosine weighted hemisphere oriented at normal
        {
            m256x3 unnormalizedNewRayDir = hitInfo.normal + RandomUnitVector_ps(rngState);
            //__m256 safeNewRayDir = dot(unnormalizedNewRayDir, unnormalizedNewRayDir) > set1_ps(0.001);
            m256x3 diffuseRayDir = normalize(unnormalizedNewRayDir);

            // reflect
            __m256 specularRoughnessSqrd = hitInfo.material.specularRoughness * hitInfo.material.specularRoughness;
            m256x3 specularRayDir = newRayDir - 2.f * hitInfo.normal * dot(newRayDir, hitInfo.normal);
            specularRayDir = normalize(lerp(specularRayDir, diffuseRayDir, specularRoughnessSqrd));

            __m256 refracRoughnessSqrd = hitInfo.material.refractionRoughness* hitInfo.material.refractionRoughness;
            m256x3 refractionRayDir = rfrct(newRayDir, hitInfo.normal, blend_ps(ConstOne / hitInfo.material.IOR, hitInfo.material.IOR, hitInfo.fromInside));
            refractionRayDir = normalize(lerp(refractionRayDir, normalize(RandomUnitVector_ps(rngState) - hitInfo.normal), refracRoughnessSqrd));

            newRayDir = lerp(diffuseRayDir, specularRayDir, doSpecular);
            newRayDir = lerp(rayDir     , refractionRayDir, doRefraction);

        }

        // add in emissive lighting
        newRet = newRet + hitInfo.material.emissive * newThroughput;

        // update the colorMultiplier
        //throughput = shouldBreak ? throughput :
        //    (throughput * hitInfo.albedo);
        //newThroughput = throughput * lerp(hitInfo.material.albedo, hitInfo.material.specularColor, doSpecular);
        newThroughput = blend3_ps(newThroughput * lerp(hitInfo.material.albedo, hitInfo.material.specularColor, doSpecular), newThroughput, doRefractionMask);

        newThroughput = newThroughput / rayProbability;


        // Do russian roulette here
        {


        }


        ret =           blend3_ps(newRet, ret, shouldBreak);
        throughput =    blend3_ps(newThroughput, throughput, shouldBreak);
        rayPos =        blend3_ps(newRayPos, rayPos, shouldBreak);
        rayDir =        blend3_ps(newRayDir, rayDir, shouldBreak);
    }

    // return pixel color
    return ret;
}

static m256x3 mainImage(m256x2 fragCoord, m256x2 iResolution, f32 iFrame, texture& Texture)
{
    // initialize a random number state based on frag coord and frame
    //u32 rngState = u32(u32(fragCoord.x.m256_f32[0]) * u32(1973) + u32(fragCoord.y.m256_f32[0]) * u32(9277) + u32(iFrame) * u32(26699)) | u32(1);
    __m256i rngState_epi =
        _mm256_or_si256(
            _mm256_add_epi32(
                _mm256_add_epi32(
                    _mm256_mullo_epi32(_mm256_cvtps_epi32(fragCoord.x), _mm256_set1_epi32((u32)1973)),
                    _mm256_mullo_epi32(_mm256_cvtps_epi32(fragCoord.y), _mm256_set1_epi32((u32)9277))),
                _mm256_mullo_epi32(_mm256_set1_epi32((u32)iFrame), _mm256_set1_epi32((u32)26699))),
            _mm256_set1_epi32((u32)1)
        );

    __m256 c_cameraDistance = set1_ps(30.0f);


    // The ray starts at the camera position (the origin)
    m256x3 rayPosition = m256x3{ ConstZero, ConstZero, ConstZero };

    m256x3 cameraPosition = m256x3{ ConstZero, ConstZero, -c_cameraDistance };

    // calculate the camera distance
    //f32 cameraDistance = 1.0f / tan(c_FOVDegrees * 0.5f * c_pi / 180.0f);

    __m256 cameraDistance = set1_ps(tan(c_FOVDegrees * 0.5f * c_pi / 180.0f));
    //__m256 cameraDistance = set1_ps(1.0f / tan(c_FOVDegrees * 0.5f * c_pi / 180.0f));


    // calculate coordinates of the ray target on the imaginary pixel plane.
    // -1 to +1 on x,y axis. 1 unit away on the z axis
    m256x2 jitter = m256x2{ Randomf3201_ps(rngState_epi), Randomf3201_ps(rngState_epi) } - .5f;
    m256x2 rayTargetxy = ((fragCoord + jitter) / iResolution) * 2.0f - m256x2{ ConstOne, ConstOne };
    m256x3 rayTarget = m256x3{ rayTargetxy.x, rayTargetxy.y, cameraDistance };
    m256x2 uvJittered = ((fragCoord + jitter) / iResolution);
    m256x2 screen = uvJittered * 2.0f - m256x2{ ConstOne, ConstOne };

    // correct for aspect ratio
    __m256 aspectRatio = iResolution.x / iResolution.y;
    rayTarget.y /= aspectRatio;
    screen.y /= aspectRatio;

    /*m256x3 rayTarget = m256x3{ rayTargetxy.x, rayTargetxy.y, set1_ps(cameraDistance) };
    rayTarget.y /= aspectRatio;*/

    // calculate a normalized vector for the ray direction.
    // it's pointing from the ray position to the ray target.
    //m256x3 rayDir = normalize(rayTarget - cameraPosition);
    m256x3 rayDir = normalize(m256x3{screen.x, screen.y, cameraDistance});
    //m256x3 rayDir = normalize(rayTarget - rayPosition);

    // raytrace for this pixel
    m256x3 color{ ConstZero, ConstZero, ConstZero };
    for (int index = 0; index < c_numRendersPerFrame; ++index)
        color += (GetColorForRay(cameraPosition, rayDir, rngState_epi, Texture) * (1.f / (c_numRendersPerFrame)));

    return color;
}

struct RenderBufferInfo
{
    f32* BufferDataPtr;
    i32 BufferWidth;
    i32 BufferHeight;
    i32 NumChannels;
};

struct RenderTileInfo
{
    i32 TileX, TileY;
    i32 TileWidth, TileHeight;
    i32 TileMinX, TileMaxX;
    i32 TileMinY, TileMaxY;
};

static void RenderTile(RenderBufferInfo& BufferInfo, RenderTileInfo& TileInfo, texture& Texture)
{
    m256x2 fragCoord;
    m256x2 iResolution;

    iResolution.x = set1_ps((f32)BufferInfo.BufferWidth);
    iResolution.y = set1_ps((f32)BufferInfo.BufferHeight);

    __m256 XLaneOffsets = set_ps(7.f, 6.f, 5.f, 4.f, 3.f, 2.f, 1.f, 0.f);

    i32 TileSize = TileInfo.TileHeight * TileInfo.TileWidth * BufferInfo.NumChannels;
    i32 TileYOffset = TileInfo.TileY * TileInfo.TileHeight * BufferInfo.BufferWidth * BufferInfo.NumChannels;
    i32 TileXOffset = TileSize * TileInfo.TileX;
    i32 BufferTileOffset = TileXOffset + TileYOffset;

    f32* BufferPos = BufferInfo.BufferDataPtr + BufferTileOffset;

    //i32 YHeight = BufferInfo.BufferHeight;
    i32 XWidth = (BufferInfo.BufferWidth / LANE_COUNT);

    for (i32 Y = TileInfo.TileMinY; Y <= TileInfo.TileMaxY; Y++)
    {
        fragCoord.y = set1_ps((f32)(BufferInfo.BufferHeight - 1 - Y));
        for (i32 X = TileInfo.TileMinX; X <= TileInfo.TileMaxX; X += LANE_COUNT)
        {
            fragCoord.x = set1_ps((f32)X) + XLaneOffsets;

            m256x3 color = mainImage(fragCoord, iResolution, iFrame, Texture);

            f32* R = &BufferPos[0];
            f32* G = &BufferPos[1 * LANE_COUNT];
            f32* B = &BufferPos[2 * LANE_COUNT];

            // average the frames together
            m256x3 lastFrameColor = m256x3{ load_ps(R), load_ps(G), load_ps(B) };
            ////color; // texture(iChannel0, fragCoord / iResolution.xy).rgb;

            color = lerp(lastFrameColor, color, (1.0f / f32(iFrame + 1.f)));

            non_temporal_store(R, color.x);
            non_temporal_store(G, color.y);
            non_temporal_store(B, color.z);
            BufferPos += LANE_COUNT * 3;
        }
    }

}

struct WorkerThreadData
{
    RenderBufferInfo BufferInfo; // TODO: this may be updated. make safe
    RenderTileInfo TileInfo;
    texture Texture;
};

static constexpr i32 NumMaxThreads = 8;
static bool threadpooluninitialized = true;
static WorkerThreadData WorkData[NumMaxThreads];

static WORK_QUEUE_CALLBACK(DoWorkerThreadWork) // test callback function
{
    WorkerThreadData* WorkerData = (WorkerThreadData*)Data;
    RenderTile(WorkerData->BufferInfo, WorkerData->TileInfo, WorkerData->Texture);
}

static work_queue* Queue;


void DemofoxRenderV3(f32* BufferOut, i32 BufferWidth, i32 BufferHeight, i32 NumTilesX, i32 NumTilesY, i32 TileWidth, i32 TileHeight, i32 NumChannels,
    texture Texture
)
{
    RenderBufferInfo BufferInfo;
    {
        BufferInfo.BufferDataPtr = BufferOut;
        BufferInfo.BufferWidth = BufferWidth;
        BufferInfo.BufferHeight = BufferHeight;
        BufferInfo.NumChannels = NumChannels;
    }

    i32 NumThreads = NumTilesX * NumTilesY;

    // If first time we render, initialize threadpool. TODO: do this at app initialization instead
    if (threadpooluninitialized)
    {
        Queue = MakeWorkQueue();
        threadpooluninitialized = false;
    }

    iFrame += 1.0f;

    for (i32 TileX = 0; TileX < NumTilesX; TileX++)
    {
        i32 TileMinX = TileX * TileWidth;
        i32 TileMaxX = TileMinX + (TileWidth - 1);
        for (i32 TileY = 0; TileY < NumTilesY; TileY++)
        {
            // render tile (TileX, TileY)
            i32 TileMinY = TileY * TileHeight;
            i32 TileMaxY = TileMinY + (TileHeight - 1);

            RenderTileInfo TileInfo;
            {
                TileInfo.TileX = TileX;
                TileInfo.TileY = TileY;

                TileInfo.TileHeight = TileHeight;
                TileInfo.TileWidth = TileWidth;

                TileInfo.TileMinX = TileMinX;
                TileInfo.TileMaxX = TileMaxX < BufferWidth ? TileMaxX : (BufferWidth - 1);

                TileInfo.TileMinY = TileMinY;
                TileInfo.TileMaxY = TileMaxY < BufferHeight ? TileMaxY : (BufferHeight - 1);

                TileInfo.TileHeight = TileInfo.TileMaxY - TileInfo.TileMinY + 1;
                TileInfo.TileWidth = TileInfo.TileMaxX - TileInfo.TileMinX + 1;
            }

            i32 FlatTileIndex = TileX + NumTilesX * TileY;
            WorkData[FlatTileIndex].BufferInfo = BufferInfo;
            WorkData[FlatTileIndex].TileInfo = TileInfo;
            WorkData[FlatTileIndex].Texture = Texture;
            void* DataPointer = &WorkData[FlatTileIndex];
            AddWorkQueueEntry(Queue, DoWorkerThreadWork, DataPointer);
        }
    }

    CompleteAllWork(Queue);
}