#include "demofox_path_tracing_simt_textured.h"

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

struct SRayHitInfo
{
    __m256 dist;
    m256x3 normal;
    m256x3 albedo;
    m256x3 emissive;
};

static
__m256 ScalarTriple(m256x3 u, m256x3 v, m256x3 w)
{
    return dot(cross(u, v), w);
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

    // back wall
    {
        m256x3 A = set1x3_ps(-12.6f, -12.6f, 25.0f) + sceneTranslation;
        m256x3 B = set1x3_ps(12.6f, -12.6f, 25.0f) + sceneTranslation;
        m256x3 C = set1x3_ps(12.6f, 12.6f, 25.0f) + sceneTranslation;
        m256x3 D = set1x3_ps(-12.6f, 12.6f, 25.0f) + sceneTranslation;

        __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
        {
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.7f, 0.7f, 0.7f), cond);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), cond);
        }
    }

    // floor
    {
        m256x3 A = set1x3_ps(-12.6f, -12.45f, 25.0f) + sceneTranslation;
        m256x3 B = set1x3_ps(12.6f, -12.45f, 25.0f) + sceneTranslation;
        m256x3 C = set1x3_ps(12.6f, -12.45f, 15.0f) + sceneTranslation;
        m256x3 D = set1x3_ps(-12.6f, -12.45f, 15.0f) + sceneTranslation;

        __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
        {
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.7f, 0.7f, 0.7f), cond);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), cond);
        }
    }

    // cieling
    {
        m256x3 A = set1x3_ps(-12.6f, 12.5f, 25.0f) + sceneTranslation;
        m256x3 B = set1x3_ps(12.6f, 12.5f, 25.0f) + sceneTranslation;
        m256x3 C = set1x3_ps(12.6f, 12.5f, 15.0f) + sceneTranslation;
        m256x3 D = set1x3_ps(-12.6f, 12.5f, 15.0f) + sceneTranslation;

        {
            __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.7f, 0.7f, 0.7f), cond);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), cond);
        }
    }

    // left wall
    {
        m256x3 A = set1x3_ps(-12.5f, -12.6f, 25.0f) + sceneTranslation;
        m256x3 B = set1x3_ps(-12.5f, -12.6f, 15.0f) + sceneTranslation;
        m256x3 C = set1x3_ps(-12.5f, 12.6f, 15.0f) + sceneTranslation;
        m256x3 D = set1x3_ps(-12.5f, 12.6f, 25.0f) + sceneTranslation;

        {
            __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.7f, 0.1f, 0.1f), cond);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), cond);
        }
    }

    // right wall 
    {
        m256x3 A = set1x3_ps(12.5f, -12.6f, 25.0f) + sceneTranslation;
        m256x3 B = set1x3_ps(12.5f, -12.6f, 15.0f) + sceneTranslation;
        m256x3 C = set1x3_ps(12.5f, 12.6f, 15.0f) + sceneTranslation;
        m256x3 D = set1x3_ps(12.5f, 12.6f, 25.0f) + sceneTranslation;

        {
            __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.1f, 0.7f, 0.1f), cond);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), cond);
        }
    }

    // light
    {
        m256x3 A = set1x3_ps(-5.0f, 12.4f, 22.5f) + sceneTranslation;
        m256x3 B = set1x3_ps(5.0f, 12.4f, 22.5f) + sceneTranslation;
        m256x3 C = set1x3_ps(5.0f, 12.4f, 17.5f) + sceneTranslation;
        m256x3 D = set1x3_ps(-5.0f, 12.4f, 17.5f) + sceneTranslation;

        {
            __m256 cond = (!(hasTraceTerminated) && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.0f, 0.0f, 0.0f), cond);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, (set1x3_ps(1.0f, 0.9f, 0.7f) * set1_ps(20.0f)), cond);
        }
    }

    {
        __m256 cond = (!(hasTraceTerminated) && (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps(-9.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4)));
        hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.9f, 0.9f, 0.75f), cond);
        hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), cond);
    }

    {
        __m256 cond = (!(hasTraceTerminated) && (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps(0.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4)));
        hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.9f, 0.75f, 0.9f), cond);
        hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), cond);
    }

    {
        __m256 cond = (!(hasTraceTerminated) && (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps(9.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4)));
        hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.9f, 0.75f, 0.9f), cond);
        hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), cond);
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
        hitInfo.dist = set1_ps(c_superFar);
        TestSceneTrace(rayPos, rayDir, hitInfo, shouldBreak);

        // if the ray missed, we are done
        __m256 prevShouldBreak = shouldBreak;
        shouldBreak = (hitInfo.dist == set1_ps(c_superFar));
        {
            m256x3 ambient = EquirectangularTextureSample(Texture, rayDir);//set1x3_ps(.15f, .15f, .25f);
            __m256 cond = (!(prevShouldBreak) && shouldBreak);
            // if this is the fist time we hit this case, we add the ambient term once
            ret = blend3_ps(ret, ret + ambient, cond);
        }

        // update the ray position
        rayPos = blend3_ps(((rayPos + (rayDir * hitInfo.dist)) + (hitInfo.normal * c_rayPosNormalNudge)), rayPos, shouldBreak);

        // calculate new ray direction, in a cosine weighted hemisphere oriented at normal
        rayDir = blend3_ps(normalize(hitInfo.normal + RandomUnitVector_ps(rngState)), rayDir, shouldBreak);

        // add in emissive lighting
        ret = blend3_ps((ret + hitInfo.emissive * throughput), ret, shouldBreak);

        // update the colorMultiplier
        //throughput = shouldBreak ? throughput :
        //    (throughput * hitInfo.albedo);
        throughput = blend3_ps((throughput * hitInfo.albedo), throughput, shouldBreak);
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



    // The ray starts at the camera position (the origin)
    m256x3 rayPosition = m256x3{ ConstZero, ConstZero, ConstZero };

    // calculate the camera distance
    f32 cameraDistance = 1.0f / tan(c_FOVDegrees * 0.5f * c_pi / 180.0f);

    // calculate coordinates of the ray target on the imaginary pixel plane.
    // -1 to +1 on x,y axis. 1 unit away on the z axis
    m256x2 rayTargetxy = (fragCoord / iResolution) * 2.0f - m256x2{ ConstOne, ConstOne };
    m256x3 rayTarget = m256x3{ rayTargetxy.x, rayTargetxy.y, set1_ps(cameraDistance) };

    // correct for aspect ratio
    __m256 aspectRatio = iResolution.x / iResolution.y;
    rayTarget.y /= aspectRatio;

    // calculate a normalized vector for the ray direction.
    // it's pointing from the ray position to the ray target.
    m256x3 rayDir = normalize(rayTarget - rayPosition);

    // raytrace for this pixel
    m256x3 color{ ConstZero, ConstZero, ConstZero };
    for (int index = 0; index < c_numRendersPerFrame; ++index)
        color += (GetColorForRay(rayPosition, rayDir, rngState_epi, Texture) * (1.f / (c_numRendersPerFrame)));

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
WorkerThreadData WorkData[NumMaxThreads];

static WORK_QUEUE_CALLBACK(DoWorkerThreadWork) // test callback function
{
    WorkerThreadData* WorkerData = (WorkerThreadData*)Data;
    RenderTile(WorkerData->BufferInfo, WorkerData->TileInfo, WorkerData->Texture);
}

work_queue* Queue;


void DemofoxRenderSimtTextured(f32* BufferOut, i32 BufferWidth, i32 BufferHeight, i32 NumTilesX, i32 NumTilesY, i32 TileWidth, i32 TileHeight, i32 NumChannels,
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