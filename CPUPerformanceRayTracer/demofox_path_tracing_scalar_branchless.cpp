#include "demofox_path_tracing_scalar_branchless.h"

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

static
u32 wang_hash(u32& seed)
{
    seed = u32(seed ^ u32(61)) ^ u32(seed >> u32(16));
    seed *= u32(9);
    seed = seed ^ (seed >> 4);
    seed *= u32(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

static
f32 Randomf3201(u32& state)
{
    return f32(wang_hash(state)) / 4294967296.0f;
}

static
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

static
f32 ScalarTriple(f32x3 u, f32x3 v, f32x3 w)
{
    return dot(cross(u, v), w);
}

static
bool TestQuadTrace(f32x3 rayPos, f32x3 rayDir, SRayHitInfo& info, f32x3 a, f32x3 b, f32x3 c, f32x3 d)
{
    bool result = false;
    bool early_return = false;

    // calculate normal and flip vertices order if needed
    f32x3 normal = normalize(cross(c - a, c - b));
    {
        bool cond = (dot(normal, rayDir) > 0.0f);
        normal = cond ? mul(normal, -1.0f) : normal;

        // Swap a and d
        f32x3 temp = d;
        d = cond ? a    : d;
        a = cond ? temp : a;

        // swap b and c
        temp = b;
        b = cond ? c    : b;
        c = cond ? temp : c;
    }

    f32x3 p = rayPos;
    f32x3 q = rayPos + rayDir;
    f32x3 pq = q - p;
    f32x3 pa = a - p;
    f32x3 pb = b - p;
    f32x3 pc = c - p;

    // determine which triangle to test against by testing against diagonal first
    f32x3 intersectPos;
    {
        f32x3 m = cross(pc, pq);
        f32 v = dot(pa, m);

        bool v_nonnegative_cond = v >= 0.0f;
        {
            // test against triangle a,b,c
            f32 u = -dot(pb, m);
            {
                bool cond = u < 0.0f;
                result = v_nonnegative_cond && cond ? false : result;
                early_return = v_nonnegative_cond && cond ? true : early_return;
            }
            f32 w = ScalarTriple(pq, pb, pa);
            {
                bool cond = w < 0.0f;
                result = v_nonnegative_cond && cond ? false : result;
                early_return = v_nonnegative_cond && cond ? true : early_return;
            }
            f32 denom = 1.0f / (u + v + w);
            u *= denom;
            v = v_nonnegative_cond ? v * denom : v;
            w *= denom;
            intersectPos = mul(u, a) + mul(v, b) + mul(w, c);
        }
        {
            f32x3 pd = d - p;
            f32 u = dot(pd, m);
            {
                bool cond = u < 0.0f;
                result = !v_nonnegative_cond && cond ? false : result;
                early_return = !v_nonnegative_cond && cond ? true : early_return;
            }
            f32 w = ScalarTriple(pq, pa, pd);
            {
                bool cond = w < 0.0f;
                result = !v_nonnegative_cond && cond ? false : result;
                early_return = !v_nonnegative_cond && cond ? true : early_return;
            }
            v =  -v;
            f32 denom = 1.0f / (u + v + w);
            u *= denom;
            v *= denom;
            w *= denom;
            intersectPos = !v_nonnegative_cond ? mul(u, a) + mul(v, d) + mul(w, c) : intersectPos;
        }
    }

    f32 dist;
    {
        bool cond1 = abs(rayDir.x) > 0.1f;
        bool cond2 = abs(rayDir.y) > 0.1f;
        dist = cond1 ? (intersectPos.x - rayPos.x) / rayDir.x :
            (cond2 ? (intersectPos.y - rayPos.y) / rayDir.y :
                (intersectPos.z - rayPos.z) / rayDir.z);

    }

    {
        bool cond = dist > c_minimumRayHitTime && dist < info.dist;
        {
            info.dist   = !early_return && cond ? dist  : info.dist;
            info.normal = !early_return && cond ? normal: info.normal;
            result = cond && !early_return ? true : result;
        }
    }

    return result;
}

static
bool TestSphereTrace(f32x3 rayPos, f32x3 rayDir, SRayHitInfo& info, f32x4 sphere)
{
    bool result = false;
    bool early_return = false;

    //get the vector from the center of this sphere to where the ray begins.
    f32x3 spherexyz = f32x3{ sphere.x, sphere.y, sphere.z };
    f32x3 m = rayPos - spherexyz;

    //get the dot product of the above vector and the ray's vector
    f32 b = dot(m, rayDir);

    f32 c = dot(m, m) - sphere.w * sphere.w;

    //exit if r's origin outside s (c > 0) and r pointing away from s (b > 0)
    {
        bool cond = c > 0.0 && b > 0.0;
        result = cond ? false : result;
        early_return = cond ? true : early_return;
    }

    //calculate discriminant
    f32 discr = b * b - c;

    //a negative discriminant corresponds to ray missing sphere
    {
        bool cond = (discr < 0.0f);
        result = cond ? false : result;
        early_return = cond ? true : early_return;
    }


    //ray now found to intersect sphere, compute smallest t value of intersection
    bool fromInside = false;
    f32 dist = -b - sqrt(discr);

    {
        fromInside = (dist < 0.0f);
        dist = fromInside ? -b + sqrt(discr) : dist;
    }

    {
        bool distCheck = dist > c_minimumRayHitTime && dist < info.dist;
        bool check = (!early_return && distCheck);
        info.dist   = check ? dist : info.dist;
        info.normal = check ?
            mul(normalize((rayPos + mul(rayDir, dist)) - spherexyz), (fromInside ? -1.0f : 1.0f)) :
            info.normal;
        result = check ? true : result;
    }

    return result;
}

static
void TestSceneTrace(f32x3 rayPos, f32x3 rayDir, SRayHitInfo& hitInfo, bool hasTraceTerminated)
{
    // to move the scene around, since we can't move the camera yet
    f32x3 sceneTranslation = f32x3{ 0.0f, 0.0f, 10.0f };
    f32x4 sceneTranslation4 = f32x4{ sceneTranslation.x, sceneTranslation.y, sceneTranslation.z, 0.0f };

    // back wall
    {
        f32x3 A = f32x3{ -12.6f, -12.6f, 25.0f } + sceneTranslation;
        f32x3 B = f32x3{ 12.6f, -12.6f, 25.0f } + sceneTranslation;
        f32x3 C = f32x3{ 12.6f, 12.6f, 25.0f } + sceneTranslation;
        f32x3 D = f32x3{ -12.6f, 12.6f, 25.0f } + sceneTranslation;

        bool cond = !hasTraceTerminated && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo   = cond ? f32x3{ 0.7f, 0.7f, 0.7f } : hitInfo.albedo;
            hitInfo.emissive = cond ? f32x3{ 0.0f, 0.0f, 0.0f } : hitInfo.emissive;
        }
    }

    // floor
    {
        f32x3 A = f32x3{ -12.6f, -12.45f, 25.0f } + sceneTranslation;
        f32x3 B = f32x3{ 12.6f, -12.45f, 25.0f } + sceneTranslation;
        f32x3 C = f32x3{ 12.6f, -12.45f, 15.0f } + sceneTranslation;
        f32x3 D = f32x3{ -12.6f, -12.45f, 15.0f } + sceneTranslation;

        bool cond = !hasTraceTerminated && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo   = cond ? f32x3{ 0.7f, 0.7f, 0.7f } : hitInfo.albedo;
            hitInfo.emissive = cond ? f32x3{ 0.0f, 0.0f, 0.0f } : hitInfo.emissive;
        }
    }

    // cieling
    {
        f32x3 A = f32x3{ -12.6f, 12.5f, 25.0f } + sceneTranslation;
        f32x3 B = f32x3{ 12.6f, 12.5f, 25.0f } + sceneTranslation;
        f32x3 C = f32x3{ 12.6f, 12.5f, 15.0f } + sceneTranslation;
        f32x3 D = f32x3{ -12.6f, 12.5f, 15.0f } + sceneTranslation;

        bool cond = !hasTraceTerminated && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
        {
            hitInfo.albedo   = cond ? f32x3{ 0.7f, 0.7f, 0.7f } : hitInfo.albedo;
            hitInfo.emissive = cond ? f32x3{ 0.0f, 0.0f, 0.0f } : hitInfo.emissive;
        }
    }

    // left wall
    {
        f32x3 A = f32x3{ -12.5f, -12.6f, 25.0f } + sceneTranslation;
        f32x3 B = f32x3{ -12.5f, -12.6f, 15.0f } + sceneTranslation;
        f32x3 C = f32x3{ -12.5f, 12.6f, 15.0f } + sceneTranslation;
        f32x3 D = f32x3{ -12.5f, 12.6f, 25.0f } + sceneTranslation;

        {
            bool cond = !hasTraceTerminated && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
            hitInfo.albedo   = cond ? f32x3{ 0.7f, 0.1f, 0.1f } : hitInfo.albedo;
            hitInfo.emissive = cond ? f32x3{ 0.0f, 0.0f, 0.0f } : hitInfo.emissive;
        }
    }

    // right wall 
    {
        f32x3 A = f32x3{ 12.5f, -12.6f, 25.0f } + sceneTranslation;
        f32x3 B = f32x3{ 12.5f, -12.6f, 15.0f } + sceneTranslation;
        f32x3 C = f32x3{ 12.5f, 12.6f, 15.0f } + sceneTranslation;
        f32x3 D = f32x3{ 12.5f, 12.6f, 25.0f } + sceneTranslation;

        {
            bool cond = !hasTraceTerminated && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
            hitInfo.albedo   = cond ? f32x3{ 0.1f, 0.7f, 0.1f } : hitInfo.albedo;
            hitInfo.emissive = cond ? f32x3{ 0.0f, 0.0f, 0.0f } : hitInfo.emissive;
        }
    }

    // light
    {
        f32x3 A = f32x3{ -5.0f, 12.4f, 22.5f } + sceneTranslation;
        f32x3 B = f32x3{ 5.0f, 12.4f, 22.5f } + sceneTranslation;
        f32x3 C = f32x3{ 5.0f, 12.4f, 17.5f } + sceneTranslation;
        f32x3 D = f32x3{ -5.0f, 12.4f, 17.5f } + sceneTranslation;
        
        {
            bool cond = !hasTraceTerminated && (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D));
            hitInfo.albedo   = cond ? f32x3{ 0.0f, 0.0f, 0.0f }             : hitInfo.albedo;
            hitInfo.emissive = cond ? mul(f32x3{ 1.0f, 0.9f, 0.7f }, 20.0f) : hitInfo.emissive;
        }
    }

    {
        bool cond = !hasTraceTerminated && (TestSphereTrace(rayPos, rayDir, hitInfo, f32x4{ -9.0f, -9.5f, 20.0f, 3.0f } + sceneTranslation4));
        hitInfo.albedo   = cond ? f32x3{ 0.9f, 0.9f, 0.75f } : hitInfo.albedo;
        hitInfo.emissive = cond ? f32x3{ 0.0f, 0.0f, 0.0f  } : hitInfo.emissive;
    }

    {
        bool cond = !hasTraceTerminated && (TestSphereTrace(rayPos, rayDir, hitInfo, f32x4{ 0.0f, -9.5f, 20.0f, 3.0f } + sceneTranslation4));
        hitInfo.albedo   = cond ? f32x3{ 0.9f, 0.75f, 0.9f } : hitInfo.albedo;
        hitInfo.emissive = cond ? f32x3{ 0.0f, 0.0f, 0.0f  } : hitInfo.emissive;
    }

    {
        bool cond = !hasTraceTerminated && (TestSphereTrace(rayPos, rayDir, hitInfo, f32x4{ 9.0f, -9.5f, 20.0f, 3.0f } + sceneTranslation4));
        hitInfo.albedo   = cond ? f32x3{ 0.75f, 0.9f, 0.9f }: hitInfo.albedo;
        hitInfo.emissive = cond ? f32x3{ 0.0f, 0.0f, 0.0f } : hitInfo.emissive;
    }
}

static
f32x3 GetColorForRay(f32x3 startRayPos, f32x3 startRayDir, u32& rngState)
{
    // initialize
    f32x3 ret = f32x3{ 0.0f, 0.0f, 0.0f };
    f32x3 throughput = f32x3{ 1.0f, 1.0f, 1.0f };
    f32x3 rayPos = startRayPos;
    f32x3 rayDir = startRayDir;

    bool shouldBreak = false;
    for (int bounceIndex = 0; (bounceIndex <= c_numBounces); ++bounceIndex)
    {
        // shoot a ray out into the world
        SRayHitInfo hitInfo;
        hitInfo.dist = shouldBreak ? hitInfo.dist : c_superFar;
        TestSceneTrace(rayPos, rayDir, hitInfo, shouldBreak);

        // if the ray missed, we are done
        bool prevShouldBreak = shouldBreak;
        shouldBreak = (hitInfo.dist == c_superFar);
        {
            f32x3 ambient = f32x3{ .1f, .1f, .1f };
            ret = (!prevShouldBreak && shouldBreak) ? // if this is the fist time we hit this case, we add the ambient term once
                (ret + ambient)
                : ret; // texture(iChannel1, rayDir).rgb* throughput;
            //break;
        }

        // update the ray position
        rayPos = shouldBreak ? rayPos :  
            ((rayPos + mul(rayDir, hitInfo.dist)) + mul(hitInfo.normal, c_rayPosNormalNudge));

        // calculate new ray direction, in a cosine weighted hemisphere oriented at normal
        rayDir = shouldBreak ? rayDir : 
            normalize(hitInfo.normal + RandomUnitVector(rngState));

        // add in emissive lighting
        ret = shouldBreak ? ret : 
            (ret + hitInfo.emissive * throughput);

        // update the colorMultiplier
        throughput = shouldBreak ? throughput : 
            (throughput * hitInfo.albedo);
    }

    // return pixel color
    return ret;
}

static f32x3 mainImage(f32x2 fragCoord, f32x2 iResolution, f32 iFrame)
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
        color = color + mul(GetColorForRay(rayPosition, rayDir, rngState), (1.f / (c_numRendersPerFrame)));

    // show the result
    return color;
}

void DemofoxRenderScalarBranchless(f32* BufferOut, i32 BufferWidth, i32 BufferHeight, i32 NumChannels)
{
    f32x2 fragCoord;
    f32x2 iResolution;

    iResolution.x = (f32)BufferWidth;
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