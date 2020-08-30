#pragma once
#include "utils.h"

#include <intrin.h>
#include <immintrin.h>

inline u32 NumFactorsOfTwo(u32 n) { return n == 0 ? 0 : _tzcnt_u32(n); }
inline u64 NumFactorsOfTwo(u64 n) { return n == 0 ? 0 : _tzcnt_u64(n); }
inline i32 NumFactorsOfTwo(i32 n) { return NumFactorsOfTwo(n & 0x7FFFFFF); }
inline i64 NumFactorsOfTwo(i64 n) { return NumFactorsOfTwo(n & 0x7FFFFFFFFFFFFFF); }

u32 RoundUpToPowerOfTwo(u32 n)
{
	if (n == 0U) return 1U;
	u32 HighestBit = 0x80000000U >> _lzcnt_u32(n);
	return HighestBit << (u32)(HighestBit < n);
}
u64 RoundUpToPowerOfTwo(u64 n)
{
	if (n == 0U) return 1U;
	u64 HighestBit = 0x8000000000000000ULL >> _lzcnt_u64(n);
	return HighestBit << (u32)(HighestBit < n);
}

inline u8  PopulationCount( u8 n) { return _mm_popcnt_u32((u32)n); }
inline u16 PopulationCount(u16 n) { return _mm_popcnt_u32((u32)n); }
inline u32 PopulationCount(u32 n) { return _mm_popcnt_u32(n); }
inline u64 PopulationCount(u64 n) { return _mm_popcnt_u64(n); }

inline i8  PopulationCount( i8 n) { return _mm_popcnt_u32((u32)(u8)n); }
inline i16 PopulationCount(i16 n) { return _mm_popcnt_u32((u32)(u16)n); }
inline i32 PopulationCount(i32 n) { return _mm_popcnt_u32(n); }
inline i64 PopulationCount(i64 n) { return _mm_popcnt_u64(n); }

