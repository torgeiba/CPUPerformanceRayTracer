#include "demofox_path_tracing_simd.h"

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
m256x3 RandomUnitVector(u32& state)
{
    __m256 wide_z = set_ps(Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state));
    __m256 wide_a = set_ps(Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state));
    __m256 z = wide_z * set1_ps(2.f) - set1_ps(1.f);
    
    __m256 a = wide_a * set1_ps(c_twopi);
    __m256 r = sroot(set1_ps(1.0f) - z * z);
    __m256 x = r * cos_ps(a);
    __m256 y = r * sin_ps(a);
    return m256x3{ x, y, z };
}

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
    __m256 result = set1_ps(0.f);
    __m256 early_return = set1_ps(0.f);

    // calculate normal and flip vertices order if needed
    m256x3 normal = normalize(cross(c - a, c - b));
    {
        __m256 cond = (dot(normal, rayDir) > set1_ps(0.0f));
        normal = blend3_ps(normal, mul(normal, set1_ps (-1.0f)), cond);

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

        __m256 v_nonnegative_cond = v >= set1_ps(0.0f);
        {
            // test against triangle a,b,c
            __m256 u = -dot(pb, m);
            {
                __m256 cond = bitwise_and(v_nonnegative_cond, u < set1_ps(0.0f));
                result = blend_ps(result, set1_ps(0.f), cond);
                early_return = blend_ps(early_return, bitwise_not(set1_ps(0.f)), cond);
            }
            __m256 w = ScalarTriple(pq, pb, pa);
            {
                __m256 cond = bitwise_and(v_nonnegative_cond, w < set1_ps(0.0f));
                result = blend_ps(result, set1_ps(0.f), cond);
                early_return = blend_ps(early_return, bitwise_not(set1_ps(0.f)), cond);
            }
            __m256 denom = set1_ps(1.0f) / (u + v + w);
            u = u * denom;
            v = blend_ps(v, v * denom, v_nonnegative_cond);
            w = w * denom;
            intersectPos = mul(u, a) + mul(v, b) + mul(w, c);
        }
        {
            m256x3 pd = d - p;
            __m256 u = dot(pd, m);
            {
                __m256 cond = bitwise_and(bitwise_not(v_nonnegative_cond), u < set1_ps(0.0f));
                result = blend_ps(result, set1_ps(0.f), cond);
                early_return = blend_ps(early_return, bitwise_not(set1_ps(0.f)), cond);
            }
            __m256 w = ScalarTriple(pq, pa, pd);
            {
                __m256 cond = bitwise_and(bitwise_not(v_nonnegative_cond), w < set1_ps(0.f));
                result = blend_ps(result, set1_ps(0.f), cond);
                early_return = blend_ps(early_return, bitwise_not(set1_ps(0.f)), cond);

            }
            v = -v;
            __m256 denom = set1_ps(1.0f) / (u + v + w);
            u = u * denom;
            v = v * denom;
            w = w * denom;
            intersectPos = blend3_ps(intersectPos, mul(u, a) + mul(v, d) + mul(w, c), bitwise_not(v_nonnegative_cond));
        }
    }

    __m256 dist;
    {
        __m256 cond1 = abs_ps(rayDir.x) > set1_ps(0.1f);
        __m256 cond2 = abs_ps(rayDir.y) > set1_ps(0.1f);

        dist = (intersectPos.z - rayPos.z) / rayDir.z;
        dist = blend_ps(dist, (intersectPos.y - rayPos.y) / rayDir.y, cond2);
        dist = blend_ps(dist, (intersectPos.x - rayPos.x) / rayDir.x, cond1);

    }

    {
        __m256 cond = bitwise_and(dist > set1_ps(c_minimumRayHitTime), dist < info.dist);
        cond = bitwise_and(bitwise_not(early_return), cond);
        {
            //info.dist = cond ? dist : info.dist;
            info.dist = blend_ps(info.dist, dist, cond);

            //info.normal = cond ? normal : info.normal;
            info.normal = blend3_ps(info.normal, normal, cond);

            //result = cond ? bitwise_not(set1_ps(0.f)) : result;
            result = blend_ps(result, bitwise_not(set1_ps(0.f)), cond);
        }
    }

    return result;
}

static
__m256 TestSphereTrace(m256x3 rayPos, m256x3 rayDir, SRayHitInfo& info, m256x4 sphere)
{
    __m256 result = set1_ps(0.f);
    __m256 early_return = set1_ps(0.f);

    //get the vector from the center of this sphere to where the ray begins.
    m256x3 spherexyz = m256x3{ sphere.x, sphere.y, sphere.z };
    m256x3 m = rayPos - spherexyz;

    //get the dot product of the above vector and the ray's vector
    __m256 b = dot(m, rayDir);
    __m256 c = dot(m, m) - sphere.w * sphere.w;

    //exit if r's origin outside s (c > 0) and r pointing away from s (b > 0)
    {
        __m256 cond = bitwise_and(c > set1_ps(0.0f), b > set1_ps(0.0f));
        result = blend_ps(result, set1_ps(0.f), cond);
        early_return = blend_ps(early_return, bitwise_not(set1_ps(0.f)), cond);
    }

    //calculate discriminant
    __m256 discr = b * b - c;

    //a negative discriminant corresponds to ray missing sphere
    {
        __m256 cond = (discr < set1_ps(0.0f));
        result = blend_ps(result, set1_ps(0.f), cond);
        early_return = blend_ps(early_return, bitwise_not(set1_ps(0.f)), cond);
    }


    //ray now found to intersect sphere, compute smallest t value of intersection
    __m256 fromInside = set1_ps(0.f);
    __m256 dist = -b - sroot(discr);

    {
        fromInside = (dist < set1_ps(0.0f));
        dist = blend_ps(dist, -b + sroot(discr), fromInside);
    }

    {
        __m256 distCheck = bitwise_and(dist > set1_ps(c_minimumRayHitTime), dist < info.dist);
        __m256 check = bitwise_and(bitwise_not(early_return), distCheck);
        info.dist = blend_ps(info.dist, dist, check);
        info.normal = blend3_ps(
            info.normal,
            mul(normalize((rayPos + mul(rayDir, dist)) - spherexyz), blend_ps(set1_ps(1.0f), set1_ps(-1.0f), fromInside)),
            check);
        result = blend_ps(result, bitwise_not(set1_ps(0.f)), check);
    }

    return result;
}

static
void TestSceneTrace(m256x3 rayPos, m256x3 rayDir, SRayHitInfo& hitInfo, __m256 hasTraceTerminated)
{
    // to move the scene around, since we can't move the camera yet
    m256x3 sceneTranslation = set1x3_ps( 0.0f, 0.0f, 10.0f );
    m256x4 sceneTranslation4 = m256x4{ sceneTranslation.x, sceneTranslation.y, sceneTranslation.z, set1_ps(0.0f) };

    // back wall
    {
        m256x3 A = set1x3_ps( -12.6f, -12.6f, 25.0f ) + sceneTranslation;
        m256x3 B = set1x3_ps( 12.6f, -12.6f, 25.0f  ) + sceneTranslation;
        m256x3 C = set1x3_ps( 12.6f, 12.6f, 25.0f   ) + sceneTranslation;
        m256x3 D = set1x3_ps( -12.6f, 12.6f, 25.0f  ) + sceneTranslation;

        __m256 cond = bitwise_and(bitwise_not(hasTraceTerminated), (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
        {
            hitInfo.albedo   = blend3_ps(hitInfo.albedo  , set1x3_ps( 0.7f, 0.7f, 0.7f ), cond);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps( 0.0f, 0.0f, 0.0f ), cond);
        }
    }

    // floor
    {
        m256x3 A = set1x3_ps( -12.6f, -12.45f, 25.0f ) + sceneTranslation;
        m256x3 B = set1x3_ps( 12.6f, -12.45f, 25.0f  ) + sceneTranslation;
        m256x3 C = set1x3_ps( 12.6f, -12.45f, 15.0f  ) + sceneTranslation;
        m256x3 D = set1x3_ps( -12.6f, -12.45f, 15.0f ) + sceneTranslation;

        __m256 cond = bitwise_and(bitwise_not(hasTraceTerminated), (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
        {
         
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps( 0.7f, 0.7f, 0.7f ), cond);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps( 0.0f, 0.0f, 0.0f ), cond);
        }
    }

    // cieling
    {
        m256x3 A = set1x3_ps( -12.6f, 12.5f, 25.0f ) + sceneTranslation;
        m256x3 B = set1x3_ps( 12.6f, 12.5f, 25.0f  ) + sceneTranslation;
        m256x3 C = set1x3_ps( 12.6f, 12.5f, 15.0f  ) + sceneTranslation;
        m256x3 D = set1x3_ps( -12.6f, 12.5f, 15.0f ) + sceneTranslation;

        {
            __m256 cond = bitwise_and(bitwise_not(hasTraceTerminated), (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
  
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps( 0.7f, 0.7f, 0.7f ), cond);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps( 0.0f, 0.0f, 0.0f ), cond);
        }
    }

    // left wall
    {
        m256x3 A = set1x3_ps( -12.5f, -12.6f, 25.0f ) + sceneTranslation;
        m256x3 B = set1x3_ps( -12.5f, -12.6f, 15.0f ) + sceneTranslation;
        m256x3 C = set1x3_ps( -12.5f, 12.6f, 15.0f  ) + sceneTranslation;
        m256x3 D = set1x3_ps( -12.5f, 12.6f, 25.0f  ) + sceneTranslation;

        {
            __m256 cond = bitwise_and(bitwise_not(hasTraceTerminated), (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));
 
            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps( 0.7f, 0.1f, 0.1f ), cond);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps( 0.0f, 0.0f, 0.0f ), cond);
        }
    }

    // right wall 
    {
        m256x3 A = set1x3_ps( 12.5f, -12.6f, 25.0f ) + sceneTranslation;
        m256x3 B = set1x3_ps( 12.5f, -12.6f, 15.0f ) + sceneTranslation;
        m256x3 C = set1x3_ps( 12.5f, 12.6f, 15.0f  ) + sceneTranslation;
        m256x3 D = set1x3_ps( 12.5f, 12.6f, 25.0f  ) + sceneTranslation;
                   
        {
            __m256 cond = bitwise_and(bitwise_not(hasTraceTerminated), (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));

            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps( 0.1f, 0.7f, 0.1f ), cond);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps( 0.0f, 0.0f, 0.0f ), cond);
        }
    }

    // light
    {
        m256x3 A = set1x3_ps( -5.0f, 12.4f, 22.5f ) + sceneTranslation;
        m256x3 B = set1x3_ps( 5.0f, 12.4f, 22.5f  ) + sceneTranslation;
        m256x3 C = set1x3_ps( 5.0f, 12.4f, 17.5f  ) + sceneTranslation;
        m256x3 D = set1x3_ps( -5.0f, 12.4f, 17.5f ) + sceneTranslation;

        {
            __m256 cond = bitwise_and(bitwise_not(hasTraceTerminated), (TestQuadTrace(rayPos, rayDir, hitInfo, A, B, C, D)));

            hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps( 0.0f, 0.0f, 0.0f ), cond);
            hitInfo.emissive = blend3_ps(hitInfo.emissive, mul(set1x3_ps( 1.0f, 0.9f, 0.7f ), set1_ps(20.0f)), cond);
        }
    }

    {
        __m256 cond = bitwise_and(bitwise_not(hasTraceTerminated), (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps( -9.0f, -9.5f, 20.0f, 3.0f ) + sceneTranslation4)));
        hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps( 0.9f, 0.9f, 0.75f ), cond);
        hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps( 0.0f, 0.0f, 0.0f ), cond);
    }

    {
        __m256 cond = bitwise_and(bitwise_not(hasTraceTerminated), (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps( 0.0f, -9.5f, 20.0f, 3.0f ) + sceneTranslation4)));
        hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps( 0.9f, 0.75f, 0.9f ), cond);
        hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps( 0.0f, 0.0f, 0.0f ), cond);
    }

    {
        __m256 cond = bitwise_and(bitwise_not(hasTraceTerminated), (TestSphereTrace(rayPos, rayDir, hitInfo, set1x4_ps( 9.0f, -9.5f, 20.0f, 3.0f ) + sceneTranslation4)));
        hitInfo.albedo = blend3_ps(hitInfo.albedo, set1x3_ps( 0.9f, 0.75f, 0.9f ), cond);
        hitInfo.emissive = blend3_ps(hitInfo.emissive, set1x3_ps( 0.0f, 0.0f, 0.0f ), cond);
    }
}

static
m256x3 GetColorForRay(m256x3 startRayPos, m256x3 startRayDir, u32& rngState)
{
    // initialize
    m256x3 ret = set1x3_ps( 0.0f, 0.0f, 0.0f );
    m256x3 throughput = set1x3_ps( 1.0f, 1.0f, 1.0f );
    m256x3 rayPos = startRayPos;
    m256x3 rayDir = startRayDir;

    __m256 shouldBreak = set1_ps(0.f);
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
            m256x3 ambient = set1x3_ps( .1f, .1f, .1f );
            __m256 cond = bitwise_and(bitwise_not(prevShouldBreak), shouldBreak);
            // if this is the fist time we hit this case, we add the ambient term once
            ret = blend3_ps(ret, ret + ambient, cond);

        }

        // update the ray position
        rayPos = blend3_ps(((rayPos + mul(rayDir, hitInfo.dist)) + mul(hitInfo.normal, set1_ps(c_rayPosNormalNudge))), rayPos, shouldBreak);

        // calculate new ray direction, in a cosine weighted hemisphere oriented at normal
        rayDir = blend3_ps(normalize(hitInfo.normal + RandomUnitVector(rngState)), rayDir, shouldBreak);

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

static m256x3 mainImage(m256x2 fragCoord, m256x2 iResolution, f32 iFrame)
{
    // initialize a random number state based on frag coord and frame
    u32 rngState = u32(u32(fragCoord.x.m256_f32[0]) * u32(1973) + u32(fragCoord.y.m256_f32[0]) * u32(9277) + u32(iFrame) * u32(26699)) | u32(1);

    // The ray starts at the camera position (the origin)
    m256x3 rayPosition = m256x3{ set1_ps(0.0f), set1_ps(0.0f), set1_ps(0.0f )};

    // calculate the camera distance
    f32 cameraDistance = 1.0f / tan(c_FOVDegrees * 0.5f * c_pi / 180.0f);

    // calculate coordinates of the ray target on the imaginary pixel plane.
    // -1 to +1 on x,y axis. 1 unit away on the z axis
    m256x2 rayTargetxy = mul((fragCoord / iResolution), set1_ps(2.0f)) - m256x2{ set1_ps(1.0f), set1_ps(1.0f) };
    m256x3 rayTarget = m256x3{ rayTargetxy.x, rayTargetxy.y, set1_ps(cameraDistance) };

    // correct for aspect ratio
    __m256 aspectRatio = iResolution.x / iResolution.y;
    rayTarget.y = rayTarget.y / aspectRatio;

    // calculate a normalized vector for the ray direction.
    // it's pointing from the ray position to the ray target.
    m256x3 rayDir = normalize(rayTarget - rayPosition);

    // raytrace for this pixel
    m256x3 color = { set1_ps(0.0f), set1_ps(0.0f), set1_ps(0.0f) };
    for (int index = 0; index < c_numRendersPerFrame; ++index)
        color = color + mul(GetColorForRay(rayPosition, rayDir, rngState), set1_ps(1.f / (c_numRendersPerFrame)));

    return color;
}

void DemofoxRenderSimd(f32* BufferOut, i32 BufferWidth, i32 BufferHeight, i32 NumChannels)
{
    m256x2 fragCoord;
    m256x2 iResolution;

    iResolution.x = set1_ps((f32)BufferWidth );
    iResolution.y = set1_ps((f32)BufferHeight);

    i32 YHeight = BufferHeight;
    i32 XWidth = (BufferWidth / LANE_COUNT);

    __m256 XLaneOffsets = set_ps(7.f, 6.f, 5.f, 4.f, 3.f, 2.f, 1.f, 0.f);

    f32* BufferPos = BufferOut;

    static f32 iFrame = 0.f;
    iFrame += 1.0f;

    for (i32 Y = 0; Y < YHeight; Y++)
    {
        fragCoord.y = set1_ps((f32)(YHeight - 1 - Y));
        for (i32 X = 0; X < XWidth; X++)
        {
            fragCoord.x = set1_ps((f32)X * LANE_COUNT) + XLaneOffsets;

            m256x3 color = mainImage(fragCoord, iResolution, iFrame);

            // average the frames together
            m256x3 lastFrameColor = m256x3{ 
                load_ps(&BufferPos[0]),
                load_ps(&BufferPos[1 * LANE_COUNT]),
                load_ps(&BufferPos[2 * LANE_COUNT])
            };
            ////color; // texture(iChannel0, fragCoord / iResolution.xy).rgb;
            color = lerp(lastFrameColor, color, set1_ps(1.0f / f32(iFrame + 1.f)));

            non_temporal_store(BufferPos, color.x);
            BufferPos += LANE_COUNT;
            non_temporal_store(BufferPos, color.y);
            BufferPos += LANE_COUNT;
            non_temporal_store(BufferPos, color.z);
            BufferPos += LANE_COUNT;
        }
    }
}