#include "demofox_path_tracing.h"

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
const int c_numBounces = 1;

// how many renders per frame - to get around the vsync limitation.
const int c_numRendersPerFrame = 1;

const f32 c_pi = 3.14159265359f;
const f32 c_twopi = 2.0f * c_pi;

struct SRayHitInfo
{
    __m256 dist;
    m256x3 normal;
    m256x3 albedo;
    m256x3 emissive;
};

//uint wang_hash(inout uint seed)
//{
//    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
//    seed *= uint(9);
//    seed = seed ^ (seed >> 4);
//    seed *= uint(0x27d4eb2d);
//    seed = seed ^ (seed >> 15);
//    return seed;
//}
//
//__m256 Random__m25601(inout uint state)
//{
//    return __m256(wang_hash(state)) / 4294967296.0;
//}
//
//m256x3 RandomUnitVector(inout uint state)
//{
//    __m256 z = Random__m25601(state) * 2.0f - 1.0f;
//    __m256 a = Random__m25601(state) * c_twopi;
//    __m256 r = sqrt(1.0f - z * z);
//    __m256 x = r * cos(a);
//    __m256 y = r * sin(a);
//    return m256x3(x, y, z);
//}

__m256 ScalarTriple(m256x3 u, m256x3 v, m256x3 w)
{
    return dot(cross(u, v), w);
}

__m256 TestQuadTrace( m256x3 rayPos,  m256x3 rayDir, SRayHitInfo info,  m256x3 a,  m256x3 b,  m256x3 c,  m256x3 d)
{
    // calculate normal and flip vertices order if needed
    m256x3 normal = normalize(cross(c - a, c - b));
    //if (dot(normal, rayDir) > set1_ps(0.0f))
    __m256 mask = dot(normal, rayDir) > set1_ps(0.0f);
    {
        normal = blend3_ps(normal, normal * set1x3_ps(-1.0f, -1.0f, -1.0f), mask);

        m256x3 temp = d;
        d = blend3_ps(d, a   , mask) ;
        a = blend3_ps(a, temp, mask);

        temp = b;
        b = blend3_ps(b, c   , mask);
        c = blend3_ps(c, temp, mask);
    }

    m256x3 p = rayPos;
    m256x3 q = rayPos + rayDir;
    m256x3 pq = q - p;
    m256x3 pa = a - p;
    m256x3 pb = b - p;
    m256x3 pc = c - p;

    // determine which triangle to test against by testing against diagonal first
    m256x3 m = cross(pc, pq);
    __m256 v = dot(pa, m);
    m256x3 intersectPos;
    //if (all_set(v >= set1_ps(0.0f)))
    mask = v >= set1_ps(0.0f);
    {
        // test against triangle a,b,c
        __m256 u = negate_ps(dot(pb, m));
        //if (all_set(u < set1_ps(0.0f))) return set1_ps(0);
        mask = bitwise_and(mask, u < set1_ps(0.0f));
        __m256 w = ScalarTriple(pq, pb, pa);
        //if (all_set(w < set1_ps(0.0f))) return set1_ps(0);
        mask = bitwise_and(mask, w < set1_ps(0.0f));
        __m256 denom = set1_ps(1.0f) / (u + v + w);
        u = u * denom;
        v = v * denom;
        w = w * denom;
        intersectPos = mul(u, a) + mul(v, b) + mul(w, c);
    
    //else
    
        m256x3 pd = d - p;
        __m256 u2 = dot(pd, m);
        //if (all_set(u < set1_ps(0.0f))) return set1_ps(0);
        mask = bitwise_and(mask, u2 < set1_ps(0.0f));
        __m256 w2 = ScalarTriple(pq, pa, pd);
       // if (all_set(w < set1_ps(0.0f))) return set1_ps(0);
        mask = bitwise_and(mask, w2 < set1_ps(0.0f));
        __m256 v2 = negate_ps(v);
        denom = set1_ps(1.0f) / (u2 + v2 + w2);
        u2 = u2 * denom;
        v2 = v2 * denom;
        w2 = w2 * denom;
        m256x3 intersectPos2 = mul(u2, a) + mul(v2, d) + mul(w2, c);


        u = blend_ps(u2, u, mask);
        v = blend_ps(v2, v, mask);
        w = blend_ps(w2, w, mask);
        intersectPos = blend3_ps(intersectPos2, intersectPos, mask);
    }

    __m256 dist;
    //if (abs_ps(rayDir.x) > set1_ps(0.1f))
    mask = abs_ps(rayDir.x) > set1_ps(0.1f);
    {
    }
    //else if (abs_ps(rayDir.y) > 0.1f)

    mask = bitwise_and_not(mask, abs_ps(rayDir.y) > set1_ps(0.1f));
    {
        dist = blend_ps((intersectPos.x - rayPos.x) / rayDir.x, (intersectPos.y - rayPos.y) / rayDir.y, mask);
    }
    //else
    mask = bitwise_not(mask);
    {
        dist = blend_ps(dist, (intersectPos.z - rayPos.z) / rayDir.z, mask);
    }

    //if (dist > c_minimumRayHitTime && dist < info.dist)
    mask = bitwise_and(dist > set1_ps(c_minimumRayHitTime), dist < info.dist);
    {
        info.dist   = blend_ps(info.dist, dist, mask);
        info.normal = blend3_ps(info.normal, normal, mask);
    }
    return mask;
}

__m256 TestSphereTrace(m256x3 rayPos, m256x3 rayDir, SRayHitInfo info, m256x4 sphere)
{
    __m256 mask = set1_ps(0.f);
    m256x3 spherexyz = { sphere.x, sphere.y, sphere.z };
    m256x3 m = rayPos - spherexyz;
    __m256 b = dot(m, rayDir);
    __m256 c = dot(m, m) - sphere.w * sphere.w;
    //if (all_set( bitwise_and(c > set1_ps(0.0), b > set1_ps(0.0)))) return set1_ps(0);

    mask = bitwise_and(c > set1_ps(0.0), b > set1_ps(0.0));

    __m256 discr = b * b - c;

    //if (all_set(discr < set1_ps(0.0))) return set1_ps(0);
    mask = bitwise_and(mask, discr < set1_ps(0.0));

    __m256 dist = negate_ps(sroot(discr)) - b;
    __m256 fromInside = bitwise_and(mask, (dist < set1_ps(0.0f)));
    {
        dist = blend_ps(dist, sroot(discr) - b, fromInside);
    }

    mask = bitwise_and(dist > set1_ps(c_minimumRayHitTime), dist < info.dist);
    {
        info.dist = dist;
        info.normal = mul(normalize((rayPos + mul(rayDir, dist)) - spherexyz), blend_ps(set1_ps(1.0f), set1_ps(-1.0f), fromInside));
    }
    return mask;
}

void TestSceneTrace( m256x3 rayPos,  m256x3 rayDir, SRayHitInfo& hitInfo)
{
    // to move the scene around, since we can't move the camera yet
    m256x3 sceneTranslation = { 0.0f, 0.0f, 10.0f };
    m256x4 sceneTranslation4 = set1x4_ps(
        sceneTranslation.x.m256_f32[0], 
        sceneTranslation.y.m256_f32[0],
        sceneTranslation.z.m256_f32[0],
        0.0f);

    __m256 mask;

    // back wall
    {
        m256x3 A = set1x3_ps(-12.6f, -12.6f, 25.0f) + sceneTranslation;
        m256x3 B = set1x3_ps(12.6f, -12.6f, 25.0f) + sceneTranslation;
        m256x3 C = set1x3_ps(12.6f, 12.6f, 25.0f) + sceneTranslation;
        m256x3 D = set1x3_ps(-12.6f, 12.6f, 25.0f) + sceneTranslation;
        mask = (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo   = blend3_ps(hitInfo.albedo,   set1x3_ps(0.7f, 0.7f, 0.7f), mask);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
        }
    }

    // floor
    {
        m256x3 A = set1x3_ps(-12.6f, -12.45f, 25.0f) + sceneTranslation;
        m256x3 B = set1x3_ps(12.6f, -12.45f, 25.0f) + sceneTranslation;
        m256x3 C = set1x3_ps(12.6f, -12.45f, 15.0f) + sceneTranslation;
        m256x3 D = set1x3_ps(-12.6f, -12.45f, 15.0f) + sceneTranslation;
        mask = (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo = blend3_ps(hitInfo.albedo,     set1x3_ps(0.7f, 0.7f, 0.7f), mask);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
        }
    }

    // cieling
    {
        m256x3 A = set1x3_ps(-12.6f, 12.5f, 25.0f) + sceneTranslation;
        m256x3 B = set1x3_ps(12.6f, 12.5f, 25.0f) + sceneTranslation;
        m256x3 C = set1x3_ps(12.6f, 12.5f, 15.0f) + sceneTranslation;
        m256x3 D = set1x3_ps(-12.6f, 12.5f, 15.0f) + sceneTranslation;
        mask = (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo   = blend3_ps(hitInfo.albedo, set1x3_ps(0.7f, 0.7f, 0.7f), mask);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
        }
    }

    // left wall
    {
        m256x3 A = set1x3_ps(-12.5f, -12.6f, 25.0f) + sceneTranslation;
        m256x3 B = set1x3_ps(-12.5f, -12.6f, 15.0f) + sceneTranslation;
        m256x3 C = set1x3_ps(-12.5f, 12.6f, 15.0f) + sceneTranslation;
        m256x3 D = set1x3_ps(-12.5f, 12.6f, 25.0f) + sceneTranslation;
        mask = (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo   = blend3_ps(hitInfo.albedo,   set1x3_ps(0.7f, 0.1f, 0.1f), mask);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
        }
    }

    // right wall 
    {
        m256x3 A = set1x3_ps(12.5f, -12.6f, 25.0f) + sceneTranslation;
        m256x3 B = set1x3_ps(12.5f, -12.6f, 15.0f) + sceneTranslation;
        m256x3 C = set1x3_ps(12.5f, 12.6f, 15.0f) + sceneTranslation;
        m256x3 D = set1x3_ps(12.5f, 12.6f, 25.0f) + sceneTranslation;
        mask = (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.1f, 0.7f, 0.1f), mask);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
        }
    }

    // light
    {
        m256x3 A = set1x3_ps(-5.0f, 12.4f, 22.5f) + sceneTranslation;
        m256x3 B = set1x3_ps(5.0f, 12.4f, 22.5f) + sceneTranslation;
        m256x3 C = set1x3_ps(5.0f, 12.4f, 17.5f) + sceneTranslation;
        m256x3 D = set1x3_ps(-5.0f, 12.4f, 17.5f) + sceneTranslation;
        mask = (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(1.0f, 0.9f, 0.7f), mask);

            __m256 emissive_scale = set1_ps(20.0f);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, mul(hitInfo.emissive, emissive_scale), mask);
        }
    }

    mask = (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps(-9.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4));
    {
        hitInfo.albedo   = blend3_ps(hitInfo.albedo, set1x3_ps(0.9f, 0.9f, 0.75f), mask);
        hitInfo.emissive = blend3_ps(hitInfo.albedo, set1x3_ps(0.0f, 0.0f, 0.0f) , mask);
    }

    mask = (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps(0.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4));
    {
        hitInfo.albedo   = blend3_ps(hitInfo.albedo, set1x3_ps(0.9f, 0.75f, 0.9f), mask);
        hitInfo.emissive = blend3_ps(hitInfo.albedo, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
    }

    mask = (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps(9.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4));
    {
        hitInfo.albedo   = blend3_ps(hitInfo.albedo, set1x3_ps(0.75f, 0.9f, 0.9f), mask);
        hitInfo.emissive = blend3_ps(hitInfo.albedo, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
    }
}

m256x3 GetColorForRay( m256x3 startRayPos,  m256x3 startRayDir, u32& rngState)
{
    m256x3 ret = set1x3_ps(0.0f, 0.0f, 0.0f);
    m256x3 throughput = set1x3_ps(1.0f, 1.0f, 1.0f);
    m256x3 rayPos = startRayPos;
    m256x3 rayDir = startRayDir;

   // for (int bounceIndex = 0; bounceIndex <= c_numBounces; ++bounceIndex)
    {
        SRayHitInfo hitInfo;
        __m256 superFar_ps = set1_ps(c_superFar);
        hitInfo.dist = set1_ps(c_superFar);
        TestSceneTrace(rayPos, rayDir, hitInfo);
        //if (hitInfo.dist == superFar_ps)
        //{
        //    //ret += texture(iChannel1, rayDir).rgb * throughput;
        //    break;
        //}
        rayPos = (rayPos + mul(rayDir, hitInfo.dist)) + mul(hitInfo.normal, set1_ps(c_rayPosNormalNudge));
        rayDir = normalize(hitInfo.normal); //+ RandomUnitVector(rngState));
        ret = ret + hitInfo.emissive * throughput;
        throughput = throughput * hitInfo.albedo;
    }
    return ret;
}

m256x3 mainImage(m256x2 fragCoord, f32 Width, f32 Height)
{
    m256x2 iResolution = set1x2_ps(Width, Height);
    __m256 iFrame = set1_ps(0.f);
    u32 rngState = 0; // u32(u32(fragCoord.x) * u32(1973) + u32(fragCoord.y) * u32(9277) + u32(iFrame) * u32(26699)) | u32(1);
    m256x3 rayPosition = set1x3_ps(0.0f, 0.0f, 0.0f);
    __m256 cameraDistance = set1_ps(1.0f / tan(c_FOVDegrees * 0.5f * c_pi / 180.0f));
    m256x3 rayTarget;
    rayTarget.x = (fragCoord.x / iResolution.x) * set1_ps(2.0f) - set1_ps(1.0f);
    rayTarget.y = (fragCoord.y / iResolution.y) * set1_ps(2.0f) - set1_ps(1.0f);
    rayTarget.z = cameraDistance;
    __m256 aspectRatio = iResolution.x / iResolution.y;
    rayTarget.y = rayTarget.y / aspectRatio;
    m256x3 rayDir = normalize(rayTarget - rayPosition);
    m256x3 color = set1x3_ps(0.0f, 0.0f, 0.0f);
    //for (int index = 0; index < c_numRendersPerFrame; ++index)
    //    color = color + GetColorForRay(rayPosition, rayDir, rngState) / set1x3_ps(c_numRendersPerFrame, c_numRendersPerFrame, c_numRendersPerFrame);

    color = GetColorForRay(rayPosition, rayDir, rngState) / set1x3_ps(c_numRendersPerFrame, c_numRendersPerFrame, c_numRendersPerFrame);

    m256x3 lastFrameColor = color; //texture(iChannel0, fragCoord / iResolution).rgb;
    color = lerp(lastFrameColor, color, set1_ps(1.0f) / (iFrame + set1_ps(1.f)));

    return color;
}


void DemofoxRender(f32* BufferOut, i32 BufferWidth, i32 BufferHeight, i32 NumChannels)
{

    m256x2 fragCoord;

    i32 YHeight = BufferHeight;
    i32 XWidth = (BufferWidth / LANE_COUNT);

    __m256 XLaneOffsets = set_ps(7.f, 6.f, 5.f, 4.f, 3.f, 2.f, 1.f, 0.f);

    f32* BufferPos = BufferOut;

    for (i32 Y = 0; Y < YHeight; Y++)
    {
        fragCoord.y = set1_ps((f32)(YHeight-1-Y));
        for (i32 X = 0; X < XWidth; X++)
        {
            fragCoord.x = set1_ps((f32)X * LANE_COUNT) + XLaneOffsets;
            m256x3 color = mainImage(fragCoord, (f32)BufferWidth, (f32)BufferHeight);

            non_temporal_store(BufferPos, color.x);
            BufferPos += LANE_COUNT;
            non_temporal_store(BufferPos, color.y);
            BufferPos += LANE_COUNT;
            non_temporal_store(BufferPos, color.z);
            BufferPos += LANE_COUNT;
        }
    }

    


}