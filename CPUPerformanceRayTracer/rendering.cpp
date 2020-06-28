#include "rendering.h"

#include <limits>

/*
Performance notes for intel core i7 6700K Skylake S

4 Cores, Hypertreading ( 8 Threads )
4 GHz

Cache
L1$		256 KiB
	L1I$	128 KiB	4x32 KiB	8-way set associative
	L1D$	128 KiB	4x32 KiB	8-way set associative	write-back
L2$	1 MiB
		4x256 KiB	4-way set associative	write-back
L3$	8 MiB
		4x2 MiB	 							write-back


Cache line sizes are 64 bytes

A full a float is 4 bytes
A SIMD register float lane count of 8 is then 4 * 8 = 32 Byte

A SIMD register is then half a CacheLine

32 KiB = 32 * 1024 B L1 data cache per core is then 512 cache lines, or 1024 SIMD registers

Given 8-way associativity and 64 Byte cache lines, there are 512Bytes in each set
this gives 64 sets in L1 data cache

Tag(64Bit - IndexBits - OffsetBits) : Index(log2(NumSets)) : Offset (log2(LineByteSize))
[ 54 ] : [ 6 bit ] : [ 6 bit ]

1920x1080x3 x 4B = 24883200B  = 24300KiB Bytes buffer
or just under 24 MiB

Estimate cache latencies:
L1 cache access latency: 4 cycles
L2 cache access latency: 11 cycles
L3 cache access latency: 39 cycles
Main memory access latency: 107 cycles

*/

struct Ray8
{
	m256x3 Origin;
	m256x3 Direction;
};

struct Sphere8
{
	m256x3 Center;
	__m256 Radius;
};

__m256 RaySphereIntersects(Ray8 ray, Sphere8 sphere, __m256& t0)
{
	m256x3 L = sphere.Center - ray.Origin;
	__m256 tca = dot(L, ray.Direction);
	__m256 d2 = dot(L, L) - tca * tca;
	__m256 r2 = sphere.Radius * sphere.Radius;
	__m256 ReturnMask = d2 <= r2; // return false;
	__m256 thc = sroot(abs_ps(r2 - d2));
	t0 = blend_ps(t0, tca - thc, ReturnMask);
	__m256 t1 = tca + thc;
	__m256 negmask = t0 >= set1_ps(0.f);
	t0 = blend_ps(t1, t0, negmask);
	ReturnMask = bitwise_and(ReturnMask, t0 >= set1_ps(0.f));
	return ReturnMask;
}

m256x3 CastRay(Ray8 r, Sphere8 s)
{
	m256x3 Result;
	__m256 dst = set1_ps(std::numeric_limits<float>::max());
	__m256 IntersectionMask = RaySphereIntersects(r, s, dst);
	Result.x = blend_ps(set1_ps(0.8f), set1_ps(0.3f), IntersectionMask);
	Result.y = blend_ps(set1_ps(0.7f), set1_ps(0.4f), IntersectionMask);
	Result.z = blend_ps(set1_ps(0.2f), set1_ps(0.4f), IntersectionMask);
	return Result;
}

void Render(f32* BufferOut, i32 BufferWidth, i32 BufferHeight, i32 NumChannels)
{

	i32 YHeight = BufferHeight;
	i32 XWidth = (BufferWidth / LANE_COUNT);

	Sphere8 sphere = { set1x3_ps(-3.f, 0.f, -16.f),  set1_ps(4.f) };
	f32 tanFov = tan(90 / 2.f); // 1
	__m256 y_const_term   = set1_ps(-tanFov / (f32)BufferHeight + tanFov);
	__m256 y_const_factor = set1_ps(-tanFov * 2.f / (f32)BufferHeight);
	__m256 x_const_term   = set1_ps((tanFov / (f32)BufferHeight) * (1.f - BufferWidth));
	__m256 x_const_factor = set1_ps(2.f * tanFov / (f32)BufferHeight);

	f32* BufferPos = BufferOut;

	m256x3 origin = set1x3_ps(0.f, 0.f, 0.f);
	/*{
		set1_ps(0.f),
		set1_ps(0.f),
		set1_ps(0.f)
	};*/
	
	__m256 XLaneOffsets = set_ps(7.f, 6.f, 5.f, 4.f, 3.f, 2.f, 1.f, 0.f);

	for (i32 Y = 0; Y < YHeight; Y++)
	{
		__m256 y = set1_ps((f32)Y) * y_const_factor + y_const_term;
		for (i32 X = 0; X < XWidth; X++)
		{
			__m256 x = (set1_ps((f32)X * LANE_COUNT) + XLaneOffsets) * x_const_factor + x_const_term;
			m256x3 udir = { x, y, set1_ps(-1.f) };
			m256x3 dir = normalize(udir);
			m256x3 color = CastRay({ origin , dir }, sphere);

			non_temporal_store(BufferPos, color.x);
			BufferPos += LANE_COUNT;
			non_temporal_store(BufferPos, color.y);
			BufferPos += LANE_COUNT;
			non_temporal_store(BufferPos, color.z);
			BufferPos += LANE_COUNT;
		}
	}
}