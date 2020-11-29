#pragma once

#include "utils.h"

#include <cmath>

#include <emmintrin.h>
#include <immintrin.h>

#define LANE_COUNT 8

#define USE_RCP_NORMALIZE 1

struct f32x2 { f32 x, y; };
struct f32x3 { f32 x, y, z; };
struct f32x4 { f32 x, y, z, w; };

struct i32x2 { i32 x, y; };
struct i32x3 { i32 x, y, z; };
struct i32x4 { i32 x, y, z, w; };

inline f32x2 add(f32x2 u, f32x2 v) { return { u.x + v.x , u.y + v.y }; }
inline f32x3 add(f32x3 u, f32x3 v) { return { u.x + v.x , u.y + v.y , u.z + v.z }; }
inline f32x4 add(f32x4 u, f32x4 v) { return { u.x + v.x , u.y + v.y , u.z + v.z , u.w + v.w }; }
inline f32x2 sub(f32x2 u, f32x2 v) { return { u.x - v.x , u.y - v.y }; }
inline f32x3 sub(f32x3 u, f32x3 v) { return { u.x - v.x , u.y - v.y , u.z - v.z }; }
inline f32x4 sub(f32x4 u, f32x4 v) { return { u.x - v.x , u.y - v.y , u.z - v.z , u.w - v.w }; }
inline f32x2 mul(f32x2 u, f32x2 v) { return { u.x * v.x , u.y * v.y }; }
inline f32x3 mul(f32x3 u, f32x3 v) { return { u.x * v.x , u.y * v.y , u.z * v.z }; }
inline f32x4 mul(f32x4 u, f32x4 v) { return { u.x * v.x , u.y * v.y , u.z * v.z , u.w * v.w }; }
inline f32x2 div(f32x2 u, f32x2 v) { return { u.x / v.x , u.y / v.y }; }
inline f32x3 div(f32x3 u, f32x3 v) { return { u.x / v.x , u.y / v.y , u.z / v.z }; }
inline f32x4 div(f32x4 u, f32x4 v) { return { u.x / v.x , u.y / v.y , u.z / v.z , u.w / v.w }; }

inline f32x2 add(f32x2 u, f32 c) { return { u.x + c, u.y + c }; }
inline f32x3 add(f32x3 u, f32 c) { return { u.x + c, u.y + c, u.z + c }; }
inline f32x4 add(f32x4 u, f32 c) { return { u.x + c, u.y + c, u.z + c, u.w + c }; }
inline f32x2 add(f32 c, f32x2 u) { return { c + u.x, c + u.y };}
inline f32x3 add(f32 c, f32x3 u) { return { c + u.x, c + u.y, c + u.z }; }
inline f32x4 add(f32 c, f32x4 u) { return { c + u.x, c + u.y, c + u.z, c + u.w }; }

inline f32x2 sub(f32x2 u, f32 c) { return { u.x - c, u.y - c }; }
inline f32x3 sub(f32x3 u, f32 c) { return { u.x - c, u.y - c, u.z - c }; }
inline f32x4 sub(f32x4 u, f32 c) { return { u.x - c, u.y - c, u.z - c, u.w - c }; }
inline f32x2 sub(f32 c, f32x2 u) { return { c - u.x, c - u.y }; }
inline f32x3 sub(f32 c, f32x3 u) { return { c - u.x, c - u.y, c - u.z }; }
inline f32x4 sub(f32 c, f32x4 u) { return { c - u.x, c - u.y, c - u.z, c - u.w }; }

inline f32x2 mul(f32x2 u, f32 c)   { return { u.x * c, u.y * c}; }
inline f32x3 mul(f32x3 u, f32 c)   { return { u.x * c, u.y * c, u.z * c}; }
inline f32x4 mul(f32x4 u, f32 c)   { return { u.x * c, u.y * c, u.z * c, u.w * c}; }
inline f32x2 mul(f32 c, f32x2 u)   { return { u.x * c, u.y * c }; }
inline f32x3 mul(f32 c, f32x3 u)   { return { u.x * c, u.y * c, u.z * c }; }
inline f32x4 mul(f32 c, f32x4 u)   { return { u.x * c, u.y * c, u.z * c, u.w * c }; }

inline f32x2 div(f32x2 u, f32 c) { return { u.x / c, u.y / c }; }
inline f32x3 div(f32x3 u, f32 c) { return { u.x / c, u.y / c, u.z / c }; }
inline f32x4 div(f32x4 u, f32 c) { return { u.x / c, u.y / c, u.z / c, u.w / c }; }
inline f32x2 div(f32 c, f32x2 u) { return { c / u.x, c / u.y }; }
inline f32x3 div(f32 c, f32x3 u) { return { c / u.x, c / u.y, c / u.z }; }
inline f32x4 div(f32 c, f32x4 u) { return { c / u.x, c / u.y, c / u.z, c / u.w }; }

inline f32 dot(f32x2 u, f32x2 v)   { return u.x * v.x + u.y * v.y; }
inline f32 dot(f32x3 u, f32x3 v)   { return u.x * v.x + u.y * v.y + u.z * v.z; }
inline f32 dot(f32x4 u, f32x4 v)   { return u.x * v.x + u.y * v.y + u.z * v.z + u.w * v.w; }

inline f32x2 fmadd(f32x2 a, f32x2 b, f32x2 c) { return { fma(a.x, b.x, c.x), fma(a.y, b.y, c.y)}; }
inline f32x3 fmadd(f32x3 a, f32x3 b, f32x3 c) { return { fma(a.x, b.x, c.x), fma(a.y, b.y, c.y), fma(a.z, b.z, c.z)}; }
inline f32x4 fmadd(f32x4 a, f32x4 b, f32x4 c) { return { fma(a.x, b.x, c.x), fma(a.y, b.y, c.y), fma(a.z, b.z, c.z), fma(a.w, b.w, c.w) }; }

inline f32 fmadot(f32x2 u, f32x2 v) { return fma(u.x, v.x, u.y * v.y); }
inline f32 fmadot(f32x3 u, f32x3 v) { return fma(u.x, v.x, fma(u.y, v.y, u.z * v.z)); }
inline f32 fmadot(f32x4 u, f32x4 v) { return fma(u.x, v.x, fma(u.y, v.y, fma(u.z, v.z, u.w * v.w))); }
//inline f32 fmadot2(f32x4 u, f32x4 v) { return fma(u.x, v.x, u.y * v.y) + fma(u.z, v.z, u.w * v.w); }


/*

	SIMD versions

*/

// See also https://github.com/vectorclass/version2/blob/master/vectorf256.h
// search for Vec8f type
// For _vectorcall: http://software.intel.com/en-us/articles/vectorcall-and-regcall-demystified

typedef __m256 m256;
typedef __m256 Mask;

struct m256x2 { __m256 x, y;       };
struct m256x3 { __m256 x, y, z;    };
struct m256x4 { __m256 x, y, z, w; };

inline __m256 add(__m256 u, __m256 v) { return _mm256_add_ps(u, v); }
inline m256x2 add(m256x2 u, m256x2 v) { return { _mm256_add_ps(u.x, v.x), _mm256_add_ps(u.y, v.y) }; }
inline m256x3 add(m256x3 u, m256x3 v) { return { _mm256_add_ps(u.x, v.x), _mm256_add_ps(u.y, v.y), _mm256_add_ps(u.z, v.z) }; }
inline m256x4 add(m256x4 u, m256x4 v) { return { _mm256_add_ps(u.x, v.x), _mm256_add_ps(u.y, v.y), _mm256_add_ps(u.z, v.z), _mm256_add_ps(u.w, v.w) }; }

inline __m256 sub(__m256 u, __m256 v) { return _mm256_sub_ps(u, v); }
inline m256x2 sub(m256x2 u, m256x2 v) { return{ _mm256_sub_ps(u.x, v.x), _mm256_sub_ps(u.y, v.y) }; }
inline m256x3 sub(m256x3 u, m256x3 v) { return{ _mm256_sub_ps(u.x, v.x), _mm256_sub_ps(u.y, v.y), _mm256_sub_ps(u.z, v.z) }; }
inline m256x4 sub(m256x4 u, m256x4 v) { return{ _mm256_sub_ps(u.x, v.x), _mm256_sub_ps(u.y, v.y), _mm256_sub_ps(u.z, v.z), _mm256_sub_ps(u.w, v.w) }; }

inline __m256 mul(__m256 u, __m256 v) { return _mm256_mul_ps(u, v); }
inline m256x2 mul(m256x2 u, m256x2 v) { return{ _mm256_mul_ps(u.x, v.x), _mm256_mul_ps(u.y, v.y) }; }
inline m256x3 mul(m256x3 u, m256x3 v) { return{ _mm256_mul_ps(u.x, v.x), _mm256_mul_ps(u.y, v.y), _mm256_mul_ps(u.z, v.z) }; }
inline m256x4 mul(m256x4 u, m256x4 v) { return{ _mm256_mul_ps(u.x, v.x), _mm256_mul_ps(u.y, v.y), _mm256_mul_ps(u.z, v.z), _mm256_mul_ps(u.w, v.w) }; }

inline __m256 div(__m256 u, __m256 v) { return _mm256_div_ps(u, v); }
inline m256x2 div(m256x2 u, m256x2 v) { return{ _mm256_div_ps(u.x, v.x), _mm256_div_ps(u.y, v.y) }; }
inline m256x3 div(m256x3 u, m256x3 v) { return{ _mm256_div_ps(u.x, v.x), _mm256_div_ps(u.y, v.y), _mm256_div_ps(u.z, v.z) }; }
inline m256x4 div(m256x4 u, m256x4 v) { return{ _mm256_div_ps(u.x, v.x), _mm256_div_ps(u.y, v.y), _mm256_div_ps(u.z, v.z), _mm256_div_ps(u.w, v.w) }; }

inline m256x2 add(m256x2 v, __m256 c) { return{ _mm256_add_ps(v.x, c), _mm256_add_ps(v.y, c) }; }
inline m256x3 add(m256x3 v, __m256 c) { return{ _mm256_add_ps(v.x, c), _mm256_add_ps(v.y, c), _mm256_add_ps(v.z, c) }; }
inline m256x4 add(m256x4 v, __m256 c) { return{ _mm256_add_ps(v.x, c), _mm256_add_ps(v.y, c), _mm256_add_ps(v.z, c), _mm256_add_ps(v.w, c) }; }

inline m256x2 add(__m256 c, m256x2 v) { return{ _mm256_add_ps(v.x, c), _mm256_add_ps(v.y, c) }; }
inline m256x3 add(__m256 c, m256x3 v) { return{ _mm256_add_ps(v.x, c), _mm256_add_ps(v.y, c), _mm256_add_ps(v.z, c) }; }
inline m256x4 add(__m256 c, m256x4 v) { return{ _mm256_add_ps(v.x, c), _mm256_add_ps(v.y, c), _mm256_add_ps(v.z, c), _mm256_add_ps(v.w, c) }; }

inline m256x2 sub(m256x2 v, __m256 c) { return{ _mm256_sub_ps(v.x, c), _mm256_sub_ps(v.y, c) }; }
inline m256x3 sub(m256x3 v, __m256 c) { return{ _mm256_sub_ps(v.x, c), _mm256_sub_ps(v.y, c), _mm256_sub_ps(v.z, c) }; }
inline m256x4 sub(m256x4 v, __m256 c) { return{ _mm256_sub_ps(v.x, c), _mm256_sub_ps(v.y, c), _mm256_sub_ps(v.z, c), _mm256_sub_ps(v.w, c) }; }

inline m256x2 sub(__m256 c, m256x2 v) { return{ _mm256_sub_ps(c, v.x), _mm256_sub_ps(c, v.y) }; }
inline m256x3 sub(__m256 c, m256x3 v) { return{ _mm256_sub_ps(c, v.x), _mm256_sub_ps(c, v.y), _mm256_sub_ps(c, v.z) }; }
inline m256x4 sub(__m256 c, m256x4 v) { return{ _mm256_sub_ps(c, v.x), _mm256_sub_ps(c, v.y), _mm256_sub_ps(c, v.z), _mm256_sub_ps(c, v.w) }; }

inline m256x2 mul(m256x2 v, __m256 c) { return{ _mm256_mul_ps(v.x, c), _mm256_mul_ps(v.y, c) }; }
inline m256x3 mul(m256x3 v, __m256 c) { return{ _mm256_mul_ps(v.x, c), _mm256_mul_ps(v.y, c), _mm256_mul_ps(v.z, c) }; }
inline m256x4 mul(m256x4 v, __m256 c) { return{ _mm256_mul_ps(v.x, c), _mm256_mul_ps(v.y, c), _mm256_mul_ps(v.z, c), _mm256_mul_ps(v.w, c) }; }

inline m256x2 mul(__m256 c, m256x2 v) { return{ _mm256_mul_ps(v.x, c), _mm256_mul_ps(v.y, c) }; }
inline m256x3 mul(__m256 c, m256x3 v) { return{ _mm256_mul_ps(v.x, c), _mm256_mul_ps(v.y, c), _mm256_mul_ps(v.z, c) }; }
inline m256x4 mul(__m256 c, m256x4 v) { return{ _mm256_mul_ps(v.x, c), _mm256_mul_ps(v.y, c), _mm256_mul_ps(v.z, c), _mm256_mul_ps(v.w, c) }; }

inline m256x2 div(m256x2 v, __m256 c) { return{ _mm256_div_ps(v.x, c), _mm256_div_ps(v.y, c) }; }
inline m256x3 div(m256x3 v, __m256 c) { return{ _mm256_div_ps(v.x, c), _mm256_div_ps(v.y, c), _mm256_div_ps(v.z, c) }; }
inline m256x4 div(m256x4 v, __m256 c) { return{ _mm256_div_ps(v.x, c), _mm256_div_ps(v.y, c), _mm256_div_ps(v.z, c), _mm256_div_ps(v.w, c) }; }

inline m256x2 div(__m256 c, m256x2 v) { return{ _mm256_div_ps(c, v.x), _mm256_div_ps(c, v.y) };}
inline m256x3 div(__m256 c, m256x3 v) { return{ _mm256_div_ps(c, v.x), _mm256_div_ps(c, v.y), _mm256_div_ps(c, v.z) }; }
inline m256x4 div(__m256 c, m256x4 v) { return{ _mm256_div_ps(c, v.x), _mm256_div_ps(c, v.y), _mm256_div_ps(c, v.z), _mm256_div_ps(c, v.w) }; }

inline __m256 dot(m256x2 u, m256x2 v) { return _mm256_fmadd_ps(u.x, v.x,                                                     _mm256_mul_ps(u.y, v.y)); }
inline __m256 dot(m256x3 u, m256x3 v) { return _mm256_fmadd_ps(u.x, v.x, _mm256_fmadd_ps(u.y, v.y,                           _mm256_mul_ps(u.z, v.z))); }
inline __m256 dot(m256x4 u, m256x4 v) { return _mm256_fmadd_ps(u.x, v.x, _mm256_fmadd_ps(u.y, v.y, _mm256_fmadd_ps(u.z, v.z, _mm256_mul_ps(u.w, v.w)))); }

// a * b + c
inline __m256 fmadd(__m256 a, __m256 b, __m256 c) { return  _mm256_fmadd_ps(a, b, c); }

inline m256x2 fmadd(m256x2 a, m256x2 b, m256x2 c) { return{ _mm256_fmadd_ps(a.x, b.x, c.x), _mm256_fmadd_ps(a.y, b.y, c.y) }; }
inline m256x3 fmadd(m256x3 a, m256x3 b, m256x3 c) { return{ _mm256_fmadd_ps(a.x, b.x, c.x), _mm256_fmadd_ps(a.y, b.y, c.y), _mm256_fmadd_ps(a.z, b.z, c.z) }; }
inline m256x4 fmadd(m256x4 a, m256x4 b, m256x4 c) { return{ _mm256_fmadd_ps(a.x, b.x, c.x), _mm256_fmadd_ps(a.y, b.y, c.y), _mm256_fmadd_ps(a.z, b.z, c.z), _mm256_fmadd_ps(a.w, b.w, c.w) }; }

inline m256x2 fmadd(__m256 a, m256x2 b, m256x2 c) { return{ _mm256_fmadd_ps(a, b.x, c.x), _mm256_fmadd_ps(a, b.y, c.y) }; }
inline m256x3 fmadd(__m256 a, m256x3 b, m256x3 c) { return{ _mm256_fmadd_ps(a, b.x, c.x), _mm256_fmadd_ps(a, b.y, c.y), _mm256_fmadd_ps(a, b.z, c.z) }; }
inline m256x4 fmadd(__m256 a, m256x4 b, m256x4 c) { return{ _mm256_fmadd_ps(a, b.x, c.x), _mm256_fmadd_ps(a, b.y, c.y), _mm256_fmadd_ps(a, b.z, c.z), _mm256_fmadd_ps(a, b.w, c.w) }; }

inline m256x2 fmadd(m256x2 a, __m256 b, m256x2 c) { return{ _mm256_fmadd_ps(a.x, b, c.x), _mm256_fmadd_ps(a.y, b, c.y) }; }
inline m256x3 fmadd(m256x3 a, __m256 b, m256x3 c) { return{ _mm256_fmadd_ps(a.x, b, c.x), _mm256_fmadd_ps(a.y, b, c.y), _mm256_fmadd_ps(a.z, b, c.z) }; }
inline m256x4 fmadd(m256x4 a, __m256 b, m256x4 c) { return{ _mm256_fmadd_ps(a.x, b, c.x), _mm256_fmadd_ps(a.y, b, c.y), _mm256_fmadd_ps(a.z, b, c.z), _mm256_fmadd_ps(a.w, b, c.w) }; }

inline m256x2 fmadd(m256x2 a, m256x2 b, __m256 c) { return{ _mm256_fmadd_ps(a.x, b.x, c), _mm256_fmadd_ps(a.y, b.y, c) }; }
inline m256x3 fmadd(m256x3 a, m256x3 b, __m256 c) { return{ _mm256_fmadd_ps(a.x, b.x, c), _mm256_fmadd_ps(a.y, b.y, c), _mm256_fmadd_ps(a.z, b.z, c) }; }
inline m256x4 fmadd(m256x4 a, m256x4 b, __m256 c) { return{ _mm256_fmadd_ps(a.x, b.x, c), _mm256_fmadd_ps(a.y, b.y, c), _mm256_fmadd_ps(a.z, b.z, c), _mm256_fmadd_ps(a.w, b.w, c) }; }

inline m256x2 fmadd(__m256 a, __m256 b, m256x2 c) { return{ _mm256_fmadd_ps(a, b, c.x), _mm256_fmadd_ps(a, b, c.y) }; }
inline m256x3 fmadd(__m256 a, __m256 b, m256x3 c) { return{ _mm256_fmadd_ps(a, b, c.x), _mm256_fmadd_ps(a, b, c.y), _mm256_fmadd_ps(a, b, c.z) }; }
inline m256x4 fmadd(__m256 a, __m256 b, m256x4 c) { return{ _mm256_fmadd_ps(a, b, c.x), _mm256_fmadd_ps(a, b, c.y), _mm256_fmadd_ps(a, b, c.z), _mm256_fmadd_ps(a, b, c.w) }; }

inline m256x2 fmadd(m256x2 a, __m256 b, __m256 c) { return{ _mm256_fmadd_ps(a.x, b, c), _mm256_fmadd_ps(a.y, b, c) }; }
inline m256x3 fmadd(m256x3 a, __m256 b, __m256 c) { return{ _mm256_fmadd_ps(a.x, b, c), _mm256_fmadd_ps(a.y, b, c), _mm256_fmadd_ps(a.z, b, c) }; }
inline m256x4 fmadd(m256x4 a, __m256 b, __m256 c) { return{ _mm256_fmadd_ps(a.x, b, c), _mm256_fmadd_ps(a.y, b, c), _mm256_fmadd_ps(a.z, b, c), _mm256_fmadd_ps(a.w, b, c) }; }

inline m256x2 fmadd(__m256 a, m256x2 b, __m256 c) { return{ _mm256_fmadd_ps(a, b.x, c), _mm256_fmadd_ps(a, b.y, c) }; }
inline m256x3 fmadd(__m256 a, m256x3 b, __m256 c) { return{ _mm256_fmadd_ps(a, b.x, c), _mm256_fmadd_ps(a, b.y, c), _mm256_fmadd_ps(a, b.z, c) }; }
inline m256x4 fmadd(__m256 a, m256x4 b, __m256 c) { return{ _mm256_fmadd_ps(a, b.x, c), _mm256_fmadd_ps(a, b.y, c), _mm256_fmadd_ps(a, b.z, c), _mm256_fmadd_ps(a, b.w, c) }; }


// -(a * b) + c
inline __m256 fnmadd(__m256 a, __m256 b, __m256 c) { return  _mm256_fnmadd_ps(a, b, c); }

inline m256x2 fnmadd(m256x2 a, m256x2 b, m256x2 c) { return{ _mm256_fnmadd_ps(a.x, b.x, c.x), _mm256_fnmadd_ps(a.y, b.y, c.y) }; }
inline m256x3 fnmadd(m256x3 a, m256x3 b, m256x3 c) { return{ _mm256_fnmadd_ps(a.x, b.x, c.x), _mm256_fnmadd_ps(a.y, b.y, c.y), _mm256_fnmadd_ps(a.z, b.z, c.z) }; }
inline m256x4 fnmadd(m256x4 a, m256x4 b, m256x4 c) { return{ _mm256_fnmadd_ps(a.x, b.x, c.x), _mm256_fnmadd_ps(a.y, b.y, c.y), _mm256_fnmadd_ps(a.z, b.z, c.z), _mm256_fnmadd_ps(a.w, b.w, c.w) }; }

inline m256x2 fnmadd(__m256 a, m256x2 b, m256x2 c) { return{ _mm256_fnmadd_ps(a, b.x, c.x), _mm256_fnmadd_ps(a, b.y, c.y) }; }
inline m256x3 fnmadd(__m256 a, m256x3 b, m256x3 c) { return{ _mm256_fnmadd_ps(a, b.x, c.x), _mm256_fnmadd_ps(a, b.y, c.y), _mm256_fnmadd_ps(a, b.z, c.z) }; }
inline m256x4 fnmadd(__m256 a, m256x4 b, m256x4 c) { return{ _mm256_fnmadd_ps(a, b.x, c.x), _mm256_fnmadd_ps(a, b.y, c.y), _mm256_fnmadd_ps(a, b.z, c.z), _mm256_fnmadd_ps(a, b.w, c.w) }; }

inline m256x2 fnmadd(m256x2 a, __m256 b, m256x2 c) { return{ _mm256_fnmadd_ps(a.x, b, c.x), _mm256_fnmadd_ps(a.y, b, c.y) }; }
inline m256x3 fnmadd(m256x3 a, __m256 b, m256x3 c) { return{ _mm256_fnmadd_ps(a.x, b, c.x), _mm256_fnmadd_ps(a.y, b, c.y), _mm256_fnmadd_ps(a.z, b, c.z) }; }
inline m256x4 fnmadd(m256x4 a, __m256 b, m256x4 c) { return{ _mm256_fnmadd_ps(a.x, b, c.x), _mm256_fnmadd_ps(a.y, b, c.y), _mm256_fnmadd_ps(a.z, b, c.z), _mm256_fnmadd_ps(a.w, b, c.w) }; }

inline m256x2 fnmadd(m256x2 a, m256x2 b, __m256 c) { return{ _mm256_fnmadd_ps(a.x, b.x, c), _mm256_fnmadd_ps(a.y, b.y, c) }; }
inline m256x3 fnmadd(m256x3 a, m256x3 b, __m256 c) { return{ _mm256_fnmadd_ps(a.x, b.x, c), _mm256_fnmadd_ps(a.y, b.y, c), _mm256_fnmadd_ps(a.z, b.z, c) }; }
inline m256x4 fnmadd(m256x4 a, m256x4 b, __m256 c) { return{ _mm256_fnmadd_ps(a.x, b.x, c), _mm256_fnmadd_ps(a.y, b.y, c), _mm256_fnmadd_ps(a.z, b.z, c), _mm256_fnmadd_ps(a.w, b.w, c) }; }

inline m256x2 fnmadd(__m256 a, __m256 b, m256x2 c) { return{ _mm256_fnmadd_ps(a, b, c.x), _mm256_fnmadd_ps(a, b, c.y) }; }
inline m256x3 fnmadd(__m256 a, __m256 b, m256x3 c) { return{ _mm256_fnmadd_ps(a, b, c.x), _mm256_fnmadd_ps(a, b, c.y), _mm256_fnmadd_ps(a, b, c.z) }; }
inline m256x4 fnmadd(__m256 a, __m256 b, m256x4 c) { return{ _mm256_fnmadd_ps(a, b, c.x), _mm256_fnmadd_ps(a, b, c.y), _mm256_fnmadd_ps(a, b, c.z), _mm256_fnmadd_ps(a, b, c.w) }; }

inline m256x2 fnmadd(m256x2 a, __m256 b, __m256 c) { return{ _mm256_fnmadd_ps(a.x, b, c), _mm256_fnmadd_ps(a.y, b, c) }; }
inline m256x3 fnmadd(m256x3 a, __m256 b, __m256 c) { return{ _mm256_fnmadd_ps(a.x, b, c), _mm256_fnmadd_ps(a.y, b, c), _mm256_fnmadd_ps(a.z, b, c) }; }
inline m256x4 fnmadd(m256x4 a, __m256 b, __m256 c) { return{ _mm256_fnmadd_ps(a.x, b, c), _mm256_fnmadd_ps(a.y, b, c), _mm256_fnmadd_ps(a.z, b, c), _mm256_fnmadd_ps(a.w, b, c) }; }

inline m256x2 fnmadd(__m256 a, m256x2 b, __m256 c) { return{ _mm256_fnmadd_ps(a, b.x, c), _mm256_fnmadd_ps(a, b.y, c) }; }
inline m256x3 fnmadd(__m256 a, m256x3 b, __m256 c) { return{ _mm256_fnmadd_ps(a, b.x, c), _mm256_fnmadd_ps(a, b.y, c), _mm256_fnmadd_ps(a, b.z, c) }; }
inline m256x4 fnmadd(__m256 a, m256x4 b, __m256 c) { return{ _mm256_fnmadd_ps(a, b.x, c), _mm256_fnmadd_ps(a, b.y, c), _mm256_fnmadd_ps(a, b.z, c), _mm256_fnmadd_ps(a, b.w, c) }; }

// a * b - c
inline __m256 fmsub(__m256 a, __m256 b, __m256 c) { return  _mm256_fmsub_ps(a, b, c); }

inline m256x2 fmsub(m256x2 a, m256x2 b, m256x2 c) { return{ _mm256_fmsub_ps(a.x, b.x, c.x), _mm256_fmsub_ps(a.y, b.y, c.y) }; }
inline m256x3 fmsub(m256x3 a, m256x3 b, m256x3 c) { return{ _mm256_fmsub_ps(a.x, b.x, c.x), _mm256_fmsub_ps(a.y, b.y, c.y), _mm256_fmsub_ps(a.z, b.z, c.z) }; }
inline m256x4 fmsub(m256x4 a, m256x4 b, m256x4 c) { return{ _mm256_fmsub_ps(a.x, b.x, c.x), _mm256_fmsub_ps(a.y, b.y, c.y), _mm256_fmsub_ps(a.z, b.z, c.z), _mm256_fmsub_ps(a.w, b.w, c.w) }; }

inline m256x2 fmsub(__m256 a, m256x2 b, m256x2 c) { return{ _mm256_fmsub_ps(a, b.x, c.x), _mm256_fmsub_ps(a, b.y, c.y) }; }
inline m256x3 fmsub(__m256 a, m256x3 b, m256x3 c) { return{ _mm256_fmsub_ps(a, b.x, c.x), _mm256_fmsub_ps(a, b.y, c.y), _mm256_fmsub_ps(a, b.z, c.z) }; }
inline m256x4 fmsub(__m256 a, m256x4 b, m256x4 c) { return{ _mm256_fmsub_ps(a, b.x, c.x), _mm256_fmsub_ps(a, b.y, c.y), _mm256_fmsub_ps(a, b.z, c.z), _mm256_fmsub_ps(a, b.w, c.w) }; }

inline m256x2 fmsub(m256x2 a, __m256 b, m256x2 c) { return{ _mm256_fmsub_ps(a.x, b, c.x), _mm256_fmsub_ps(a.y, b, c.y) }; }
inline m256x3 fmsub(m256x3 a, __m256 b, m256x3 c) { return{ _mm256_fmsub_ps(a.x, b, c.x), _mm256_fmsub_ps(a.y, b, c.y), _mm256_fmsub_ps(a.z, b, c.z) }; }
inline m256x4 fmsub(m256x4 a, __m256 b, m256x4 c) { return{ _mm256_fmsub_ps(a.x, b, c.x), _mm256_fmsub_ps(a.y, b, c.y), _mm256_fmsub_ps(a.z, b, c.z), _mm256_fmsub_ps(a.w, b, c.w) }; }

inline m256x2 fmsub(m256x2 a, m256x2 b, __m256 c) { return{ _mm256_fmsub_ps(a.x, b.x, c), _mm256_fmsub_ps(a.y, b.y, c) }; }
inline m256x3 fmsub(m256x3 a, m256x3 b, __m256 c) { return{ _mm256_fmsub_ps(a.x, b.x, c), _mm256_fmsub_ps(a.y, b.y, c), _mm256_fmsub_ps(a.z, b.z, c) }; }
inline m256x4 fmsub(m256x4 a, m256x4 b, __m256 c) { return{ _mm256_fmsub_ps(a.x, b.x, c), _mm256_fmsub_ps(a.y, b.y, c), _mm256_fmsub_ps(a.z, b.z, c), _mm256_fmsub_ps(a.w, b.w, c) }; }

inline m256x2 fmsub(__m256 a, __m256 b, m256x2 c) { return{ _mm256_fmsub_ps(a, b, c.x), _mm256_fmsub_ps(a, b, c.y) }; }
inline m256x3 fmsub(__m256 a, __m256 b, m256x3 c) { return{ _mm256_fmsub_ps(a, b, c.x), _mm256_fmsub_ps(a, b, c.y), _mm256_fmsub_ps(a, b, c.z) }; }
inline m256x4 fmsub(__m256 a, __m256 b, m256x4 c) { return{ _mm256_fmsub_ps(a, b, c.x), _mm256_fmsub_ps(a, b, c.y), _mm256_fmsub_ps(a, b, c.z), _mm256_fmsub_ps(a, b, c.w) }; }

inline m256x2 fmsub(m256x2 a, __m256 b, __m256 c) { return{ _mm256_fmsub_ps(a.x, b, c), _mm256_fmsub_ps(a.y, b, c) }; }
inline m256x3 fmsub(m256x3 a, __m256 b, __m256 c) { return{ _mm256_fmsub_ps(a.x, b, c), _mm256_fmsub_ps(a.y, b, c), _mm256_fmsub_ps(a.z, b, c) }; }
inline m256x4 fmsub(m256x4 a, __m256 b, __m256 c) { return{ _mm256_fmsub_ps(a.x, b, c), _mm256_fmsub_ps(a.y, b, c), _mm256_fmsub_ps(a.z, b, c), _mm256_fmsub_ps(a.w, b, c) }; }

inline m256x2 fmsub(__m256 a, m256x2 b, __m256 c) { return{ _mm256_fmsub_ps(a, b.x, c), _mm256_fmsub_ps(a, b.y, c) }; }
inline m256x3 fmsub(__m256 a, m256x3 b, __m256 c) { return{ _mm256_fmsub_ps(a, b.x, c), _mm256_fmsub_ps(a, b.y, c), _mm256_fmsub_ps(a, b.z, c) }; }
inline m256x4 fmsub(__m256 a, m256x4 b, __m256 c) { return{ _mm256_fmsub_ps(a, b.x, c), _mm256_fmsub_ps(a, b.y, c), _mm256_fmsub_ps(a, b.z, c), _mm256_fmsub_ps(a, b.w, c) }; }

// -(a * b) - c
inline __m256 fnmsub(__m256 a, __m256 b, __m256 c) { return  _mm256_fnmsub_ps(a, b, c); }

inline m256x2 fnmsub(m256x2 a, m256x2 b, m256x2 c) { return{ _mm256_fnmsub_ps(a.x, b.x, c.x), _mm256_fnmsub_ps(a.y, b.y, c.y) }; }
inline m256x3 fnmsub(m256x3 a, m256x3 b, m256x3 c) { return{ _mm256_fnmsub_ps(a.x, b.x, c.x), _mm256_fnmsub_ps(a.y, b.y, c.y), _mm256_fnmsub_ps(a.z, b.z, c.z) }; }
inline m256x4 fnmsub(m256x4 a, m256x4 b, m256x4 c) { return{ _mm256_fnmsub_ps(a.x, b.x, c.x), _mm256_fnmsub_ps(a.y, b.y, c.y), _mm256_fnmsub_ps(a.z, b.z, c.z), _mm256_fnmsub_ps(a.w, b.w, c.w) }; }

inline m256x2 fnmsub(__m256 a, m256x2 b, m256x2 c) { return{ _mm256_fnmsub_ps(a, b.x, c.x), _mm256_fnmsub_ps(a, b.y, c.y) }; }
inline m256x3 fnmsub(__m256 a, m256x3 b, m256x3 c) { return{ _mm256_fnmsub_ps(a, b.x, c.x), _mm256_fnmsub_ps(a, b.y, c.y), _mm256_fnmsub_ps(a, b.z, c.z) }; }
inline m256x4 fnmsub(__m256 a, m256x4 b, m256x4 c) { return{ _mm256_fnmsub_ps(a, b.x, c.x), _mm256_fnmsub_ps(a, b.y, c.y), _mm256_fnmsub_ps(a, b.z, c.z), _mm256_fnmsub_ps(a, b.w, c.w) }; }

inline m256x2 fnmsub(m256x2 a, __m256 b, m256x2 c) { return{ _mm256_fnmsub_ps(a.x, b, c.x), _mm256_fnmsub_ps(a.y, b, c.y) }; }
inline m256x3 fnmsub(m256x3 a, __m256 b, m256x3 c) { return{ _mm256_fnmsub_ps(a.x, b, c.x), _mm256_fnmsub_ps(a.y, b, c.y), _mm256_fnmsub_ps(a.z, b, c.z) }; }
inline m256x4 fnmsub(m256x4 a, __m256 b, m256x4 c) { return{ _mm256_fnmsub_ps(a.x, b, c.x), _mm256_fnmsub_ps(a.y, b, c.y), _mm256_fnmsub_ps(a.z, b, c.z), _mm256_fnmsub_ps(a.w, b, c.w) }; }

inline m256x2 fnmsub(m256x2 a, m256x2 b, __m256 c) { return{ _mm256_fnmsub_ps(a.x, b.x, c), _mm256_fnmsub_ps(a.y, b.y, c) }; }
inline m256x3 fnmsub(m256x3 a, m256x3 b, __m256 c) { return{ _mm256_fnmsub_ps(a.x, b.x, c), _mm256_fnmsub_ps(a.y, b.y, c), _mm256_fnmsub_ps(a.z, b.z, c) }; }
inline m256x4 fnmsub(m256x4 a, m256x4 b, __m256 c) { return{ _mm256_fnmsub_ps(a.x, b.x, c), _mm256_fnmsub_ps(a.y, b.y, c), _mm256_fnmsub_ps(a.z, b.z, c), _mm256_fnmsub_ps(a.w, b.w, c) }; }

inline m256x2 fnmsub(__m256 a, __m256 b, m256x2 c) { return{ _mm256_fnmsub_ps(a, b, c.x), _mm256_fnmsub_ps(a, b, c.y) }; }
inline m256x3 fnmsub(__m256 a, __m256 b, m256x3 c) { return{ _mm256_fnmsub_ps(a, b, c.x), _mm256_fnmsub_ps(a, b, c.y), _mm256_fnmsub_ps(a, b, c.z) }; }
inline m256x4 fnmsub(__m256 a, __m256 b, m256x4 c) { return{ _mm256_fnmsub_ps(a, b, c.x), _mm256_fnmsub_ps(a, b, c.y), _mm256_fnmsub_ps(a, b, c.z), _mm256_fnmsub_ps(a, b, c.w) }; }

inline m256x2 fnmsub(m256x2 a, __m256 b, __m256 c) { return{ _mm256_fnmsub_ps(a.x, b, c), _mm256_fnmsub_ps(a.y, b, c) }; }
inline m256x3 fnmsub(m256x3 a, __m256 b, __m256 c) { return{ _mm256_fnmsub_ps(a.x, b, c), _mm256_fnmsub_ps(a.y, b, c), _mm256_fnmsub_ps(a.z, b, c) }; }
inline m256x4 fnmsub(m256x4 a, __m256 b, __m256 c) { return{ _mm256_fnmsub_ps(a.x, b, c), _mm256_fnmsub_ps(a.y, b, c), _mm256_fnmsub_ps(a.z, b, c), _mm256_fnmsub_ps(a.w, b, c) }; }

inline m256x2 fnmsub(__m256 a, m256x2 b, __m256 c) { return{ _mm256_fnmsub_ps(a, b.x, c), _mm256_fnmsub_ps(a, b.y, c) }; }
inline m256x3 fnmsub(__m256 a, m256x3 b, __m256 c) { return{ _mm256_fnmsub_ps(a, b.x, c), _mm256_fnmsub_ps(a, b.y, c), _mm256_fnmsub_ps(a, b.z, c) }; }
inline m256x4 fnmsub(__m256 a, m256x4 b, __m256 c) { return{ _mm256_fnmsub_ps(a, b.x, c), _mm256_fnmsub_ps(a, b.y, c), _mm256_fnmsub_ps(a, b.z, c), _mm256_fnmsub_ps(a, b.w, c) }; }

/*
	Convenience versions for promoting single float operands to packed floats
*/

inline __m256 add(__m256 v, f32 c) { return add(v, _mm256_set1_ps(c)); }
inline m256x2 add(m256x2 v, f32 c) { return add(v, _mm256_set1_ps(c)); }
inline m256x3 add(m256x3 v, f32 c) { return add(v, _mm256_set1_ps(c)); }
inline m256x4 add(m256x4 v, f32 c) { return add(v, _mm256_set1_ps(c)); }
inline __m256 add(f32 c, __m256 v) { return add(_mm256_set1_ps(c), v); }
inline m256x2 add(f32 c, m256x2 v) { return add(_mm256_set1_ps(c), v); }
inline m256x3 add(f32 c, m256x3 v) { return add(_mm256_set1_ps(c), v); }
inline m256x4 add(f32 c, m256x4 v) { return add(_mm256_set1_ps(c), v); }

inline __m256 sub(__m256 v, f32 c) { return sub(v, _mm256_set1_ps(c)); }
inline m256x2 sub(m256x2 v, f32 c) { return sub(v, _mm256_set1_ps(c)); }
inline m256x3 sub(m256x3 v, f32 c) { return sub(v, _mm256_set1_ps(c)); }
inline m256x4 sub(m256x4 v, f32 c) { return sub(v, _mm256_set1_ps(c)); }
inline __m256 sub(f32 c, __m256 v) { return sub(_mm256_set1_ps(c), v); }
inline m256x2 sub(f32 c, m256x2 v) { return sub(_mm256_set1_ps(c), v); }
inline m256x3 sub(f32 c, m256x3 v) { return sub(_mm256_set1_ps(c), v); }
inline m256x4 sub(f32 c, m256x4 v) { return sub(_mm256_set1_ps(c), v); }

inline __m256 mul(__m256 v, f32 c) { return mul(v, _mm256_set1_ps(c)); }
inline m256x2 mul(m256x2 v, f32 c) { return mul(v, _mm256_set1_ps(c)); }
inline m256x3 mul(m256x3 v, f32 c) { return mul(v, _mm256_set1_ps(c)); }
inline m256x4 mul(m256x4 v, f32 c) { return mul(v, _mm256_set1_ps(c)); }
inline __m256 mul(f32 c, __m256 v) { return mul(_mm256_set1_ps(c), v); }
inline m256x2 mul(f32 c, m256x2 v) { return mul(_mm256_set1_ps(c), v); }
inline m256x3 mul(f32 c, m256x3 v) { return mul(_mm256_set1_ps(c), v); }
inline m256x4 mul(f32 c, m256x4 v) { return mul(_mm256_set1_ps(c), v); }

inline __m256 div(__m256 v, f32 c) { return div(v, _mm256_set1_ps(c)); }
inline m256x2 div(m256x2 v, f32 c) { return div(v, _mm256_set1_ps(c)); }
inline m256x3 div(m256x3 v, f32 c) { return div(v, _mm256_set1_ps(c)); }
inline m256x4 div(m256x4 v, f32 c) { return div(v, _mm256_set1_ps(c)); }
inline __m256 div(f32 c, __m256 v) { return div(_mm256_set1_ps(c), v); }
inline m256x2 div(f32 c, m256x2 v) { return div(_mm256_set1_ps(c), v); }
inline m256x3 div(f32 c, m256x3 v) { return div(_mm256_set1_ps(c), v); }
inline m256x4 div(f32 c, m256x4 v) { return div(_mm256_set1_ps(c), v); }

/*
Float comparison operators
https://stackoverflow.com/questions/16988199/how-to-choose-avx-compare-predicate-variants
// Se Intel intrinsics guide for _mm256_cmp_ps
*/
inline __m256 compare_eq(__m256 a, __m256 b) { return _mm256_cmp_ps(a, b, _CMP_EQ_OQ); }  // == ? ? 0xFFFFFFFF : 0
inline __m256 compare_ne(__m256 a, __m256 b) { return _mm256_cmp_ps(a, b, _CMP_NEQ_OQ); } // !=	? ? 0xFFFFFFFF : 0
inline __m256 compare_lt(__m256 a, __m256 b) { return _mm256_cmp_ps(a, b, _CMP_LT_OQ); }  // <	? ? 0xFFFFFFFF : 0
inline __m256 compare_le(__m256 a, __m256 b) { return _mm256_cmp_ps(a, b, _CMP_LE_OQ); }  // <=	? ? 0xFFFFFFFF : 0
inline __m256 compare_gt(__m256 a, __m256 b) { return _mm256_cmp_ps(a, b, _CMP_GT_OQ); }  // >	? ? 0xFFFFFFFF : 0
inline __m256 compare_ge(__m256 a, __m256 b) { return _mm256_cmp_ps(a, b, _CMP_GE_OQ); }  // >=	? ? 0xFFFFFFFF : 0

inline m256x2 compare_eq(m256x2 a, m256x2 b) { return m256x2{ compare_eq(a.x, b.x), compare_eq(a.y, b.y) }; }   // == ? ? 0xFFFFFFFF : 0
inline m256x2 compare_ne(m256x2 a, m256x2 b) { return m256x2{ compare_ne(a.x, b.x), compare_ne(a.y, b.y) }; }  // !=	? ? 0xFFFFFFFF : 0
inline m256x2 compare_lt(m256x2 a, m256x2 b) { return m256x2{ compare_lt(a.x, b.x), compare_lt(a.y, b.y) }; }  // <	? ? 0xFFFFFFFF : 0
inline m256x2 compare_le(m256x2 a, m256x2 b) { return m256x2{ compare_le(a.x, b.x), compare_le(a.y, b.y) }; }  // <=	? ? 0xFFFFFFFF : 0
inline m256x2 compare_gt(m256x2 a, m256x2 b) { return m256x2{ compare_gt(a.x, b.x), compare_gt(a.y, b.y) }; }  // >	? ? 0xFFFFFFFF : 0
inline m256x2 compare_ge(m256x2 a, m256x2 b) { return m256x2{ compare_ge(a.x, b.x), compare_ge(a.y, b.y) }; }  // >=	? ? 0xFFFFFFFF : 0

inline m256x3 compare_eq(m256x3 a, m256x3 b) { return m256x3{ compare_eq(a.x, b.x), compare_eq(a.y, b.y), compare_eq(a.z, b.z) }; }   // == ? ? 0xFFFFFFFF : 0
inline m256x3 compare_ne(m256x3 a, m256x3 b) { return m256x3{ compare_ne(a.x, b.x), compare_ne(a.y, b.y), compare_ne(a.z, b.z) }; }  // !=	? ? 0xFFFFFFFF : 0
inline m256x3 compare_lt(m256x3 a, m256x3 b) { return m256x3{ compare_lt(a.x, b.x), compare_lt(a.y, b.y), compare_lt(a.z, b.z) }; }  // <	? ? 0xFFFFFFFF : 0
inline m256x3 compare_le(m256x3 a, m256x3 b) { return m256x3{ compare_le(a.x, b.x), compare_le(a.y, b.y), compare_le(a.z, b.z) }; }  // <=	? ? 0xFFFFFFFF : 0
inline m256x3 compare_gt(m256x3 a, m256x3 b) { return m256x3{ compare_gt(a.x, b.x), compare_gt(a.y, b.y), compare_gt(a.z, b.z) }; }  // >	? ? 0xFFFFFFFF : 0
inline m256x3 compare_ge(m256x3 a, m256x3 b) { return m256x3{ compare_ge(a.x, b.x), compare_ge(a.y, b.y), compare_ge(a.z, b.z) }; }  // >=	? ? 0xFFFFFFFF : 0

inline m256x4 compare_eq(m256x4 a, m256x4 b) { return m256x4{ compare_eq(a.x, b.x), compare_eq(a.y, b.y), compare_eq(a.z, b.z), compare_eq(a.w, b.w) }; }   // == ? ? 0xFFFFFFFF : 0
inline m256x4 compare_ne(m256x4 a, m256x4 b) { return m256x4{ compare_ne(a.x, b.x), compare_ne(a.y, b.y), compare_ne(a.z, b.z), compare_ne(a.w, b.w) }; }  // !=	? ? 0xFFFFFFFF : 0
inline m256x4 compare_lt(m256x4 a, m256x4 b) { return m256x4{ compare_lt(a.x, b.x), compare_lt(a.y, b.y), compare_lt(a.z, b.z), compare_lt(a.w, b.w) }; }  // <	? ? 0xFFFFFFFF : 0
inline m256x4 compare_le(m256x4 a, m256x4 b) { return m256x4{ compare_le(a.x, b.x), compare_le(a.y, b.y), compare_le(a.z, b.z), compare_le(a.w, b.w) }; }  // <=	? ? 0xFFFFFFFF : 0
inline m256x4 compare_gt(m256x4 a, m256x4 b) { return m256x4{ compare_gt(a.x, b.x), compare_gt(a.y, b.y), compare_gt(a.z, b.z), compare_gt(a.w, b.w) }; }  // >	? ? 0xFFFFFFFF : 0
inline m256x4 compare_ge(m256x4 a, m256x4 b) { return m256x4{ compare_ge(a.x, b.x), compare_ge(a.y, b.y), compare_ge(a.z, b.z), compare_ge(a.w, b.w) }; }  // >=	? ? 0xFFFFFFFF : 0

/*
	Logic and test operators
*/

inline __m256 bitwise_and(__m256 a, __m256 b)		{	return _mm256_and_ps(a, b);		}
inline __m256 bitwise_and_not(__m256 a, __m256 b)	{	return _mm256_andnot_ps(a, b);	}
inline __m256 bitwise_or(__m256 a, __m256 b)		{	return _mm256_or_ps(a, b);		}
inline __m256 bitwise_xor(__m256 a, __m256 b)		{	return _mm256_xor_ps(a, b);		}
inline __m256 bitwise_not(__m256 a) { return _mm256_xor_ps(a, _mm256_castsi256_ps(_mm256_set1_epi32(-1))); }

/*
	'Special' Math functions
*/

inline __m256 max_ps(__m256 a, __m256 b) { return _mm256_max_ps(a, b); }
inline m256x2 max_ps(m256x2 a, m256x2 b) { return m256x2{ _mm256_max_ps(a.x, b.x), _mm256_max_ps(a.y, b.y) }; }
inline m256x3 max_ps(m256x3 a, m256x3 b) { return m256x3{ _mm256_max_ps(a.x, b.x), _mm256_max_ps(a.y, b.y), _mm256_max_ps(a.z, b.z) }; }
inline m256x4 max_ps(m256x4 a, m256x4 b) { return m256x4{ _mm256_max_ps(a.x, b.x), _mm256_max_ps(a.y, b.y), _mm256_max_ps(a.z, b.z), _mm256_max_ps(a.w, b.w) }; }

inline __m256 min_ps(__m256 a, __m256 b) { return _mm256_min_ps(a, b); }
inline m256x2 min_ps(m256x2 a, m256x2 b) { return m256x2{ _mm256_min_ps(a.x, b.x), _mm256_min_ps(a.y, b.y) }; }
inline m256x3 min_ps(m256x3 a, m256x3 b) { return m256x3{ _mm256_min_ps(a.x, b.x), _mm256_min_ps(a.y, b.y), _mm256_min_ps(a.z, b.z) }; }
inline m256x4 min_ps(m256x4 a, m256x4 b) { return m256x4{ _mm256_min_ps(a.x, b.x), _mm256_min_ps(a.y, b.y), _mm256_min_ps(a.z, b.z), _mm256_min_ps(a.w, b.w) }; }

inline __m256 negate_ps(__m256 a) { return _mm256_xor_ps(a, _mm256_set1_ps(-0.f)); }
inline m256x2 negate_ps(m256x2 a) { return m256x2{ negate_ps(a.x), negate_ps(a.y) }; }
inline m256x3 negate_ps(m256x3 a) { return m256x3{ negate_ps(a.x), negate_ps(a.y), negate_ps(a.z) }; }
inline m256x4 negate_ps(m256x4 a) { return m256x4{ negate_ps(a.x), negate_ps(a.y), negate_ps(a.z), negate_ps(a.w) }; }

inline __m256 abs_ps(__m256 a) { return _mm256_andnot_ps(_mm256_set1_ps(-0.f), a); }
inline m256x2 abs_ps(m256x2 a) { return m256x2{ abs_ps(a.x), abs_ps(a.y) }; }
inline m256x3 abs_ps(m256x3 a) { return m256x3{ abs_ps(a.x), abs_ps(a.y), abs_ps(a.z) }; }
inline m256x4 abs_ps(m256x4 a) { return m256x4{ abs_ps(a.x), abs_ps(a.y), abs_ps(a.z), abs_ps(a.w) }; }

inline __m256 signflip_ps(__m256 a) { return _mm256_xor_ps(_mm256_set1_ps(-0.f), a); }
inline m256x2 signflip_ps(m256x2 a) { return m256x2{ signflip_ps(a.x), signflip_ps(a.y) }; }
inline m256x3 signflip_ps(m256x3 a) { return m256x3{ signflip_ps(a.x), signflip_ps(a.y), signflip_ps(a.z) }; }
inline m256x4 signflip_ps(m256x4 a) { return m256x4{ signflip_ps(a.x), signflip_ps(a.y), signflip_ps(a.z), signflip_ps(a.w) }; }

inline __m256 round_ceil(__m256 a) { return _mm256_round_ps(a, _MM_FROUND_CEIL); }
inline m256x2 round_ceil(m256x2 a) { return m256x2{ round_ceil(a.x), round_ceil(a.y) }; }
inline m256x3 round_ceil(m256x3 a) { return m256x3{ round_ceil(a.x), round_ceil(a.y), round_ceil(a.z) }; }
inline m256x4 round_ceil(m256x4 a) { return m256x4{ round_ceil(a.x), round_ceil(a.y), round_ceil(a.z), round_ceil(a.w) }; }

inline __m256 round_floor(__m256 a) { return _mm256_round_ps(a, _MM_FROUND_FLOOR); }
inline m256x2 round_floor(m256x2 a) { return m256x2{ round_floor(a.x), round_floor(a.y) }; }
inline m256x3 round_floor(m256x3 a) { return m256x3{ round_floor(a.x), round_floor(a.y), round_floor(a.z) }; }
inline m256x4 round_floor(m256x4 a) { return m256x4{ round_floor(a.x), round_floor(a.y), round_floor(a.z), round_floor(a.w) }; }

inline __m256 round_nearest(__m256 a) { return _mm256_round_ps(a, _MM_FROUND_NINT); }
inline m256x2 round_nearest(m256x2 a) { return m256x2{ round_nearest(a.x), round_nearest(a.y) }; }
inline m256x3 round_nearest(m256x3 a) { return m256x3{ round_nearest(a.x), round_nearest(a.y), round_nearest(a.z) }; }
inline m256x4 round_nearest(m256x4 a) { return m256x4{ round_nearest(a.x), round_nearest(a.y), round_nearest(a.z), round_nearest(a.w) }; }

inline __m256 fract(__m256 a) { return sub(a, round_floor(a)); }
inline m256x2 fract(m256x2 a) { return m256x2{ fract(a.x), fract(a.y) }; }
inline m256x3 fract(m256x3 a) { return m256x3{ fract(a.x), fract(a.y), fract(a.z) }; }
inline m256x4 fract(m256x4 a) { return m256x4{ fract(a.x), fract(a.y), fract(a.z), fract(a.w) }; }

inline __m256 clamp(__m256 x, __m256 minval, __m256 maxval) { return min_ps(max_ps(x, minval), maxval); }
inline m256x2 clamp(m256x2 x, m256x2 minval, m256x2 maxval) { return m256x2{ clamp(x.x, minval.x, maxval.x), clamp(x.y, minval.y, maxval.y) }; }
inline m256x3 clamp(m256x3 x, m256x3 minval, m256x3 maxval) { return m256x3{ clamp(x.x, minval.x, maxval.x), clamp(x.y, minval.y, maxval.y), clamp(x.z, minval.z, maxval.z) }; }
inline m256x4 clamp(m256x4 x, m256x4 minval, m256x4 maxval) { return m256x4{ clamp(x.x, minval.x, maxval.x), clamp(x.y, minval.y, maxval.y), clamp(x.z, minval.z, maxval.z), clamp(x.w, minval.w, maxval.w) }; }

inline __m256 saturate(__m256 x) { return clamp(x, _mm256_set1_ps(0.f), _mm256_set1_ps(1.f)); }
inline m256x2 saturate(m256x2 x) { return m256x2{ saturate(x.x), saturate(x.y) }; }
inline m256x3 saturate(m256x3 x) { return m256x3{ saturate(x.x), saturate(x.y), saturate(x.z) }; }
inline m256x4 saturate(m256x4 x) { return m256x4{ saturate(x.x), saturate(x.y), saturate(x.z), saturate(x.w) }; }

inline f32 rcp(f32 a) { return 1.f / a;    } // reciprocal

inline __m256 rcp(__m256 a)   { return _mm256_rcp_ps(a); } // reciprocal
inline m256x2 rcp(m256x2 a) { return m256x2{ rcp(a.x), rcp(a.y) }; }
inline m256x3 rcp(m256x3 a) { return m256x3{ rcp(a.x), rcp(a.y), rcp(a.z) }; }
inline m256x4 rcp(m256x4 a) { return m256x4{ rcp(a.x), rcp(a.y), rcp(a.z), rcp(a.w) }; }

inline __m256 setzero_ps() { return _mm256_setzero_ps(); } // compiles to xor reg reg
inline m256x2 setzero2_ps() { return m256x2{ _mm256_setzero_ps(), _mm256_setzero_ps() }; }
inline m256x3 setzero3_ps() { return m256x3{ _mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps() }; }
inline m256x4 setzero4_ps() { return m256x4{ _mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps() }; }

/*
	shuffle operators
*/

/*
	Algebraic and trancendental functions
	// Some require SVML, and needs MS Visual Studio 2019
*/

inline f32 sroot(f32 a) { return sqrtf(a); } // square root
inline f32 rsroot(f32 a) { return 1.f / sqrtf(a); } // reciprocal square root

inline __m256 sroot(__m256 a)  { return _mm256_sqrt_ps(a); } // square root
inline m256x2 sroot(m256x2 a) { return m256x2{ _mm256_sqrt_ps(a.x), _mm256_sqrt_ps(a.y) }; }
inline m256x3 sroot(m256x3 a) { return m256x3{ _mm256_sqrt_ps(a.x), _mm256_sqrt_ps(a.y), _mm256_sqrt_ps(a.z) }; }
inline m256x4 sroot(m256x4 a) { return m256x4{ _mm256_sqrt_ps(a.x), _mm256_sqrt_ps(a.y), _mm256_sqrt_ps(a.z), _mm256_sqrt_ps(a.w) }; }

inline __m256 rsroot(__m256 a) { return _mm256_rsqrt_ps(a); } // reciprocal square root
inline m256x2 rsroot(m256x2 a) { return m256x2{ _mm256_rsqrt_ps(a.x), _mm256_rsqrt_ps(a.y) }; }
inline m256x3 rsroot(m256x3 a) { return m256x3{ _mm256_rsqrt_ps(a.x), _mm256_rsqrt_ps(a.y), _mm256_rsqrt_ps(a.z) }; }
inline m256x4 rsroot(m256x4 a) { return m256x4{ _mm256_rsqrt_ps(a.x), _mm256_rsqrt_ps(a.y), _mm256_rsqrt_ps(a.z), _mm256_rsqrt_ps(a.w) }; }

inline __m256 sin_ps(__m256 a) { return _mm256_sin_ps(a); }
inline m256x2 sin_ps(m256x2 a) { return m256x2{_mm256_sin_ps(a.x), _mm256_sin_ps(a.y) }; }
inline m256x3 sin_ps(m256x3 a) { return m256x3{ _mm256_sin_ps(a.x), _mm256_sin_ps(a.y), _mm256_sin_ps(a.z) }; }
inline m256x4 sin_ps(m256x4 a) { return m256x4{ _mm256_sin_ps(a.x), _mm256_sin_ps(a.y), _mm256_sin_ps(a.z), _mm256_sin_ps(a.w) }; }

inline __m256 cos_ps(__m256 a) { return _mm256_cos_ps(a); }
inline m256x2 cos_ps(m256x2 a) { return m256x2{ _mm256_cos_ps(a.x), _mm256_cos_ps(a.y) }; }
inline m256x3 cos_ps(m256x3 a) { return m256x3{ _mm256_cos_ps(a.x), _mm256_cos_ps(a.y), _mm256_cos_ps(a.z) }; }
inline m256x4 cos_ps(m256x4 a) { return m256x4{ _mm256_cos_ps(a.x), _mm256_cos_ps(a.y), _mm256_cos_ps(a.z), _mm256_cos_ps(a.w) }; }

inline __m256 tan_ps(__m256 a) { return _mm256_tan_ps(a); }
inline m256x2 tan_ps(m256x2 a) { return m256x2{ _mm256_tan_ps(a.x), _mm256_tan_ps(a.y) }; }
inline m256x3 tan_ps(m256x3 a) { return m256x3{ _mm256_tan_ps(a.x), _mm256_tan_ps(a.y), _mm256_tan_ps(a.z) }; }
inline m256x4 tan_ps(m256x4 a) { return m256x4{ _mm256_tan_ps(a.x), _mm256_tan_ps(a.y), _mm256_tan_ps(a.z), _mm256_tan_ps(a.w) }; }

inline __m256 asin_ps(__m256 a) { return _mm256_asin_ps(a); }
inline m256x2 asin_ps(m256x2 a) { return m256x2{ _mm256_asin_ps(a.x), _mm256_asin_ps(a.y) }; }
inline m256x3 asin_ps(m256x3 a) { return m256x3{ _mm256_asin_ps(a.x), _mm256_asin_ps(a.y), _mm256_asin_ps(a.z) }; }
inline m256x4 asin_ps(m256x4 a) { return m256x4{ _mm256_asin_ps(a.x), _mm256_asin_ps(a.y), _mm256_asin_ps(a.z), _mm256_asin_ps(a.w) }; }

inline __m256 acos_ps(__m256 a) { return _mm256_acos_ps(a); }
inline m256x2 acos_ps(m256x2 a) { return m256x2{ _mm256_acos_ps(a.x), _mm256_acos_ps(a.y) }; }
inline m256x3 acos_ps(m256x3 a) { return m256x3{ _mm256_acos_ps(a.x), _mm256_acos_ps(a.y), _mm256_acos_ps(a.z) }; }
inline m256x4 acos_ps(m256x4 a) { return m256x4{ _mm256_acos_ps(a.x), _mm256_acos_ps(a.y), _mm256_acos_ps(a.z), _mm256_acos_ps(a.w) }; }

inline __m256 atan_ps(__m256 a) { return _mm256_atan_ps(a); }
inline m256x2 atan_ps(m256x2 a) { return m256x2{ _mm256_atan_ps(a.x), _mm256_atan_ps(a.y) }; }
inline m256x3 atan_ps(m256x3 a) { return m256x3{ _mm256_atan_ps(a.x), _mm256_atan_ps(a.y), _mm256_atan_ps(a.z) }; }
inline m256x4 atan_ps(m256x4 a) { return m256x4{ _mm256_atan_ps(a.x), _mm256_atan_ps(a.y), _mm256_atan_ps(a.z), _mm256_atan_ps(a.w) }; }

inline __m256 atan2_ps(__m256 a, __m256 b) { return _mm256_atan2_ps(a, b); }
inline m256x2 atan2_ps(m256x2 a, m256x2 b) { return m256x2{ _mm256_atan2_ps(a.x, b.x), _mm256_atan2_ps(a.y, b.y) }; }
inline m256x3 atan2_ps(m256x3 a, m256x3 b) { return m256x3{ _mm256_atan2_ps(a.x, b.x), _mm256_atan2_ps(a.y, b.y), _mm256_atan2_ps(a.z, b.z) }; }
inline m256x4 atan2_ps(m256x4 a, m256x4 b) { return m256x4{ _mm256_atan2_ps(a.x, b.x), _mm256_atan2_ps(a.y, b.y), _mm256_atan2_ps(a.z, b.z), _mm256_atan2_ps(a.w, b.w) }; }

inline __m256 exp_ps(__m256 a) { return _mm256_exp_ps(a); }
inline m256x2 exp_ps(m256x2 a) { return m256x2{ _mm256_exp_ps(a.x), _mm256_exp_ps(a.y) }; }
inline m256x3 exp_ps(m256x3 a) { return m256x3{ _mm256_exp_ps(a.x), _mm256_exp_ps(a.y), _mm256_exp_ps(a.z) }; }
inline m256x4 exp_ps(m256x4 a) { return m256x4{ _mm256_exp_ps(a.x), _mm256_exp_ps(a.y), _mm256_exp_ps(a.z), _mm256_exp_ps(a.w) }; }

inline __m256 log_ps(__m256 a) { return _mm256_log_ps(a); }
inline m256x2 log_ps(m256x2 a) { return m256x2{ _mm256_log_ps(a.x), _mm256_log_ps(a.y) }; }
inline m256x3 log_ps(m256x3 a) { return m256x3{ _mm256_log_ps(a.x), _mm256_log_ps(a.y), _mm256_log_ps(a.z) }; }
inline m256x4 log_ps(m256x4 a) { return m256x4{ _mm256_log_ps(a.x), _mm256_log_ps(a.y), _mm256_log_ps(a.z), _mm256_log_ps(a.w) }; }

inline __m256 pow_ps(__m256 a, __m256 b) { return _mm256_pow_ps(a, b); }
inline m256x2 pow_ps(m256x2 a, m256x2 b) { return m256x2{ _mm256_pow_ps(a.x, b.x), _mm256_pow_ps(a.y, b.y) }; }
inline m256x3 pow_ps(m256x3 a, m256x3 b) { return m256x3{ _mm256_pow_ps(a.x, b.x), _mm256_pow_ps(a.y, b.y), _mm256_pow_ps(a.z, b.z) }; }
inline m256x4 pow_ps(m256x4 a, m256x4 b) { return m256x4{ _mm256_pow_ps(a.x, b.x), _mm256_pow_ps(a.y, b.y), _mm256_pow_ps(a.z, b.z), _mm256_pow_ps(a.w, b.w) }; }

inline __m256 sincos_ps(__m256* p_cos, __m256 v1) { return _mm256_sincos_ps(p_cos, v1); }

inline __m256 approx_exp_ps(__m256 a)
{ 
	// calc (1 + a / 32) ^ 32
	// 1./32. = 0.03125
	// ajustment: 1./32.67 = 0.0306091215182124

	// 16 factors: 1./16.68 = 0.05995203836930455f
	//__m256 b = _mm256_fmadd_ps(a, _mm256_set1_ps(0.0306091215182124f), _mm256_set1_ps(1.f));  // (1.f + a * 0.0306091215182124f); // 32 factor approx
	__m256 b = _mm256_fmadd_ps(a, _mm256_set1_ps(0.05995203836930455f), _mm256_set1_ps(1.f));  // (1.f + a * 0.05995203836930455f); // 16 factor approx
	__m256 b2  = mul(b, b);
	__m256 b4  = mul(b2, b2);
	__m256 b8  = mul(b4, b4);
	__m256 b16 = mul(b8, b8); // 16 factor approx, max abs error about about 0.008
	//__m256 b32 = mul(b16, b16); // should have max abs error of about 0.004 from -10 to 0
	return b16;
}

inline m256x2 approx_exp_ps(m256x2 a) { return m256x2{ approx_exp_ps(a.x), approx_exp_ps(a.y) }; }
inline m256x3 approx_exp_ps(m256x3 a) { return m256x3{ approx_exp_ps(a.x), approx_exp_ps(a.y), approx_exp_ps(a.z) }; }
inline m256x4 approx_exp_ps(m256x4 a) { return m256x4{ approx_exp_ps(a.x), approx_exp_ps(a.y), approx_exp_ps(a.z), approx_exp_ps(a.w) }; }

/*
	Loads and stores
*/
  
inline __m256 set1_ps(f32 a)						{ return   _mm256_set1_ps(a); }
inline m256x2 set1x2_ps(f32 x, f32 y)				{ return { _mm256_set1_ps(x), _mm256_set1_ps(y) }; }
inline m256x3 set1x3_ps(f32 x, f32 y, f32 z)		{ return { _mm256_set1_ps(x), _mm256_set1_ps(y), _mm256_set1_ps(z) }; }
inline m256x4 set1x4_ps(f32 x, f32 y, f32 z, f32 w)	{ return { _mm256_set1_ps(x), _mm256_set1_ps(y), _mm256_set1_ps(z), _mm256_set1_ps(w),}; }

inline __m256 set_ps(f32 e7, f32 e6, f32 e5, f32 e4, f32 e3, f32 e2, f32 e1, f32 e0) { return _mm256_set_ps(e7, e6, e5, e4, e3, e2, e1, e0); }

inline __m256 load_ps(f32 const* mem_addr) { return _mm256_load_ps(mem_addr); }
inline __m256 maskload_ps(f32 const* mem_addr, __m256i mask) { return _mm256_maskload_ps(mem_addr, mask); }

inline void store_ps(f32* mem_addr, __m256 a) { _mm256_store_ps(mem_addr, a); }
inline void maskstore_ps(f32* mem_addr, __m256i mask, __m256 a) { _mm256_maskstore_ps(mem_addr, mask, a); }

// Scale ~ size of datatype (?)
// Gather addresses calculated in bytes
// each address loads an f32 (4 byte)
 //inline __m256 gather_ps(f32 const* base_addr, __m256i vindex, const i32 scale) { return _mm256_i32gather_ps(base_addr, vindex, scale); }
// inline __m256 mask_gather_ps(__m256 src, f32 const * base_addr, __m256i vindex, __m256 mask, const i32 scale) { return _mm256_mask_i32gather_ps(src, base_addr, vindex, mask, scale); }

inline __m256 blend_ps(__m256 a, __m256 b, __m256 mask) { return  _mm256_blendv_ps(a, b, mask); }
inline __m256 if_then_else(__m256 cond, __m256 thenDo, __m256 elseDo) { return  _mm256_blendv_ps(elseDo, thenDo, cond); } // switcheroo of blend_ps to match C ternary operator

inline m256x2 blend2_ps(m256x2 a, m256x2 b, __m256 mask) { return  { _mm256_blendv_ps(a.x, b.x, mask), _mm256_blendv_ps(a.y, b.y, mask) }; }
inline m256x3 blend3_ps(m256x3 a, m256x3 b, __m256 mask) { return  { _mm256_blendv_ps(a.x, b.x, mask), _mm256_blendv_ps(a.y, b.y, mask), _mm256_blendv_ps(a.z, b.z, mask) }; }
inline m256x4 blend4_ps(m256x4 a, m256x4 b, __m256 mask) { return  { _mm256_blendv_ps(a.x, b.x, mask), _mm256_blendv_ps(a.y, b.y, mask), _mm256_blendv_ps(a.z, b.z, mask), _mm256_blendv_ps(a.w, b.w, mask) }; }

inline m256x2 blend2_ps(m256x2 a, m256x2 b, m256x2 mask) { return  { _mm256_blendv_ps(a.x, b.x, mask.x), _mm256_blendv_ps(a.y, b.y, mask.y) }; }
inline m256x3 blend3_ps(m256x3 a, m256x3 b, m256x3 mask) { return  { _mm256_blendv_ps(a.x, b.x, mask.x), _mm256_blendv_ps(a.y, b.y, mask.y), _mm256_blendv_ps(a.z, b.z, mask.z) }; }
inline m256x4 blend4_ps(m256x4 a, m256x4 b, m256x4 mask) { return  { _mm256_blendv_ps(a.x, b.x, mask.x), _mm256_blendv_ps(a.y, b.y, mask.y), _mm256_blendv_ps(a.z, b.z, mask.z), _mm256_blendv_ps(a.w, b.w, mask.w) }; }

inline bool any_set(__m256 mask)  { return _mm256_movemask_ps(mask) != 0; }
inline bool all_set(__m256 mask)  { return _mm256_movemask_ps(mask) == 0b11111111; }
inline bool none_set(__m256 mask) { return _mm256_movemask_ps(mask) == 0; }

/*
	Random number generation
	//See https://software.intel.com/content/www/us/en/develop/documentation/cpp-compiler-developer-guide-and-reference/top/compiler-reference/intrinsics/intrinsics-for-the-short-vector-random-number-generator-library.html
*/

inline void non_temporal_store(f32 * mem_addr, __m256 a) { _mm256_stream_ps(mem_addr, a); };

/*
	Operator overloads and functions
*/

template<typename T> inline T operator==(T u, T v) { return compare_eq(u, v); }
template<typename T> inline T operator!=(T u, T v) { return compare_ne(u, v); }
template<typename T> inline T operator <(T u, T v) { return compare_lt(u, v); }
template<typename T> inline T operator<=(T u, T v) { return compare_le(u, v); }
template<typename T> inline T operator >(T u, T v) { return compare_gt(u, v); }
template<typename T> inline T operator>=(T u, T v) { return compare_ge(u, v); }

/*
	Non-SIMD arithmetic operators
*/

inline f32x2 operator+(f32x2 u, f32x2 v) { return add(u, v); }
inline f32x2 operator-(f32x2 u, f32x2 v) { return sub(u, v); }
inline f32x2 operator*(f32x2 u, f32x2 v) { return mul(u, v); }
inline f32x2 operator/(f32x2 u, f32x2 v) { return div(u, v); }
inline f32x3 operator+(f32x3 u, f32x3 v) { return add(u, v); }
inline f32x3 operator-(f32x3 u, f32x3 v) { return sub(u, v); }
inline f32x3 operator*(f32x3 u, f32x3 v) { return mul(u, v); }
inline f32x3 operator/(f32x3 u, f32x3 v) { return div(u, v); }
inline f32x4 operator+(f32x4 u, f32x4 v) { return add(u, v); }
inline f32x4 operator-(f32x4 u, f32x4 v) { return sub(u, v); }
inline f32x4 operator*(f32x4 u, f32x4 v) { return mul(u, v); }
inline f32x4 operator/(f32x4 u, f32x4 v) { return div(u, v); }

inline f32x2 operator+(f32x2 u, f32 c) { return add(u, c); }
inline f32x2 operator-(f32x2 u, f32 c) { return sub(u, c); }
inline f32x2 operator*(f32x2 u, f32 c) { return mul(u, c); }
inline f32x2 operator/(f32x2 u, f32 c) { return div(u, c); }
inline f32x3 operator+(f32x3 u, f32 c) { return add(u, c); }
inline f32x3 operator-(f32x3 u, f32 c) { return sub(u, c); }
inline f32x3 operator*(f32x3 u, f32 c) { return mul(u, c); }
inline f32x3 operator/(f32x3 u, f32 c) { return div(u, c); }
inline f32x4 operator+(f32x4 u, f32 c) { return add(u, c); }
inline f32x4 operator-(f32x4 u, f32 c) { return sub(u, c); }
inline f32x4 operator*(f32x4 u, f32 c) { return mul(u, c); }
inline f32x4 operator/(f32x4 u, f32 c) { return div(u, c); }

inline f32x2 operator+(f32 c, f32x2 v) { return add(c, v); }
inline f32x2 operator-(f32 c, f32x2 v) { return sub(c, v); }
inline f32x2 operator*(f32 c, f32x2 v) { return mul(c, v); }
inline f32x2 operator/(f32 c, f32x2 v) { return div(c, v); }
inline f32x3 operator+(f32 c, f32x3 v) { return add(c, v); }
inline f32x3 operator-(f32 c, f32x3 v) { return sub(c, v); }
inline f32x3 operator*(f32 c, f32x3 v) { return mul(c, v); }
inline f32x3 operator/(f32 c, f32x3 v) { return div(c, v); }
inline f32x4 operator+(f32 c, f32x4 v) { return add(c, v); }
inline f32x4 operator-(f32 c, f32x4 v) { return sub(c, v); }
inline f32x4 operator*(f32 c, f32x4 v) { return mul(c, v); }
inline f32x4 operator/(f32 c, f32x4 v) { return div(c, v); }

/*
	SIMD arithmetic operators
*/

inline __m256 operator+(__m256 u, __m256 v) { return add(u, v); }
inline __m256 operator-(__m256 u, __m256 v) { return sub(u, v); }
inline __m256 operator*(__m256 u, __m256 v) { return mul(u, v); }
inline __m256 operator/(__m256 u, __m256 v) { return div(u, v); }
inline m256x2 operator+(m256x2 u, m256x2 v) { return add(u, v); }
inline m256x2 operator-(m256x2 u, m256x2 v) { return sub(u, v); }
inline m256x2 operator*(m256x2 u, m256x2 v) { return mul(u, v); }
inline m256x2 operator/(m256x2 u, m256x2 v) { return div(u, v); }
inline m256x3 operator+(m256x3 u, m256x3 v) { return add(u, v); }
inline m256x3 operator-(m256x3 u, m256x3 v) { return sub(u, v); }
inline m256x3 operator*(m256x3 u, m256x3 v) { return mul(u, v); }
inline m256x3 operator/(m256x3 u, m256x3 v) { return div(u, v); }
inline m256x4 operator+(m256x4 u, m256x4 v) { return add(u, v); }
inline m256x4 operator-(m256x4 u, m256x4 v) { return sub(u, v); }
inline m256x4 operator*(m256x4 u, m256x4 v) { return mul(u, v); }
inline m256x4 operator/(m256x4 u, m256x4 v) { return div(u, v); }

inline m256x2 operator+(__m256 c, m256x2 v) { return add(c, v); }
inline m256x2 operator-(__m256 c, m256x2 v) { return sub(c, v); }
inline m256x2 operator*(__m256 c, m256x2 v) { return mul(c, v); }
inline m256x2 operator/(__m256 c, m256x2 v) { return div(c, v); }
inline m256x3 operator+(__m256 c, m256x3 v) { return add(c, v); }
inline m256x3 operator-(__m256 c, m256x3 v) { return sub(c, v); }
inline m256x3 operator*(__m256 c, m256x3 v) { return mul(c, v); }
inline m256x3 operator/(__m256 c, m256x3 v) { return div(c, v); }
inline m256x4 operator+(__m256 c, m256x4 v) { return add(c, v); }
inline m256x4 operator-(__m256 c, m256x4 v) { return sub(c, v); }
inline m256x4 operator*(__m256 c, m256x4 v) { return mul(c, v); }
inline m256x4 operator/(__m256 c, m256x4 v) { return div(c, v); }

inline m256x2 operator+(m256x2 u, __m256 c) { return add(u, c); }
inline m256x2 operator-(m256x2 u, __m256 c) { return sub(u, c); }
inline m256x2 operator*(m256x2 u, __m256 c) { return mul(u, c); }
inline m256x2 operator/(m256x2 u, __m256 c) { return div(u, c); }
inline m256x3 operator+(m256x3 u, __m256 c) { return add(u, c); }
inline m256x3 operator-(m256x3 u, __m256 c) { return sub(u, c); }
inline m256x3 operator*(m256x3 u, __m256 c) { return mul(u, c); }
inline m256x3 operator/(m256x3 u, __m256 c) { return div(u, c); }
inline m256x4 operator+(m256x4 u, __m256 c) { return add(u, c); }
inline m256x4 operator-(m256x4 u, __m256 c) { return sub(u, c); }
inline m256x4 operator*(m256x4 u, __m256 c) { return mul(u, c); }
inline m256x4 operator/(m256x4 u, __m256 c) { return div(u, c); }

inline __m256 operator+(__m256 u, f32 c) { return add(u, c); }
inline __m256 operator-(__m256 u, f32 c) { return sub(u, c); }
inline __m256 operator*(__m256 u, f32 c) { return mul(u, c); }
inline __m256 operator/(__m256 u, f32 c) { return div(u, c); }
inline m256x2 operator+(m256x2 u, f32 c) { return add(u, c); }
inline m256x2 operator-(m256x2 u, f32 c) { return sub(u, c); }
inline m256x2 operator*(m256x2 u, f32 c) { return mul(u, c); }
inline m256x2 operator/(m256x2 u, f32 c) { return div(u, c); }
inline m256x3 operator+(m256x3 u, f32 c) { return add(u, c); }
inline m256x3 operator-(m256x3 u, f32 c) { return sub(u, c); }
inline m256x3 operator*(m256x3 u, f32 c) { return mul(u, c); }
inline m256x3 operator/(m256x3 u, f32 c) { return div(u, c); }
inline m256x4 operator+(m256x4 u, f32 c) { return add(u, c); }
inline m256x4 operator-(m256x4 u, f32 c) { return sub(u, c); }
inline m256x4 operator*(m256x4 u, f32 c) { return mul(u, c); }
inline m256x4 operator/(m256x4 u, f32 c) { return div(u, c); }

inline __m256 operator+(f32 c, __m256 v) { return add(c, v); }
inline __m256 operator-(f32 c, __m256 v) { return sub(c, v); }
inline __m256 operator*(f32 c, __m256 v) { return mul(c, v); }
inline __m256 operator/(f32 c, __m256 v) { return div(c, v); }
inline m256x2 operator+(f32 c, m256x2 v) { return add(c, v); }
inline m256x2 operator-(f32 c, m256x2 v) { return sub(c, v); }
inline m256x2 operator*(f32 c, m256x2 v) { return mul(c, v); }
inline m256x2 operator/(f32 c, m256x2 v) { return div(c, v); }
inline m256x3 operator+(f32 c, m256x3 v) { return add(c, v); }
inline m256x3 operator-(f32 c, m256x3 v) { return sub(c, v); }
inline m256x3 operator*(f32 c, m256x3 v) { return mul(c, v); }
inline m256x3 operator/(f32 c, m256x3 v) { return div(c, v); }
inline m256x4 operator+(f32 c, m256x4 v) { return add(c, v); }
inline m256x4 operator-(f32 c, m256x4 v) { return sub(c, v); }
inline m256x4 operator*(f32 c, m256x4 v) { return mul(c, v); }
inline m256x4 operator/(f32 c, m256x4 v) { return div(c, v); }

/*
	Logical operators
*/

inline __m256 operator| (__m256 u, __m256 v) { return bitwise_or(u, v);  }
inline __m256 operator& (__m256 u, __m256 v) { return bitwise_and(u, v); }
inline __m256 operator||(__m256 u, __m256 v) { return bitwise_or(u, v);  }
inline __m256 operator&&(__m256 u, __m256 v) { return bitwise_and(u, v); }
inline __m256 operator^ (__m256 u, __m256 v) { return bitwise_xor(u, v); }
inline __m256 operator~ (__m256 u) { return bitwise_not(u); }
inline __m256 operator! (__m256 u) { return bitwise_not(u); }

inline __m256 operator-(__m256 u) { return signflip_ps(u); }
inline m256x2 operator-(m256x2 u) { return signflip_ps(u); }
inline m256x3 operator-(m256x3 u) { return signflip_ps(u); }
inline m256x4 operator-(m256x4 u) { return signflip_ps(u); }

inline __m256& operator|=(__m256& u, __m256 v) { return u = bitwise_or(u, v);  }
inline __m256& operator&=(__m256& u, __m256 v) { return u = bitwise_and(u, v); }
inline __m256& operator^=(__m256& u, __m256 v) { return u = bitwise_xor(u, v); }

template<typename T> inline T& operator+=(T& u, T v) { return u = add(u, v); }
template<typename T> inline T& operator-=(T& u, T v) { return u = sub(u, v); }
template<typename T> inline T& operator*=(T& u, T v) { return u = mul(u, v); }
template<typename T> inline T& operator/=(T& u, T v) { return u = div(u, v); }


inline f32 len(f32x2 u) { return sroot(dot(u, u)); }
inline f32 len(f32x3 u) { return sroot(dot(u, u)); }
inline f32 len(f32x4 u) { return sroot(dot(u, u)); }
inline __m256 len(m256x2 u) { return sroot(dot(u, u)); }
inline __m256 len(m256x3 u) { return sroot(dot(u, u)); }
inline __m256 len(m256x4 u) { return sroot(dot(u, u)); }

// template<typename T, typename S> 
// template<typename T> inline f32 len(T u) { return sroot(dot(u, u)); }
// template<typename T> inline __m256 len(T u) { return sroot(dot(u, u)); }


template<typename T, typename S> inline S lenSqrd(T u) { return dot(u, u); }
//template<typename T> inline T normalize(T v) { return mul(v, rcp(len(v))); }
//template<typename T> inline T normalize(T v) { return div(v, sroot(dot(v, v))); }
//template<typename T> inline T normalize(T v) { return mul(v, rsroot(dot(v, v))); }


// It seems like using rsroot and multiply instead of divide is not accurate enough to avoid artifacts
// Therefore reverting to using a straightforward implementation, although perhaps slower implementation
inline f32x2 normalize(f32x2 v) { return div(v, sroot(dot(v, v))); }
inline f32x3 normalize(f32x3 v) { return mul(v, 1.f / sroot(dot(v, v))); }
inline f32x4 normalize(f32x4 v) { return mul(v, 1.f / sroot(dot(v, v))); }


inline m256x2 fast_approx_normalize(m256x2 v) { return mul(v, rsroot(dot(v, v))); }
inline m256x3 fast_approx_normalize(m256x3 v) { return mul(v, rsroot(dot(v, v))); }
inline m256x4 fast_approx_normalize(m256x4 v) { return mul(v, rsroot(dot(v, v))); }

inline m256x2 normalize(m256x2 v) { return div(v, sroot(dot(v, v))); }
inline m256x3 normalize(m256x3 v) { return mul(v, set1_ps(1.f) / sroot(dot(v, v))); }
inline m256x4 normalize(m256x4 v) { return mul(v, set1_ps(1.f) / sroot(dot(v, v))); }


template<typename T, typename S> inline T lerp(T u, T v, S x) { return add(u, mul(x, sub(v, u))); }
template<typename T> inline T proj(T u, T v) { return mul((dot(u, v) / dot(v, v)), v); }
template<typename T> inline T rjec(T u, T v) { return sub(u, proj(u, v)); }
template<typename T> inline T rflc(T u, T v) { return sub(mul(2., proj(u, v)), u); }

inline f32x3 cross(f32x3 u, f32x3 v) { return { u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x }; }
//inline m256x3 cross(m256x3 u, m256x3 v) { return { u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x }; }
inline m256x3 cross(m256x3 u, m256x3 v)
{ 
	return
	{ 
		fmsub(u.y, v.z, u.z * v.y),
		fmsub(u.z, v.x, u.x * v.z),
		fmsub(u.x, v.y, u.y * v.x)
	};
}

//https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/refract.xhtml
inline m256x3 rfrct(m256x3 v /*unit len*/, m256x3 n /*unit len*/, __m256 ior)
{
	__m256 vdotn = dot(v, n);
	__m256 k = fnmadd(ior, ior * fnmadd(vdotn, vdotn, set1_ps(1.f)), set1_ps(1.f));
	m256x3 Result = fmsub(ior, v, fmadd(ior, vdotn, sroot(k)) * n);
	__m256 RetZeroCond = (k < set1_ps(0.f));
	Result = blend3_ps(Result, set1x3_ps(0.f, 0.f, 0.f), RetZeroCond);
	return Result;
}

/*

	Integer intrinsics

*/
inline __m256i set1_epi(i32 a) { return _mm256_set1_epi32(a); }

inline __m256i xor(__m256i a, __m256i b) { return _mm256_xor_si256(a, b); }
inline __m256i  or(__m256i a, __m256i b) { return  _mm256_or_si256(a, b); }
inline __m256i and(__m256i a, __m256i b) { return _mm256_and_si256(a, b); }

inline __m256i xor(__m256i a, i32 b) { return _mm256_xor_si256(a, set1_epi(b)); }
inline __m256i  or(__m256i a, i32 b) { return  _mm256_or_si256(a, set1_epi(b)); }
inline __m256i and(__m256i a, i32 b) { return _mm256_and_si256(a, set1_epi(b)); }

inline __m256i xor(i32 a, __m256i b) { return _mm256_xor_si256(set1_epi(a), b); }
inline __m256i  or(i32 a, __m256i b) { return  _mm256_or_si256(set1_epi(a), b); }
inline __m256i and(i32 a, __m256i b) { return _mm256_and_si256(set1_epi(a), b); }

inline __m256i operator^(__m256i u, __m256i v) { return xor(u, v); }
inline __m256i operator|(__m256i u, __m256i v) { return  or(u, v); }
inline __m256i operator&(__m256i u, __m256i v) { return and(u, v); }

inline __m256i operator^(__m256i u, i32 v) { return xor(u, v); }
inline __m256i operator|(__m256i u, i32 v) { return  or(u, v); }
inline __m256i operator&(__m256i u, i32 v) { return and(u, v); }

inline __m256i operator^(i32 u, __m256i v) { return xor(u, v); }
inline __m256i operator|(i32 u, __m256i v) { return  or(u, v); }
inline __m256i operator&(i32 u, __m256i v) { return and(u, v); }

inline __m256i mul(__m256i a, __m256i b) { return _mm256_mullo_epi32(a, b); }
inline __m256i div(__m256i a, __m256i b) { return   _mm256_div_epi32(a, b); }
inline __m256i add(__m256i a, __m256i b) { return   _mm256_add_epi32(a, b); }
inline __m256i sub(__m256i a, __m256i b) { return   _mm256_sub_epi32(a, b); }

inline __m256i mul(__m256i a, i32 b) { return _mm256_mullo_epi32(a, set1_epi(b)); }
inline __m256i div(__m256i a, i32 b) { return   _mm256_div_epi32(a, set1_epi(b)); }
inline __m256i add(__m256i a, i32 b) { return   _mm256_add_epi32(a, set1_epi(b)); }
inline __m256i sub(__m256i a, i32 b) { return   _mm256_sub_epi32(a, set1_epi(b)); }

inline __m256i mul(i32 a, __m256i b) { return _mm256_mullo_epi32(set1_epi(a), b); }
inline __m256i div(i32 a, __m256i b) { return   _mm256_div_epi32(set1_epi(a), b); }
inline __m256i add(i32 a, __m256i b) { return   _mm256_add_epi32(set1_epi(a), b); }
inline __m256i sub(i32 a, __m256i b) { return   _mm256_sub_epi32(set1_epi(a), b); }

inline __m256i operator+(__m256i u, __m256i v) { return add(u, v); }
inline __m256i operator-(__m256i u, __m256i v) { return sub(u, v); }
inline __m256i operator*(__m256i u, __m256i v) { return mul(u, v); }
inline __m256i operator/(__m256i u, __m256i v) { return div(u, v); }
inline __m256i operator+(__m256i u, i32 v) { return add(u, v); }
inline __m256i operator-(__m256i u, i32 v) { return sub(u, v); }
inline __m256i operator*(__m256i u, i32 v) { return mul(u, v); }
inline __m256i operator/(__m256i u, i32 v) { return div(u, v); }
inline __m256i operator+(i32 u, __m256i v) { return add(u, v); }
inline __m256i operator-(i32 u, __m256i v) { return sub(u, v); }
inline __m256i operator*(i32 u, __m256i v) { return mul(u, v); }
inline __m256i operator/(i32 u, __m256i v) { return div(u, v); }

	/*
		Shift operators
	*/
inline __m256i srli(__m256i a, const i32 imm8) { return _mm256_srli_epi32(a, imm8); }
inline __m256i slli(__m256i a, const i32 imm8) { return _mm256_slli_epi32(a, imm8); }

inline __m256i operator>>(__m256i u, const i32 imm8) { return srli(u, imm8); }
inline __m256i operator<<(__m256i u, const i32 imm8) { return slli(u, imm8); }

inline __m256i blend_epi(__m256i a, __m256i b, __m256i mask) { return _mm256_blendv_epi8(a, b, mask); }

inline __m256i bitcast_epi(__m256 a) { return _mm256_castps_si256(a); }
inline __m256 bitcast_ps(__m256i a) { return _mm256_castsi256_ps(a); }
inline __m256i to_epi32(__m256 a) { return _mm256_cvtps_epi32(a); };
inline __m256 to_ps(__m256i a) { return _mm256_cvtepi32_ps(a); };
