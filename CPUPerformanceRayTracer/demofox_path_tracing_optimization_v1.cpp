#include "demofox_path_tracing_optimization_v1.h"
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
const int c_numBounces = 8;

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

//// ACES tone mapping curve fit to go from HDR to LDR
////https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
//m256x3 ACESFilm(m256x3 X)
//{
//    f32 a = 2.51f;
//    f32 b = 0.03f;
//    f32 c = 2.43f;
//    f32 d = 0.59f;
//    f32 e = 0.14f;
//    return clamp((X * (a * X + b)) / (X * (c * X + d) + e), set1x3_ps(0.f, 0.f, 0.f), set1x3_ps(1.f, 1.f, 1.f));
//}
//
//m256x3 LinearToSRGB(m256x3 rgb)
//{
//    rgb = clamp(rgb, set1x3_ps(0.f, 0.f, 0.f), set1x3_ps(1.f, 1.f, 1.f));
//
//    return m256x3{
//        blend_ps(pow_ps(rgb.x, ConstOne / 2.4f) * 1.055f - 0.055f, rgb.x * 12.92f, rgb.x < set1_ps(0.0031308f)),
//        blend_ps(pow_ps(rgb.y, ConstOne / 2.4f) * 1.055f - 0.055f, rgb.y * 12.92f, rgb.y < set1_ps(0.0031308f)),
//        blend_ps(pow_ps(rgb.z, ConstOne / 2.4f) * 1.055f - 0.055f, rgb.z * 12.92f, rgb.z < set1_ps(0.0031308f))
//    };
//}
//
//m256x3 SRGBToLinear(m256x3 rgb)
//{
//    rgb = clamp(rgb, set1x3_ps(0.f, 0.f, 0.f), set1x3_ps(1.f, 1.f, 1.f));
//
//    return m256x3{
//        blend_ps(pow_ps((rgb.x + 0.055f)/ 1.055f, set1_ps(2.4f)), rgb.x / 12.92f, rgb.x < set1_ps(0.04045f)),
//        blend_ps(pow_ps((rgb.y + 0.055f)/ 1.055f, set1_ps(2.4f)), rgb.y / 12.92f, rgb.y < set1_ps(0.04045f)),
//        blend_ps(pow_ps((rgb.z + 0.055f)/ 1.055f, set1_ps(2.4f)), rgb.z / 12.92f, rgb.z < set1_ps(0.04045f))
//    };
//}

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
};

struct SRayHitInfo
{
    __m256 fromInside;
    __m256 dist;
    m256x3 normal;
    SMaterialInfo material;
    __m256i materialIndex;
};

static SMaterialInfo GetZeroedMaterial()
{
    SMaterialInfo ret;
    ret.albedo = set1x3_ps(0.0f, 0.0f, 0.0f);
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

static SMaterialInfo ConditionalWriteMaterial(SMaterialInfo OldMaterial, SMaterialInfo NewMaterial, __m256 cond)
{
    SMaterialInfo ret = OldMaterial;
    ret.albedo = blend3_ps(OldMaterial.albedo, NewMaterial.albedo, cond);
    ret.emissive = blend3_ps(OldMaterial.emissive, NewMaterial.emissive, cond);
    ret.specularChance = blend_ps(OldMaterial.specularChance, NewMaterial.specularChance, cond);
    ret.specularRoughness = blend_ps(OldMaterial.specularRoughness, NewMaterial.specularRoughness, cond);
    ret.specularColor = blend3_ps(OldMaterial.specularColor, NewMaterial.specularColor, cond);
    ret.IOR = blend_ps(OldMaterial.IOR, NewMaterial.IOR, cond);
    ret.refractionChance = blend_ps(OldMaterial.refractionChance, NewMaterial.refractionChance, cond);
    ret.refractionRoughness = blend_ps(OldMaterial.refractionRoughness, NewMaterial.refractionRoughness, cond);
    ret.refractionColor = blend3_ps(OldMaterial.refractionColor, NewMaterial.refractionColor, cond);
    return ret;
}


struct QuadSceneObject
{
    // Vertices
    m256x3 V0;// = set1x3_ps(-25.0f, -12.5f, 5.0f);
    m256x3 V1;// = set1x3_ps(25.0f, -12.5f, 5.0f);
    m256x3 V2;// = set1x3_ps(25.0f, -12.5f, -5.0f);
    m256x3 V3;// = set1x3_ps(-25.0f, -12.5f, -5.0f);
};

//static void PrecomputeQuadData(QuadSceneObject& Quad, m256x3 rayOrigin)
//{
//    m256x3 V0 = Quad.V0 - rayOrigin;
//    m256x3 V1 = Quad.V1 - rayOrigin;
//    m256x3 V2 = Quad.V2 - rayOrigin;
//    m256x3 V3 = Quad.V3 - rayOrigin;
//
//    Quad.E01 = cross(V0, V1);
//
//    //__m256 DetA = dot(invQuad.E01, 
//
//    Quad.E12 = cross(V1, V2);
//    Quad.E20 = cross(V2, V0);
//
//    Quad.E13 = cross(V1, V3);
//    Quad.E32 = cross(V3, V2);
//
//    Quad.normal = cross(V1-V0, V2-V0);
//}

struct SphereSceneObject
{
    m256x4 PositionAndRadius;
};


#define MAX_OBJECTS 12
#define MAX_MATERIALS 12

struct SceneMaterialSOA
{
    f32 albedoR[MAX_MATERIALS];
    f32 albedoG[MAX_MATERIALS];
    f32 albedoB[MAX_MATERIALS];
    f32 emissiveR[MAX_MATERIALS];
    f32 emissiveG[MAX_MATERIALS];
    f32 emissiveB[MAX_MATERIALS];
    f32 specularChance[MAX_MATERIALS];
    f32 specularRoughness[MAX_MATERIALS];
    f32 specularColorR[MAX_MATERIALS];
    f32 specularColorG[MAX_MATERIALS];
    f32 specularColorB[MAX_MATERIALS];
    f32 IOR[MAX_MATERIALS];
    f32 refractionChance[MAX_MATERIALS];
    f32 refractionRoughness[MAX_MATERIALS];
    f32 refractionColorR[MAX_MATERIALS];
    f32 refractionColorG[MAX_MATERIALS];
    f32 refractionColorB[MAX_MATERIALS];
};

struct SceneMaterial
{
    f32x3 albedo;
    f32x3 emissive;
    f32 specularChance;
    f32 specularRoughness;
    f32x3 specularColor;
    f32 IOR;
    f32 refractionChance;
    f32 refractionRoughness;
    f32x3 refractionColor;
};

struct Scene
{
    i32 NumQuadObjects = 0;
    QuadSceneObject QuadObjects[MAX_OBJECTS];

    i32 NumSphereObjects = 0;
    SphereSceneObject SphereObjects[MAX_OBJECTS];

    i32 NumMaterials = 0;
    SceneMaterialSOA materials;

    m256x3 sceneTranslation;
    m256x4 sceneTranslation4;
};

static Scene scene;

static SMaterialInfo GatherMaterials(Scene& scene, __m256i materialIndices)
{
    SMaterialInfo Info{ 0 };
    Info.albedo.x = _mm256_i32gather_ps((f32*)&scene.materials.albedoR, materialIndices, 4);
    Info.albedo.y = _mm256_i32gather_ps((f32*)&scene.materials.albedoG, materialIndices, 4);
    Info.albedo.z = _mm256_i32gather_ps((f32*)&scene.materials.albedoB, materialIndices, 4);

    Info.emissive.x = _mm256_i32gather_ps((f32*)&scene.materials.emissiveR, materialIndices, 4);
    Info.emissive.y = _mm256_i32gather_ps((f32*)&scene.materials.emissiveG, materialIndices, 4);
    Info.emissive.z = _mm256_i32gather_ps((f32*)&scene.materials.emissiveB, materialIndices, 4);

    Info.specularChance = _mm256_i32gather_ps((f32*)&scene.materials.specularChance, materialIndices, 4);
    Info.specularRoughness = _mm256_i32gather_ps((f32*)&scene.materials.specularRoughness, materialIndices, 4);

    Info.specularColor.x = _mm256_i32gather_ps((f32*)&scene.materials.specularColorR, materialIndices, 4);
    Info.specularColor.y = _mm256_i32gather_ps((f32*)&scene.materials.specularColorG, materialIndices, 4);
    Info.specularColor.z = _mm256_i32gather_ps((f32*)&scene.materials.specularColorB, materialIndices, 4);

    Info.IOR = _mm256_i32gather_ps((f32*)&scene.materials.IOR, materialIndices, 4);

    Info.refractionChance = _mm256_i32gather_ps((f32*)&scene.materials.refractionChance, materialIndices, 4);
    Info.refractionRoughness = _mm256_i32gather_ps((f32*)&scene.materials.refractionRoughness, materialIndices, 4);

    Info.refractionColor.x = _mm256_i32gather_ps((f32*)&scene.materials.refractionColorR, materialIndices, 4);
    Info.refractionColor.y = _mm256_i32gather_ps((f32*)&scene.materials.refractionColorG, materialIndices, 4);
    Info.refractionColor.z = _mm256_i32gather_ps((f32*)&scene.materials.refractionColorB, materialIndices, 4);

    return Info;
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
static __m256 FresnelReflectAmount(__m256 n1, __m256 n2, m256x3 normal, m256x3 incident, __m256 f0, __m256 f90)
{
    // Schlick approximation
    __m256 r0 = (n1 - n2) / (n1 + n2);
    r0 *= r0;

    __m256 cosX = -dot(normal, incident);
    __m256 cond = (n1 > n2);

    __m256 n = n1 / n2;
    __m256 sinT2 = n * n * (ConstOne - cosX * cosX);
    __m256 newCosX = sroot(ConstOne - sinT2);
    __m256 totalInternalReflection = (sinT2 > ConstOne);
    cosX = blend_ps(cosX, newCosX, cond & !totalInternalReflection);

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

            if (all_set(early_return)) return result;

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

            if (all_set(early_return)) return result;

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
            info.fromInside = blend_ps(info.fromInside, MaskFalse, cond);
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

    if (all_set(early_return)) return result;

    //calculate discriminant
    __m256 discr = b * b - c;

    //a negative discriminant corresponds to ray missing sphere
    {
        __m256 cond = (discr < ConstZero);
        result = blend_ps(result, MaskFalse, cond);
        early_return = blend_ps(early_return, MaskTrue, cond);
    }

    if (all_set(early_return)) return result;

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
            (normalize((rayPos + (rayDir * dist)) - spherexyz)) * blend_ps(ConstOne, -ConstOne, fromInside),
            check);
        result = blend_ps(result, MaskTrue, check);
    }

    return result;
}

#define SCENE 1

static
void TestSceneTrace(Scene& scene, m256x3 rayPos, m256x3 rayDir, SRayHitInfo& hitInfo, __m256 hasTraceTerminated)
{
    i32 objectIndex = 0;
    for (i32 quadIndex = 0; quadIndex < scene.NumQuadObjects; quadIndex++)
    {
        __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, scene.QuadObjects[quadIndex].V0, scene.QuadObjects[quadIndex].V1, scene.QuadObjects[quadIndex].V2, scene.QuadObjects[quadIndex].V3)));
        hitInfo.materialIndex = _mm256_blendv_epi8(hitInfo.materialIndex, _mm256_set1_epi32(objectIndex), _mm256_castps_si256(cond));
        objectIndex++;
    }

    for (i32 sphereIndex = 0; sphereIndex < scene.NumSphereObjects; sphereIndex++)
    {
        __m256 cond = (!(hasTraceTerminated) && (TestSphereTrace(rayPos, rayDir, hitInfo, scene.SphereObjects[sphereIndex].PositionAndRadius)));
        hitInfo.materialIndex = _mm256_blendv_epi8(hitInfo.materialIndex, _mm256_set1_epi32(objectIndex), _mm256_castps_si256(cond));
        objectIndex++;
    }

    hitInfo.material = GatherMaterials(scene, hitInfo.materialIndex);
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
    for (i32 bounceIndex = 0; (bounceIndex <= c_numBounces) && !(all_set(shouldBreak)); ++bounceIndex)
    {
        // shoot a ray out into the world
        SRayHitInfo hitInfo{ 0.f };
        hitInfo.material = GetZeroedMaterial();
        hitInfo.dist = set1_ps(c_superFar);
        hitInfo.fromInside = MaskFalse;

        TestSceneTrace(scene, rayPos, rayDir, hitInfo, shouldBreak);

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
            m256x3 ambient = EquirectangularTextureSampleBilinear(Texture, SampleDir);
            //m256x3 ambient = set1x3_ps(.11f, .1f, .15f);
            ambient *= newThroughput;
            __m256 cond = (!(prevShouldBreak) && shouldBreak);
            // if this is the fist time we hit this case, we add the ambient term once
            ret = blend3_ps(newRet, newRet + ambient, cond);
            newRet = blend3_ps(newRet, ret, cond);
        }
        shouldBreak = prevShouldBreak || shouldBreak;

        if (all_set(shouldBreak)) break;

        // if (hitInfo.fromInside)
        newThroughput.x = blend_ps(newThroughput.x, newThroughput.x * exp_ps(-hitInfo.material.refractionColor.x * hitInfo.dist), hitInfo.fromInside);
        newThroughput.y = blend_ps(newThroughput.y, newThroughput.y * exp_ps(-hitInfo.material.refractionColor.y * hitInfo.dist), hitInfo.fromInside);
        newThroughput.z = blend_ps(newThroughput.z, newThroughput.z * exp_ps(-hitInfo.material.refractionColor.z * hitInfo.dist), hitInfo.fromInside);

        // get the pre-fresnel chances
        __m256 specularChance = hitInfo.material.specularChance;
        __m256 refractionChance = hitInfo.material.refractionChance;

        __m256 rayProbability = ConstOne;

        {
            __m256 hasSpecularChance = specularChance > ConstZero;

#if 1
            __m256 n1 = blend_ps(ConstOne, hitInfo.material.IOR, hitInfo.fromInside);
            __m256 n2 = blend_ps(hitInfo.material.IOR, ConstOne, hitInfo.fromInside);
            __m256 f0 = hitInfo.material.specularChance;
            __m256 f90 = ConstOne;
            __m256 newSpecularChance = FresnelReflectAmount(n1, n2, hitInfo.normal, newRayDir, f0, f90);
#else
            __m256 newSpecularChance = specularChance;
#endif
            __m256 chanceMultiplier = (1.f - newSpecularChance) / (1.f - hitInfo.material.specularChance);

            specularChance = blend_ps(specularChance, newSpecularChance, hasSpecularChance);
            refractionChance = blend_ps(refractionChance, refractionChance * chanceMultiplier, hasSpecularChance);
        }

        __m256 raySelectRoll = Randomf3201_ps(rngState);

        __m256 doSpecularMask = (specularChance > ConstZero) && (raySelectRoll < specularChance);
        __m256 doRefractionMask = (!doSpecularMask) && (refractionChance > ConstZero) && (raySelectRoll < (specularChance + refractionChance));
        __m256 doDiffuseMask = (!doSpecularMask) && (!doRefractionMask);

        //__m256 doSpecular   = blend_ps(ConstZero, ConstOne, doSpecularMask  );
        //__m256 doRefraction = blend_ps(ConstZero, ConstOne, doRefractionMask);

        __m256 diffuseChance = max_ps(1.f - (specularChance + refractionChance), ConstZero);

        rayProbability = blend_ps(rayProbability, specularChance, doSpecularMask);
        rayProbability = blend_ps(rayProbability, refractionChance, doRefractionMask);
        rayProbability = blend_ps(rayProbability, diffuseChance, doDiffuseMask);
        rayProbability = max_ps(rayProbability, ConstOne * 0.001f);

        // update the ray position
        __m256 doRefractionSign = blend_ps(ConstOne, -ConstOne, doRefractionMask); //1.f - (2.f * doRefraction);
        newRayPos += (newRayDir * hitInfo.dist) + (doRefractionSign * hitInfo.normal * c_rayPosNormalNudge);

        // calculate new ray direction, in a cosine weighted hemisphere oriented at normal
        {
            m256x3 diffuseRayDir = normalize(hitInfo.normal + RandomUnitVector_ps(rngState));

            // reflect
            m256x3 specularRayDir = newRayDir - 2.f * hitInfo.normal * dot(newRayDir, hitInfo.normal);
            __m256 specularRoughnessSqrd = hitInfo.material.specularRoughness * hitInfo.material.specularRoughness;
            specularRayDir = normalize(lerp(specularRayDir, diffuseRayDir, specularRoughnessSqrd));

            __m256 IOR = blend_ps(1.0f / hitInfo.material.IOR, hitInfo.material.IOR, hitInfo.fromInside);
            __m256 refractionRoughnessSquared = hitInfo.material.refractionRoughness * hitInfo.material.refractionRoughness;
            m256x3 refractionRayDir = rfrct(newRayDir, hitInfo.normal, IOR);
            refractionRayDir = normalize(lerp(refractionRayDir, normalize(RandomUnitVector_ps(rngState) - hitInfo.normal), refractionRoughnessSquared));

#if 0
            newRayDir = normalize(lerp(diffuseRayDir, specularRayDir, doSpecular));
            newRayDir = normalize(lerp(newRayDir, refractionRayDir, doRefraction));
#else
            newRayDir = blend3_ps(diffuseRayDir, specularRayDir, doSpecularMask);
            newRayDir = blend3_ps(newRayDir, refractionRayDir, doRefractionMask);
#endif
        }

        // Add in emissive lighting
        newRet += hitInfo.material.emissive * newThroughput;

        // update the colorMultiplier
#if 0
        m256x3 colorFactor = lerp(hitInfo.material.albedo, hitInfo.material.specularColor, doSpecular);
        newThroughput = blend3_ps(newThroughput, newThroughput * colorFactor, doRefraction == ConstZero);
#else
        m256x3 colorFactor = blend3_ps(hitInfo.material.albedo, hitInfo.material.specularColor, doSpecularMask);
        newThroughput = blend3_ps(newThroughput, newThroughput * colorFactor, !doRefractionMask);
#endif
        // since we chose randomly between diffuse, specular, refract,
        // we need to account for the times we didn't do one or the other.
        newThroughput = newThroughput / rayProbability;

        // Russian Roulette
        // As the throughput gets smaller, the ray is more likely to get terminated early.
        // Survivors have their value boosted to make up for fewer samples being in the average.
        {
            __m256 p = max_ps(newThroughput.x, max_ps(newThroughput.y, newThroughput.z));
            __m256 rouletteTermination = (Randomf3201_ps(rngState) > p);

            // Add the energy we 'lose' by randomly terminating paths
            newThroughput = blend3_ps(newThroughput * (1.0f / p), newThroughput, rouletteTermination);
        }

        // Conditionally write back persistent (non-local) values
        ret = blend3_ps(newRet, ret, shouldBreak);
        throughput = blend3_ps(newThroughput, throughput, shouldBreak);
        rayPos = blend3_ps(newRayPos, rayPos, shouldBreak);
        rayDir = blend3_ps(newRayDir, rayDir, shouldBreak);
    }

    return ret; // return pixel color
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



    // The ray starts at the camera position (the origin)
    m256x3 rayPosition = m256x3{ ConstZero, ConstZero, ConstZero };

    // calculate the camera distance
    f32 cameraDistance = 1.0f / tan(c_FOVDegrees * 0.5f * c_pi / 180.0f);

    // calculate coordinates of the ray target on the imaginary pixel plane.
    // -1 to +1 on x,y axis. 1 unit away on the z axis
    m256x2 jitter = m256x2{ Randomf3201_ps(rngState_epi), Randomf3201_ps(rngState_epi) } - .5f;
    m256x2 rayTargetxy = ((fragCoord + jitter) / iResolution) * 2.0f - m256x2{ ConstOne, ConstOne };
    m256x3 rayTarget = m256x3{ rayTargetxy.x, rayTargetxy.y, set1_ps(cameraDistance) };

    // correct for aspect ratio
    __m256 aspectRatio = iResolution.x / iResolution.y;
    rayTarget.y /= aspectRatio;

    // calculate a normalized vector for the ray direction.
    // it's pointing from the ray position to the ray target.
    m256x3 rayDir = normalize(rayTarget - rayPosition);
    rayDir.z = rayDir.z * -1.f;

    m256x3 cameraPosition = m256x3{ ConstZero, ConstZero, ConstOne * 40.f };

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

static constexpr i32 NumMaxThreads = 1024;
static bool threadpooluninitialized = true;
static WorkerThreadData WorkData[NumMaxThreads];

static bool scene_uninitialized = true;

static WORK_QUEUE_CALLBACK(DoWorkerThreadWork) // test callback function
{
    WorkerThreadData* WorkerData = (WorkerThreadData*)Data;
    RenderTile(WorkerData->BufferInfo, WorkerData->TileInfo, WorkerData->Texture);
}

static work_queue* Queue;



//i32 AddMaterialToScene(Scene& ScenePtr, SMaterialInfo NewMaterial)
//{
//    scene.Materials[scene.NumMaterials++] = NewMaterial;
//    return scene.NumMaterials;
//}

static i32 AddMaterialToScene(Scene& scene, SceneMaterial material)
{
    scene.materials.albedoR[scene.NumMaterials] = material.albedo.x;
    scene.materials.albedoG[scene.NumMaterials] = material.albedo.x;
    scene.materials.albedoB[scene.NumMaterials] = material.albedo.x;
    scene.materials.emissiveR[scene.NumMaterials] = material.emissive.x;
    scene.materials.emissiveG[scene.NumMaterials] = material.emissive.y;
    scene.materials.emissiveB[scene.NumMaterials] = material.emissive.z;
    scene.materials.specularChance[scene.NumMaterials] = material.specularChance;
    scene.materials.specularRoughness[scene.NumMaterials] = material.specularRoughness;
    scene.materials.specularColorR[scene.NumMaterials] = material.specularColor.x;
    scene.materials.specularColorG[scene.NumMaterials] = material.specularColor.y;
    scene.materials.specularColorB[scene.NumMaterials] = material.specularColor.z;
    scene.materials.IOR[scene.NumMaterials] = material.IOR;
    scene.materials.refractionChance[scene.NumMaterials] = material.refractionChance;
    scene.materials.refractionRoughness[scene.NumMaterials] = material.refractionRoughness;
    scene.materials.refractionColorR[scene.NumMaterials] = material.refractionColor.x;
    scene.materials.refractionColorG[scene.NumMaterials] = material.refractionColor.y;
    scene.materials.refractionColorB[scene.NumMaterials] = material.refractionColor.z;
    return scene.NumMaterials++;
}

static i32 AddQuadObjectToScene(Scene& ScenePtr, QuadSceneObject NewQuadObject)
{
    scene.QuadObjects[scene.NumQuadObjects++] = NewQuadObject;
    return scene.NumQuadObjects;
}

static i32 AddSphereObjectToScene(Scene& ScenePtr, SphereSceneObject NewSphereObject)
{
    scene.SphereObjects[scene.NumSphereObjects++] = NewSphereObject;
    return scene.NumQuadObjects;
}

static void InitializeScene()
{
    // to move the scene around, since we can't move the camera yet
#if SCENE == 1
    scene.sceneTranslation = set1x3_ps(0.0f, 0.0f, 10.0f);
#else
    m256x3 sceneTranslation = set1x3_ps(0.0f, 0.0f, 0.0f);
#endif
    scene.sceneTranslation4 = m256x4{ scene.sceneTranslation.x, scene.sceneTranslation.y, scene.sceneTranslation.z, ConstZero };

   // floor material
    {
        QuadSceneObject NewQuadObject{ 0 };
        NewQuadObject.V0 = set1x3_ps(-25.0f, -12.5f, 5.0f)  + scene.sceneTranslation;
        NewQuadObject.V1 = set1x3_ps(25.0f, -12.5f, 5.0f)   + scene.sceneTranslation;
        NewQuadObject.V2 = set1x3_ps(25.0f, -12.5f, -5.0f)  + scene.sceneTranslation;
        NewQuadObject.V3 = set1x3_ps(-25.0f, -12.5f, -5.0f) + scene.sceneTranslation;
        AddQuadObjectToScene(scene, NewQuadObject);

        SceneMaterial NewMaterial{0};
        NewMaterial.albedo = f32x3{0.7f, 0.7f, 0.7f};
        AddMaterialToScene(scene, NewMaterial);
    }

    // striped background
    {
        QuadSceneObject NewQuadObject{ 0 };
        NewQuadObject.V0 = set1x3_ps(-25.0f, -1.5f, 5.0f);
        NewQuadObject.V1 = set1x3_ps(25.0f, -1.5f, 5.0f);
        NewQuadObject.V2 = set1x3_ps(25.0f, -10.5f, 5.0f);
        NewQuadObject.V3 = set1x3_ps(-25.0f, -10.5f, 5.0f);
        AddQuadObjectToScene(scene, NewQuadObject);

        SceneMaterial NewMaterial{ 0 };
        //m256x3 hitPos = rayPos + rayDir * hitInfo.dist;
        //__m256 shade = round_floor(fract(hitPos.x) * 2.0f);
        //NewMaterial.albedo = m256x3{ shade, shade, shade };
        NewMaterial.albedo = f32x3{ .35f, .35f, .35f };
        AddMaterialToScene(scene, NewMaterial);
    }

    // ceiling
    {
        QuadSceneObject NewQuadObject{ 0 };
        NewQuadObject.V0 = set1x3_ps(-7.5f, 12.5f, 5.0f)  + scene.sceneTranslation;
        NewQuadObject.V1 = set1x3_ps(7.5f, 12.5f, 5.0f)   + scene.sceneTranslation;
        NewQuadObject.V2 = set1x3_ps(7.5f, 12.5f, -5.0f)  + scene.sceneTranslation;
        NewQuadObject.V3 = set1x3_ps(-7.5f, 12.5f, -5.0f) + scene.sceneTranslation;
        AddQuadObjectToScene(scene, NewQuadObject);

        SceneMaterial NewMaterial{ 0 };
        NewMaterial.albedo = f32x3{ 0.7f, 0.7f, 0.7f };
        AddMaterialToScene(scene, NewMaterial);

    }
    // light
    {
        QuadSceneObject NewQuadObject{ 0 };
        NewQuadObject.V0 = set1x3_ps(-5.0f, 12.4f, 2.5f) + scene.sceneTranslation;
        NewQuadObject.V1 = set1x3_ps(5.0f, 12.4f, 2.5f)  + scene.sceneTranslation;
        NewQuadObject.V2 = set1x3_ps(5.0f, 12.4f, -2.5f) + scene.sceneTranslation;
        NewQuadObject.V3 = set1x3_ps(-5.0f, 12.4f, -2.5f) + scene.sceneTranslation;
        AddQuadObjectToScene(scene, NewQuadObject);

        SceneMaterial NewMaterial{ 0 };
        //NewMaterial.albedo = f32x3{ 0.7f, 0.7f, 0.7f };
        NewMaterial.emissive = (f32x3{ 1.0f, 0.9f, 0.7f } * 20.0f);
        AddMaterialToScene(scene, NewMaterial);
    }

    const i32 c_numSpheres = 7;
    for (i32 sphereIndex = 0; sphereIndex < c_numSpheres; sphereIndex += 1)
    {
        SphereSceneObject NewObject;
        NewObject.PositionAndRadius = set1x4_ps(-18.0f + 6.0f * (f32)(sphereIndex), -8.0f, 0.0f, 2.8f) + scene.sceneTranslation4;
        AddSphereObjectToScene(scene, NewObject);

        SceneMaterial NewMaterial{ 0 };

        f32 r = (((f32)sphereIndex) / (c_numSpheres - 1)) * 0.5f;

        NewMaterial.specularChance      = 0.02f;
        NewMaterial.IOR                 = 1.1f;
        NewMaterial.refractionChance    = 1.0f;
        NewMaterial.albedo              = f32x3{ 0.9f, 0.25f, 0.25f };
        NewMaterial.emissive            = f32x3{0.0f, 0.0f, 0.0f};
        NewMaterial.refractionColor     = f32x3{0.0f, 0.5f, 1.0f};
        NewMaterial.specularColor       = f32x3{1.0f, 1.0f, 1.0f} * 0.8f;
        NewMaterial.specularRoughness   = r;
        NewMaterial.refractionRoughness = r;

        AddMaterialToScene(scene, NewMaterial);
    }
}

void DemofoxRenderOptV1(f32* BufferOut, i32 BufferWidth, i32 BufferHeight, i32 NumTilesX, i32 NumTilesY, i32 TileWidth, i32 TileHeight, i32 NumChannels,
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

    if (scene_uninitialized)
    {
        InitializeScene();
        scene_uninitialized = false;
    }

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