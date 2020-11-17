#pragma once

#include "mathlib.h"

static const f32 c_pi = 3.14159265359f;
static const f32 c_twopi = 2.0f * c_pi;

inline __m256i wang_hash_ps(__m256i& seeds)
{
    seeds = (seeds ^ 61) ^ (seeds >> 16);
    seeds = seeds * 9;
    seeds = seeds ^ (seeds >> 4);
    seeds = seeds * 0x27d4eb2d;
    seeds = seeds ^ (seeds >> 15);
    return seeds;
}

inline __m256 Randomf3201_ps(__m256i& state)
{
    // _mm256_cvtepi32_ps does not work here, since it converts from signed integer
    //return _mm256_cvtepi32_ps(wang_hash_ps(state)) / 4294967296.0f;
    //return _mm_cvtepu32_ps(wang_hash_ps(state)) / 4294967296.0f;

    // faster but possibly fewer states version, the point is to not have 1 integer codepoint of bias due to there being more negative numbers than positive in signed integer
    return to_ps(0x7FFFFFFF & wang_hash_ps(state)) / 2147483648.0f;
}

inline __m256 SignedRandomf3201_ps(__m256i& state)
{
    return to_ps(wang_hash_ps(state)) / 2147483648.0f;
}

inline m256x3 RandomUnitVector_ps(__m256i& state)
{
    __m256 wide_z = Randomf3201_ps(state);
    __m256 wide_a = Randomf3201_ps(state);
    __m256 z = wide_z * 2.f - 1.f;

    __m256 a = wide_a * set1_ps(c_twopi);
    __m256 r = sroot(1.f - z * z);

    __m256 cos_a;
    __m256 sin_a = sincos_ps(&cos_a, a);
    __m256 x = r * cos_a; //cos_ps(a);
    __m256 y = r * sin_a; //sin_ps(a);
    return m256x3{ x, y, z };
}