#pragma once

#include "utils.h"

#include <cmath>

#include <emmintrin.h>
#include <immintrin.h>

#define LANE_COUNT 8

struct f32x2 { f32 x, y; };
struct f32x3 { f32 x, y, z; };
struct f32x4 { f32 x, y, z, w; };

//f32x3& operator=(f32 const& rhs);
//f32x3& f32x3::operator=(f32 const& rhs) { x = rhs; y = rhs, z = rhs; }


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
inline f32x2 mul(f32x2 u, f32 c)   { return { u.x * c, u.y * c}; }
inline f32x3 mul(f32x3 u, f32 c)   { return { u.x * c, u.y * c, u.z * c}; }
inline f32x4 mul(f32x4 u, f32 c)   { return { u.x * c, u.y * c, u.z * c, u.w * c}; }
inline f32x2 mul(f32 c, f32x2 u)   { return { u.x * c, u.y * c }; }
inline f32x3 mul(f32 c, f32x3 u)   { return { u.x * c, u.y * c, u.z * c }; }
inline f32x4 mul(f32 c, f32x4 u)   { return { u.x * c, u.y * c, u.z * c, u.w * c }; }
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

inline m256x2 mul(m256x2 v, __m256 c) { return{ _mm256_mul_ps(v.x, c), _mm256_mul_ps(v.y, c) }; }
inline m256x3 mul(m256x3 v, __m256 c) { return{ _mm256_mul_ps(v.x, c), _mm256_mul_ps(v.y, c), _mm256_mul_ps(v.z, c) }; }
inline m256x4 mul(m256x4 v, __m256 c) { return{ _mm256_mul_ps(v.x, c), _mm256_mul_ps(v.y, c), _mm256_mul_ps(v.z, c), _mm256_mul_ps(v.w, c) }; }

inline m256x2 mul(__m256 c, m256x2 v) { return{ _mm256_mul_ps(v.x, c), _mm256_mul_ps(v.y, c) }; }
inline m256x3 mul(__m256 c, m256x3 v) { return{ _mm256_mul_ps(v.x, c), _mm256_mul_ps(v.y, c), _mm256_mul_ps(v.z, c) }; }
inline m256x4 mul(__m256 c, m256x4 v) { return{ _mm256_mul_ps(v.x, c), _mm256_mul_ps(v.y, c), _mm256_mul_ps(v.z, c), _mm256_mul_ps(v.w, c) }; }

inline __m256 dot(m256x2 u, m256x2 v) { return _mm256_fmadd_ps(u.x, v.x,                                                     _mm256_mul_ps(u.y, v.y)); }
inline __m256 dot(m256x3 u, m256x3 v) { return _mm256_fmadd_ps(u.x, v.x, _mm256_fmadd_ps(u.y, v.y,                           _mm256_mul_ps(u.z, v.z))); }
inline __m256 dot(m256x4 u, m256x4 v) { return _mm256_fmadd_ps(u.x, v.x, _mm256_fmadd_ps(u.y, v.y, _mm256_fmadd_ps(u.z, v.z, _mm256_mul_ps(u.w, v.w)))); }

inline m256x2 fmadd(m256x2 a, m256x2 b, m256x2 c) { return{ _mm256_fmadd_ps(a.x, b.x, c.x), _mm256_fmadd_ps(a.y, b.y, c.y) }; }
inline m256x3 fmadd(m256x3 a, m256x3 b, m256x3 c) { return{ _mm256_fmadd_ps(a.x, b.x, c.x), _mm256_fmadd_ps(a.y, b.y, c.y), _mm256_fmadd_ps(a.z, b.z, c.z) }; }
inline m256x4 fmadd(m256x4 a, m256x4 b, m256x4 c) { return{ _mm256_fmadd_ps(a.x, b.x, c.x), _mm256_fmadd_ps(a.y, b.y, c.y), _mm256_fmadd_ps(a.z, b.z, c.z), _mm256_fmadd_ps(a.w, b.w, c.w) }; }

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
inline __m256 min_ps(__m256 a, __m256 b) { return _mm256_min_ps(a, b); }

inline __m256 negate_ps(__m256 a) { return _mm256_xor_ps(a, _mm256_set1_ps(-0.f)); }
inline __m256 abs_ps(__m256 a) { return _mm256_andnot_ps(_mm256_set1_ps(-0.f), a); }

inline __m256 signflip_ps(__m256 a) { return _mm256_xor_ps(_mm256_set1_ps(-0.f), a); }

inline __m256 round_ceil(__m256 a)	  { return _mm256_round_ps(a, _MM_FROUND_CEIL); }
inline __m256 round_floor(__m256 a)   { return _mm256_round_ps(a, _MM_FROUND_FLOOR); }
inline __m256 round_nearest(__m256 a) { return _mm256_round_ps(a, _MM_FROUND_NINT); }

inline __m256 fract(__m256 a) { return sub(a, round_floor(a)); }

inline __m256 clamp(__m256 x, __m256 minval, __m256 maxval) { return min_ps(max_ps(x, minval), maxval); }



/*
	Shift operators
*/

/*
	shuffle operators
*/

/*
	Algebraic and trancendental functions
	// Some require SVML, and needs MS Visual Studio 2019
*/


inline __m256 rcp(__m256 a)   { return _mm256_rcp_ps(a); } // reciprocal
inline __m256 sroot(__m256 a)  { return _mm256_sqrt_ps(a); } // square root
inline __m256 rsroot(__m256 a) { return _mm256_rsqrt_ps(a); } // reciprocal square root

inline __m256 sin_ps(__m256 a) { return _mm256_sin_ps(a); }
inline __m256 cos_ps(__m256 a) { return _mm256_cos_ps(a); }
inline __m256 tan_ps(__m256 a) { return _mm256_tan_ps(a); }
inline __m256 asin_ps(__m256 a) { return _mm256_asin_ps(a); }
inline __m256 acos_ps(__m256 a) { return _mm256_acos_ps(a); }
inline __m256 atan_ps(__m256 a) { return _mm256_atan_ps(a); }
inline __m256 atan2_ps(__m256 a, __m256 b) { return _mm256_atan2_ps(a, b); }

inline __m256 exp_ps(__m256 a) { return _mm256_exp_ps(a); }
inline __m256 log_ps(__m256 a) { return _mm256_log_ps(a); }
inline __m256 pow_ps(__m256 a, __m256 b) { return _mm256_pow_ps(a, b); }

inline f32 rcp(f32 a) { return 1.f / a;    } // reciprocal
inline f32 sroot(f32 a) { return sqrtf(a); } // square root
inline f32 rsroot(f32 a) { return 1.f/sqrtf(a); } // reciprocal square root

/*
	Loads and stores
*/
// __m256 _mm256_set1_ps(float a) // broadcast single float to all lanes
// __m256 _mm256_set_ps(float e7, float e6, float e5, float e4, float e3, float e2, float e1, float e0) // Set all lanes
  
inline __m256 set1_ps(f32 a)						{ return   _mm256_set1_ps(a); }
inline m256x2 set1x2_ps(f32 x, f32 y)				{ return { _mm256_set1_ps(x), _mm256_set1_ps(y) }; }
inline m256x3 set1x3_ps(f32 x, f32 y, f32 z)		{ return { _mm256_set1_ps(x), _mm256_set1_ps(y), _mm256_set1_ps(z) }; }
inline m256x4 set1x4_ps(f32 x, f32 y, f32 z, f32 w)	{ return { _mm256_set1_ps(x), _mm256_set1_ps(y), _mm256_set1_ps(z), _mm256_set1_ps(w),}; }

inline __m256 set_ps(f32 e7, f32 e6, f32 e5, f32 e4, f32 e3, f32 e2, f32 e1, f32 e0) { return _mm256_set_ps(e7, e6, e5, e4, e3, e2, e1, e0); }

inline __m256 load_ps(f32 const* mem_addr) { return _mm256_load_ps(mem_addr); }
inline __m256 maskload_ps(f32 const* mem_addr, __m256i mask) { return _mm256_maskload_ps(mem_addr, mask); }
inline void store_ps(f32* mem_addr, __m256 a) { _mm256_store_ps(mem_addr, a); }
inline void maskstore_ps(f32* mem_addr, __m256i mask, __m256 a) { _mm256_maskstore_ps(mem_addr, mask, a); }

inline __m256 blend_ps(__m256 a, __m256 b, __m256 mask) { return  _mm256_blendv_ps(a, b, mask); }
inline __m256 if_then_else(__m256 cond, __m256 thenDo, __m256 elseDo) { return  _mm256_blendv_ps(elseDo, thenDo, cond); } // switcheroo of blend_ps to match C ternary operator

inline m256x2 blend2_ps(m256x2 a, m256x2 b, __m256 mask) { return  { _mm256_blendv_ps(a.x, b.x, mask), _mm256_blendv_ps(a.y, b.y, mask) }; }
inline m256x3 blend3_ps(m256x3 a, m256x3 b, __m256 mask) { return  { _mm256_blendv_ps(a.x, b.x, mask), _mm256_blendv_ps(a.y, b.y, mask), _mm256_blendv_ps(a.z, b.z, mask) }; }
inline m256x4 blend4_ps(m256x4 a, m256x4 b, __m256 mask) { return  { _mm256_blendv_ps(a.x, b.x, mask), _mm256_blendv_ps(a.y, b.y, mask), _mm256_blendv_ps(a.z, b.z, mask), _mm256_blendv_ps(a.w, b.w, mask) }; }


inline bool any_set(__m256 mask) { return _mm256_movemask_ps(mask) > 0; }
inline bool all_set(__m256 mask) { return _mm256_movemask_ps(mask) == 0b11111111; }
inline bool none_set(__m256 mask) { return _mm256_movemask_ps(mask) == 0; }

/*
	Random number generation
	//See https://software.intel.com/content/www/us/en/develop/documentation/cpp-compiler-developer-guide-and-reference/top/compiler-reference/intrinsics/intrinsics-for-the-short-vector-random-number-generator-library.html
*/

inline void non_temporal_store(f32 * mem_addr, __m256 a) { _mm256_stream_ps(mem_addr, a); };

/*
	Operator overloads and functions
*/
//
//template<typename ToType>
//inline ToType Convert(f32 FromValue) { return (ToType)FromValue; }
//
//template<>
//inline __m256 Convert<__m256>(f32 FromValue) { return _mm256_set1_ps(FromValue); }

template<typename T> inline T operator==(T u, T v) { return compare_eq(u, v); }
template<typename T> inline T operator!=(T u, T v) { return compare_ne(u, v); }
template<typename T> inline T operator <(T u, T v) { return compare_lt(u, v); }
template<typename T> inline T operator<=(T u, T v) { return compare_le(u, v); }
template<typename T> inline T operator >(T u, T v) { return compare_gt(u, v); }
template<typename T> inline T operator>=(T u, T v) { return compare_ge(u, v); }

template<typename T> inline T operator+(T u, T v) { return add(u, v); }
template<typename T> inline T operator-(T u, T v) { return sub(u, v); }
template<typename T> inline T operator*(T u, T v) { return mul(u, v); }
template<typename T> inline T operator/(T u, T v) { return div(u, v); }
template<typename T, typename S> inline T operator*(T v, S c) { return mul(c, v); }
template<typename T, typename S> inline T operator*(S c, T v) { return mul(c, v); }/*
template<typename T> inline T operator*(f32 c, T v) { return mul(c, v); }
template<typename T> inline T operator*(T v, f32 c) { return mul(c, v); }

template<typename T> inline T operator*(__m256 c, T v) { return mul(c, v); }
template<typename T> inline T operator*(T v, __m256 c) { return mul(c, v); }*/

inline __m256 operator|(__m256 u, __m256 v) { return bitwise_or(u, v); }
inline __m256 operator&(__m256 u, __m256 v) { return bitwise_and(u, v); }
inline __m256 operator||(__m256 u, __m256 v) { return bitwise_or(u, v); }
inline __m256 operator&&(__m256 u, __m256 v) { return bitwise_and(u, v); }
inline __m256 operator^(__m256 u, __m256 v) { return bitwise_xor(u, v); }
inline __m256 operator~(__m256 u) { return bitwise_not(u); }
inline __m256 operator!(__m256 u) { return bitwise_not(u); }

inline __m256 operator-(__m256 u) { return signflip_ps(u); }


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
template<typename T> inline T normalize(T v) { return mul(v, rsroot(dot(v, v))); }

template<> inline m256x2 normalize(m256x2 v) { return mul(v, set1_ps(1.) / sroot(dot(v, v))); }
template<> inline m256x3 normalize(m256x3 v) { return mul(v, set1_ps(1.) / sroot(dot(v, v))); }
template<> inline m256x4 normalize(m256x4 v) { return mul(v, set1_ps(1.) / sroot(dot(v, v))); }

//template<> inline m256x3 normalize(m256x3 v) {
//
//	m256x3 result;
//	for (i32 lane = 0; lane < 8; lane++)
//	{
//		f32 x = v.x.m256_f32[lane];
//		f32 y = v.y.m256_f32[lane];
//		f32 z = v.z.m256_f32[lane];
//		f32 l2norm = sqrt(x * x + y * y + z * z);
//		result.x.m256_f32[lane] = x / l2norm;
//		result.y.m256_f32[lane] = y / l2norm;
//		result.z.m256_f32[lane] = z / l2norm;
//	}
//	return result;
//}


template<typename T, typename S> inline T lerp(T u, T v, S x) { return add(u, mul(x, sub(v, u))); }
template<typename T> inline T proj(T u, T v) { return mul((dot(u, v) / dot(v, v)), v); }
template<typename T> inline T rjec(T u, T v) { return sub(u, proj(u, v)); }
template<typename T> inline T rflc(T u, T v) { return sub(mul(2., proj(u, v)), u); }

inline f32x3 cross(f32x3 u, f32x3 v) { return { u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x }; }
inline m256x3 cross(m256x3 u, m256x3 v) { return { u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x }; }

template<typename T>
inline T rfrct(T v /*unit len*/, T n /*unit len*/, float ior)
{
	f32 vdotn = dot(n, v);
	f32 k = 1.f - ior * ior * (1.F - vdotn * vdotn);
	if (k < 0.0) return{ 0.f };
	return sub(mul(ior, v), mul((ior * vdotn + sqrt(k)), n));
}


// TODOs:

// set / broadcast for vector component types, m256x3 etc
// conditional move based on mask for vector component types

