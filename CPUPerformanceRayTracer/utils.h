#pragma once

#include "stdint.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

i64 GetPerformanceCounter();
f64 GetPerformanceCounterIntervalSeconds(i64 Start, i64 End);
void InitializeQueryPerformanceCounter();
i64 GetPerformanceCounterFrequency();

f64 GetRefreshRate();