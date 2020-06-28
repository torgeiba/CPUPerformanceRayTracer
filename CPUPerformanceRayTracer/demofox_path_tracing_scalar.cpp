#include "demofox_path_tracing_scalar.h"


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

u32 wang_hash(u32& seed)
{
    seed = u32(seed ^ u32(61)) ^ u32(seed >> u32(16));
    seed *= u32(9);
    seed = seed ^ (seed >> 4);
    seed *= u32(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

f32 Randomf3201(u32& state)
{
    return f32(wang_hash(state)) / 4294967296.0f;
}

f32x3 RandomUnitVector(u32& state)
{
    f32 z = Randomf3201(state) * 2.0f - 1.0f;
    f32 a = Randomf3201(state) * c_twopi;
    f32 r = sqrt(1.0f - z * z);
    f32 x = r * cos(a);
    f32 y = r * sin(a);
    return f32x3{ x, y, z };
}

struct SRayHitInfo
{
    f32 dist;
    f32x3 normal;
    f32x3 albedo;
    f32x3 emissive;
};

f32 ScalarTriple(f32x3 u, f32x3 v, f32x3 w)
{
    return dot(cross(u, v), w);
}

bool TestQuadTrace(f32x3 rayPos, f32x3 rayDir, SRayHitInfo& info, f32x3 a, f32x3 b, f32x3 c, f32x3 d)
{
    // calculate normal and flip vertices order if needed
    f32x3 normal = normalize(cross(c - a, c - b));
    if (dot(normal, rayDir) > 0.0f)
    {
        normal = mul(normal, -1.0f);

        f32x3 temp = d;
        d = a;
        a = temp;

        temp = b;
        b = c;
        c = temp;
    }

    f32x3 p = rayPos;
    f32x3 q = rayPos + rayDir;
    f32x3 pq = q - p;
    f32x3 pa = a - p;
    f32x3 pb = b - p;
    f32x3 pc = c - p;

    // determine which triangle to test against by testing against diagonal first
    f32x3 m = cross(pc, pq);
    f32 v = dot(pa, m);
    f32x3 intersectPos;
    if (v >= 0.0f)
    {
        // test against triangle a,b,c
        f32 u = -dot(pb, m);
        if (u < 0.0f) return false;
        f32 w = ScalarTriple(pq, pb, pa);
        if (w < 0.0f) return false;
        f32 denom = 1.0f / (u + v + w);
        u *= denom;
        v *= denom;
        w *= denom;
        intersectPos = mul(u, a) + mul(v, b) + mul(w, c);
    }
    else
    {
        f32x3 pd = d - p;
        f32 u = dot(pd, m);
        if (u < 0.0f) return false;
        f32 w = ScalarTriple(pq, pa, pd);
        if (w < 0.0f) return false;
        v = -v;
        f32 denom = 1.0f / (u + v + w);
        u *= denom;
        v *= denom;
        w *= denom;
        intersectPos = mul(u, a) + mul(v, d) + mul(w, c);
    }

    f32 dist;
    if (abs(rayDir.x) > 0.1f)
    {
        dist = (intersectPos.x - rayPos.x) / rayDir.x;
    }
    else if (abs(rayDir.y) > 0.1f)
    {
        dist = (intersectPos.y - rayPos.y) / rayDir.y;
    }
    else
    {
        dist = (intersectPos.z - rayPos.z) / rayDir.z;
    }

    if (dist > c_minimumRayHitTime && dist < info.dist)
    {
        info.dist = dist;
        info.normal = normal;
        return true;
    }

    return false;
}

bool TestSphereTrace(f32x3 rayPos, f32x3 rayDir, SRayHitInfo& info, f32x4 sphere)
{
    //get the vector from the center of this sphere to where the ray begins.
    f32x3 spherexyz = f32x3{ sphere.x, sphere.y, sphere.z };
    f32x3 m = rayPos - spherexyz;

    //get the dot product of the above vector and the ray's vector
    f32 b = dot(m, rayDir);

    f32 c = dot(m, m) - sphere.w * sphere.w;

    //exit if r's origin outside s (c > 0) and r pointing away from s (b > 0)
    if (c > 0.0 && b > 0.0)
        return false;

    //calculate discriminant
    f32 discr = b * b - c;

    //a negative discriminant corresponds to ray missing sphere
    if (discr < 0.0)
        return false;

    //ray now found to intersect sphere, compute smallest t value of intersection
    bool fromInside = false;
    f32 dist = -b - sqrt(discr);
    if (dist < 0.0f)
    {
        fromInside = true;
        dist = -b + sqrt(discr);
    }

    if (dist > c_minimumRayHitTime && dist < info.dist)
    {
        info.dist = dist;
        info.normal = mul(normalize((rayPos + mul(rayDir, dist)) - spherexyz), (fromInside ? -1.0f : 1.0f));
        return true;
    }

    return false;
}

void TestSceneTrace(f32x3 rayPos, f32x3 rayDir, SRayHitInfo& hitInfo)
{
    // to move the scene around, since we can't move the camera yet
    f32x3 sceneTranslation = f32x3{ 0.0f, 0.0f, 10.0f };
    f32x4 sceneTranslation4 = f32x4{ sceneTranslation.x, sceneTranslation.y, sceneTranslation.z, 0.0f };

    // back wall
    {
        f32x3 A = f32x3{-12.6f, -12.6f, 25.0f} + sceneTranslation;
        f32x3 B = f32x3{12.6f, -12.6f, 25.0f } + sceneTranslation;
        f32x3 C = f32x3{12.6f, 12.6f, 25.0f  } + sceneTranslation;
        f32x3 D = f32x3{-12.6f, 12.6f, 25.0f } + sceneTranslation;
        if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
        {
            hitInfo.albedo   = f32x3{0.7f, 0.7f, 0.7f};
            hitInfo.emissive = f32x3{0.0f, 0.0f, 0.0f};
        }
    }

    // floor
    {
        f32x3 A = f32x3{-12.6f, -12.45f, 25.0f} + sceneTranslation;
        f32x3 B = f32x3{12.6f, -12.45f, 25.0f } + sceneTranslation;
        f32x3 C = f32x3{12.6f, -12.45f, 15.0f } + sceneTranslation;
        f32x3 D = f32x3{-12.6f, -12.45f, 15.0f} + sceneTranslation;
        if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
        {
            hitInfo.albedo   = f32x3{0.7f, 0.7f, 0.7f};
            hitInfo.emissive = f32x3{0.0f, 0.0f, 0.0f};
        }
    }

    // cieling
    {
        f32x3 A = f32x3{-12.6f, 12.5f, 25.0f} + sceneTranslation;
        f32x3 B = f32x3{12.6f, 12.5f, 25.0f } + sceneTranslation;
        f32x3 C = f32x3{12.6f, 12.5f, 15.0f } + sceneTranslation;
        f32x3 D = f32x3{-12.6f, 12.5f, 15.0f} + sceneTranslation;
        if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
        {
            hitInfo.albedo   = f32x3{0.7f, 0.7f, 0.7f};
            hitInfo.emissive = f32x3{0.0f, 0.0f, 0.0f};
        }
    }

    // left wall
    {
        f32x3 A = f32x3{-12.5f, -12.6f, 25.0f} + sceneTranslation;
        f32x3 B = f32x3{-12.5f, -12.6f, 15.0f} + sceneTranslation;
        f32x3 C = f32x3{-12.5f, 12.6f, 15.0f } + sceneTranslation;
        f32x3 D = f32x3{-12.5f, 12.6f, 25.0f } + sceneTranslation;
        if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
        {
            hitInfo.albedo   = f32x3{0.7f, 0.1f, 0.1f};
            hitInfo.emissive = f32x3{0.0f, 0.0f, 0.0f};
        }
    }

    // right wall 
    {
        f32x3 A = f32x3{12.5f, -12.6f, 25.0f} + sceneTranslation;
        f32x3 B = f32x3{12.5f, -12.6f, 15.0f} + sceneTranslation;
        f32x3 C = f32x3{12.5f, 12.6f, 15.0f } + sceneTranslation;
        f32x3 D = f32x3{12.5f, 12.6f, 25.0f } + sceneTranslation;
        if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
        {
            hitInfo.albedo   = f32x3{0.1f, 0.7f, 0.1f};
            hitInfo.emissive = f32x3{0.0f, 0.0f, 0.0f};
        }
    }

    // light
    {
        f32x3 A = f32x3{-5.0f, 12.4f, 22.5f} + sceneTranslation;
        f32x3 B = f32x3{5.0f, 12.4f, 22.5f } + sceneTranslation;
        f32x3 C = f32x3{5.0f, 12.4f, 17.5f } + sceneTranslation;
        f32x3 D = f32x3{-5.0f, 12.4f, 17.5f} + sceneTranslation;
        if (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D))
        {
            hitInfo.albedo =   f32x3{0.0f, 0.0f, 0.0f};
            hitInfo.emissive = mul(f32x3{1.0f, 0.9f, 0.7f}, 20.0f);
        }
    }

    if (TestSphereTrace(rayPos, rayDir, hitInfo, f32x4{-9.0f, -9.5f, 20.0f, 3.0f} + sceneTranslation4))
    {
        hitInfo.albedo   = f32x3{0.9f, 0.9f, 0.75f};
        hitInfo.emissive = f32x3{0.0f, 0.0f, 0.0f };
    }

    if (TestSphereTrace(rayPos, rayDir, hitInfo, f32x4{ 0.0f, -9.5f, 20.0f, 3.0f } + sceneTranslation4))
    {
        hitInfo.albedo =   f32x3{0.9f, 0.75f, 0.9f};
        hitInfo.emissive = f32x3{0.0f, 0.0f, 0.0f };
    }

    if (TestSphereTrace(rayPos, rayDir, hitInfo, f32x4{ 9.0f, -9.5f, 20.0f, 3.0f } + sceneTranslation4))
    {
        hitInfo.albedo   = f32x3{0.75f, 0.9f, 0.9f};
        hitInfo.emissive = f32x3{0.0f, 0.0f, 0.0f };
    }
}

f32x3 GetColorForRay(f32x3 startRayPos, f32x3 startRayDir, u32& rngState)
{
    // initialize
    f32x3 ret = f32x3{ 0.0f, 0.0f, 0.0f };
    f32x3 throughput = f32x3{ 1.0f, 1.0f, 1.0f };
    f32x3 rayPos = startRayPos;
    f32x3 rayDir = startRayDir;

    for (int bounceIndex = 0; bounceIndex <= c_numBounces; ++bounceIndex)
    {
        // shoot a ray out into the world
        SRayHitInfo hitInfo;
        hitInfo.dist = c_superFar;
        TestSceneTrace(rayPos, rayDir, hitInfo);

        // if the ray missed, we are done
        if (hitInfo.dist == c_superFar)
        {
            f32x3 ambient = f32x3{ .1f, .1f, .1f };
            ret = ret + ambient; // texture(iChannel1, rayDir).rgb* throughput;
            break;
        }

        // update the ray position
        rayPos = (rayPos + mul(rayDir, hitInfo.dist)) + mul(hitInfo.normal, c_rayPosNormalNudge);

        // calculate new ray direction, in a cosine weighted hemisphere oriented at normal
        rayDir = normalize(hitInfo.normal + RandomUnitVector(rngState));

        // add in emissive lighting
        ret = ret + hitInfo.emissive * throughput;

        // update the colorMultiplier
        throughput = throughput * hitInfo.albedo;
    }

    // return pixel color
    return ret;
}

f32x3 mainImage(f32x2 fragCoord, f32x2 iResolution, f32 iFrame)
{
    // initialize a random number state based on frag coord and frame
    u32 rngState = u32(u32(fragCoord.x) * u32(1973) + u32(fragCoord.y) * u32(9277) + u32(iFrame) * u32(26699)) | u32(1);

    // The ray starts at the camera position (the origin)
    f32x3 rayPosition = f32x3{ 0.0f, 0.0f, 0.0f };

    // calculate the camera distance
    f32 cameraDistance = 1.0f / tan(c_FOVDegrees * 0.5f * c_pi / 180.0f);

    // calculate coordinates of the ray target on the imaginary pixel plane.
    // -1 to +1 on x,y axis. 1 unit away on the z axis
    f32x2 rayTargetxy = mul((fragCoord / iResolution), 2.0f) - f32x2{ 1.0f, 1.0f };
    f32x3 rayTarget = f32x3{ rayTargetxy.x, rayTargetxy.y, cameraDistance };

    // correct for aspect ratio
    f32 aspectRatio = iResolution.x / iResolution.y;
    rayTarget.y /= aspectRatio;

    // calculate a normalized vector for the ray direction.
    // it's pointing from the ray position to the ray target.
    f32x3 rayDir = normalize(rayTarget - rayPosition);

    // raytrace for this pixel
    f32x3 color = { 0.0f, 0.0f, 0.0f };
    for (int index = 0; index < c_numRendersPerFrame; ++index)
        color = color + mul(GetColorForRay(rayPosition, rayDir, rngState), (1.f/ (c_numRendersPerFrame)));

    // show the result
    return color;
}









































































#if 0
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
    f32x3 normal;
    f32x3 albedo;
    f32x3 emissive;
};

//u32 wang_hash(inout u32 seed)
//{
//    seed = u32(seed ^ u32(61)) ^ u32(seed >> u32(16));
//    seed *= u32(9);
//    seed = seed ^ (seed >> 4);
//    seed *= u32(0x27d4eb2d);
//    seed = seed ^ (seed >> 15);
//    return seed;
//}
//
//__m256 Random__m25601(inout u32 state)
//{
//    return __m256(wang_hash(state)) / 4294967296.0;
//}
//
//f32x3 RandomUnitVector(inout u32 state)
//{
//    __m256 z = Random__m25601(state) * 2.0f - 1.0f;
//    __m256 a = Random__m25601(state) * c_twopi;
//    __m256 r = sqrt(1.0f - z * z);
//    __m256 x = r * cos(a);
//    __m256 y = r * sin(a);
//    return f32x3(x, y, z);
//}

__m256 ScalarTriple(f32x3 u, f32x3 v, f32x3 w)
{
    return dot(cross(u, v), w);
}

__m256 TestQuadTrace(f32x3 rayPos, f32x3 rayDir, SRayHitInfo info, f32x3 a, f32x3 b, f32x3 c, f32x3 d)
{
    // calculate normal and flip vertices order if needed
    f32x3 normal = normalize(cross(c - a, c - b));
    //if (dot(normal, rayDir) > set1_ps(0.0f))
    __m256 mask = dot(normal, rayDir) > set1_ps(0.0f);
    {
        normal = blend3_ps(normal, normal * set1x3_ps(-1.0f, -1.0f, -1.0f), mask);

        f32x3 temp = d;
        d = blend3_ps(d, a, mask);
        a = blend3_ps(a, temp, mask);

        temp = b;
        b = blend3_ps(b, c, mask);
        c = blend3_ps(c, temp, mask);
    }

    f32x3 p = rayPos;
    f32x3 q = rayPos + rayDir;
    f32x3 pq = q - p;
    f32x3 pa = a - p;
    f32x3 pb = b - p;
    f32x3 pc = c - p;

    // determine which triangle to test against by testing against diagonal first
    f32x3 m = cross(pc, pq);
    __m256 v = dot(pa, m);
    f32x3 intersectPos;
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

        f32x3 pd = d - p;
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
        f32x3 intersectPos2 = mul(u2, a) + mul(v2, d) + mul(w2, c);


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
        info.dist = blend_ps(info.dist, dist, mask);
        info.normal = blend3_ps(info.normal, normal, mask);
    }
    return mask;
}

__m256 TestSphereTrace(f32x3 rayPos, f32x3 rayDir, SRayHitInfo info, m256x4 sphere)
{
    __m256 mask = set1_ps(0.f);
    f32x3 spherexyz = { sphere.x, sphere.y, sphere.z };
    f32x3 m = rayPos - spherexyz;
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

void TestSceneTrace(f32x3 rayPos, f32x3 rayDir, SRayHitInfo& hitInfo)
{
    // to move the scene around, since we can't move the camera yet
    f32x3 sceneTranslation = { 0.0f, 0.0f, 10.0f };
    m256x4 sceneTranslation4 = set1x4_ps(
        sceneTranslation.x.m256_f32[0],
        sceneTranslation.y.m256_f32[0],
        sceneTranslation.z.m256_f32[0],
        0.0f);

    __m256 mask;

    // back wall
    {
        f32x3 A = set1x3_ps(-12.6f, -12.6f, 25.0f) + sceneTranslation;
        f32x3 B = set1x3_ps(12.6f, -12.6f, 25.0f) + sceneTranslation;
        f32x3 C = set1x3_ps(12.6f, 12.6f, 25.0f) + sceneTranslation;
        f32x3 D = set1x3_ps(-12.6f, 12.6f, 25.0f) + sceneTranslation;
        mask = (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.7f, 0.7f, 0.7f), mask);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
        }
    }

    // floor
    {
        f32x3 A = set1x3_ps(-12.6f, -12.45f, 25.0f) + sceneTranslation;
        f32x3 B = set1x3_ps(12.6f, -12.45f, 25.0f) + sceneTranslation;
        f32x3 C = set1x3_ps(12.6f, -12.45f, 15.0f) + sceneTranslation;
        f32x3 D = set1x3_ps(-12.6f, -12.45f, 15.0f) + sceneTranslation;
        mask = (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.7f, 0.7f, 0.7f), mask);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
        }
    }

    // cieling
    {
        f32x3 A = set1x3_ps(-12.6f, 12.5f, 25.0f) + sceneTranslation;
        f32x3 B = set1x3_ps(12.6f, 12.5f, 25.0f) + sceneTranslation;
        f32x3 C = set1x3_ps(12.6f, 12.5f, 15.0f) + sceneTranslation;
        f32x3 D = set1x3_ps(-12.6f, 12.5f, 15.0f) + sceneTranslation;
        mask = (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.7f, 0.7f, 0.7f), mask);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
        }
    }

    // left wall
    {
        f32x3 A = set1x3_ps(-12.5f, -12.6f, 25.0f) + sceneTranslation;
        f32x3 B = set1x3_ps(-12.5f, -12.6f, 15.0f) + sceneTranslation;
        f32x3 C = set1x3_ps(-12.5f, 12.6f, 15.0f) + sceneTranslation;
        f32x3 D = set1x3_ps(-12.5f, 12.6f, 25.0f) + sceneTranslation;
        mask = (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.7f, 0.1f, 0.1f), mask);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
        }
    }

    // right wall 
    {
        f32x3 A = set1x3_ps(12.5f, -12.6f, 25.0f) + sceneTranslation;
        f32x3 B = set1x3_ps(12.5f, -12.6f, 15.0f) + sceneTranslation;
        f32x3 C = set1x3_ps(12.5f, 12.6f, 15.0f) + sceneTranslation;
        f32x3 D = set1x3_ps(12.5f, 12.6f, 25.0f) + sceneTranslation;
        mask = (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.1f, 0.7f, 0.1f), mask);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
        }
    }

    // light
    {
        f32x3 A = set1x3_ps(-5.0f, 12.4f, 22.5f) + sceneTranslation;
        f32x3 B = set1x3_ps(5.0f, 12.4f, 22.5f) + sceneTranslation;
        f32x3 C = set1x3_ps(5.0f, 12.4f, 17.5f) + sceneTranslation;
        f32x3 D = set1x3_ps(-5.0f, 12.4f, 17.5f) + sceneTranslation;
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
        hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.9f, 0.9f, 0.75f), mask);
        hitInfo.emissive = blend3_ps(hitInfo.albedo, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
    }

    mask = (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps(0.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4));
    {
        hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.9f, 0.75f, 0.9f), mask);
        hitInfo.emissive = blend3_ps(hitInfo.albedo, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
    }

    mask = (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps(9.0f, -9.5f, 20.0f, 3.0f) + sceneTranslation4));
    {
        hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps(0.75f, 0.9f, 0.9f), mask);
        hitInfo.emissive = blend3_ps(hitInfo.albedo, set1x3_ps(0.0f, 0.0f, 0.0f), mask);
    }
}

f32x3 GetColorForRay(f32x3 startRayPos, f32x3 startRayDir, u32& rngState)
{
    f32x3 ret = set1x3_ps(0.0f, 0.0f, 0.0f);
    f32x3 throughput = set1x3_ps(1.0f, 1.0f, 1.0f);
    f32x3 rayPos = startRayPos;
    f32x3 rayDir = startRayDir;

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

f32x3 mainImage(f32x2 fragCoord, f32 Width, f32 Height)
{
    m256x2 iResolution = set1x2_ps(Width, Height);
    __m256 iFrame = set1_ps(0.f);
    u32 rngState = 0; // u32(u32(fragCoord.x) * u32(1973) + u32(fragCoord.y) * u32(9277) + u32(iFrame) * u32(26699)) | u32(1);
    f32x3 rayPosition = set1x3_ps(0.0f, 0.0f, 0.0f);
    __m256 cameraDistance = set1_ps(1.0f / tan(c_FOVDegrees * 0.5f * c_pi / 180.0f));
    f32x3 rayTarget;
    rayTarget.x = (fragCoord.x / iResolution.x) * set1_ps(2.0f) - set1_ps(1.0f);
    rayTarget.y = (fragCoord.y / iResolution.y) * set1_ps(2.0f) - set1_ps(1.0f);
    rayTarget.z = cameraDistance;
    __m256 aspectRatio = iResolution.x / iResolution.y;
    rayTarget.y = rayTarget.y / aspectRatio;
    f32x3 rayDir = normalize(rayTarget - rayPosition);
    f32x3 color = set1x3_ps(0.0f, 0.0f, 0.0f);
    //for (int index = 0; index < c_numRendersPerFrame; ++index)
    //    color = color + GetColorForRay(rayPosition, rayDir, rngState) / set1x3_ps(c_numRendersPerFrame, c_numRendersPerFrame, c_numRendersPerFrame);

    color = GetColorForRay(rayPosition, rayDir, rngState) / set1x3_ps(c_numRendersPerFrame, c_numRendersPerFrame, c_numRendersPerFrame);

    f32x3 lastFrameColor = color; //texture(iChannel0, fragCoord / iResolution).rgb;
    color = lerp(lastFrameColor, color, set1_ps(1.0f) / (iFrame + set1_ps(1.f)));

    return color;
}
#endif

void DemofoxRenderScalar(f32* BufferOut, i32 BufferWidth, i32 BufferHeight, i32 NumChannels)
{
    f32x2 fragCoord;
    f32x2 iResolution;

    iResolution.x = (f32)BufferWidth ;
    iResolution.y = (f32)BufferHeight;

    i32 YHeight = BufferHeight;
    i32 XWidth = BufferWidth;

    f32* BufferPos = BufferOut;

    static f32 iFrame = 0.f;
    iFrame += 1.0f;

    for (i32 Y = 0; Y < YHeight; Y++)
    {
        fragCoord.y = (f32)(YHeight - 1 - Y);
        for (i32 X = 0; X < XWidth; X++)
        {
            fragCoord.x = (f32)X;
            f32x3 color = mainImage(fragCoord, iResolution, iFrame);

            // average the frames together
            f32x3 lastFrameColor = f32x3{ BufferPos[0], BufferPos[1], BufferPos[2] };
                //color; // texture(iChannel0, fragCoord / iResolution.xy).rgb;
            color = lerp(lastFrameColor, color, 1.0f / f32(iFrame + 1.f));

            BufferPos[0] = color.x;
            BufferPos[1] = color.y;
            BufferPos[2] = color.z;
            BufferPos += 3;
        }
    }




}