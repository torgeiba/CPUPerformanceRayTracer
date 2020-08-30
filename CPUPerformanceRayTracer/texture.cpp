#pragma once

#include "texture.h"

f32x3 TexelFetch(texture texture, i32 Row, i32 Col)
{
	f32* texelPtr = texture.Data + ((u64)texture.Components * (Row * (u64)texture.Width + Col));
	f32x3 Result;
	Result.x = texelPtr[0];
	Result.y = texelPtr[1];
	Result.z = texelPtr[2];
	return Result;
}

static inline m256x3 GatherRGB(const f32* const base_addr, __m256i Indices)
{
	__m256i GreenChannelOffset = _mm256_set1_epi32(1);
	__m256i BlueChannelOffset = _mm256_set1_epi32(2);
	m256x3 Result = m256x3
	{
		_mm256_i32gather_ps(base_addr, Indices, 4),
		_mm256_i32gather_ps(base_addr, _mm256_add_epi32(Indices, GreenChannelOffset), 4),
		_mm256_i32gather_ps(base_addr, _mm256_add_epi32(Indices, BlueChannelOffset), 4)
	};
	return Result;
}

m256x3 TexelFetchGather(texture texture, __m256i Rows, __m256i Cols)
{
	__m256i RowOffset     = _mm256_mullo_epi32(Rows, _mm256_set1_epi32(texture.Width));
	__m256i PixelOffsets  = _mm256_add_epi32(Cols, RowOffset);
	__m256i NumComponents = _mm256_set1_epi32(texture.Components);
	__m256i Indices       = _mm256_mullo_epi32(NumComponents, PixelOffsets);

	const i32 scale = sizeof(f32);
	__m256i GreenChannelOffset = _mm256_set1_epi32(1);
	__m256i BlueChannelOffset = _mm256_set1_epi32(2);
	m256x3 Result = GatherRGB(texture.Data, Indices);
	return Result;
}

m256x3 TexelSampleBilinear(texture texture, m256x2 UVs)
{
	const i32 scale = sizeof(f32);
	__m256i GreenChannelOffset = _mm256_set1_epi32(1);
	__m256i BlueChannelOffset = _mm256_set1_epi32(2);
	const f32* const base_addr = (f32*)texture.Data;

	__m256 Row = UVs.y * (f32)(texture.Height - 1);
	__m256 Col = UVs.x * (f32)(texture.Width  - 1);
	__m256 Row0 = round_floor(Row);
	__m256 Row1 = round_ceil( Row);
	__m256 Col0 = round_floor(Col);
	__m256 Col1 = round_ceil( Col);

	__m256 dV = Row - Row0;
	__m256 dU = Col - Col0;

	__m256i I00 = _mm256_cvtps_epi32(3.0f * (Col0 + Row0 * (f32)(texture.Width)));
	__m256i I10 = _mm256_cvtps_epi32(3.0f * (Col1 + Row0 * (f32)(texture.Width)));
	__m256i I01 = _mm256_cvtps_epi32(3.0f * (Col0 + Row1 * (f32)(texture.Width)));
	__m256i I11 = _mm256_cvtps_epi32(3.0f * (Col1 + Row1 * (f32)(texture.Width)));

	m256x3 C00 = GatherRGB(base_addr, I00);
	m256x3 C10 = GatherRGB(base_addr, I10);
	m256x3 C01 = GatherRGB(base_addr, I01);
	m256x3 C11 = GatherRGB(base_addr, I11);

	m256x3 C0 = lerp(C00, C10, dU);
	m256x3 C1 = lerp(C01, C11, dU);
	m256x3 C = lerp(C0, C1, dV);
	return C;
}

f32x3 EquirectangularTextureSample(texture texture, f32x3 Direction)
{
	f32x2 invAtan = f32x2{ 0.1591f, 0.3183f };
	f32x2 uv = f32x2{ atan2(Direction.z, Direction.x), asin(Direction.y) };
	uv *= invAtan;
	uv = uv + 0.5f;
	i32 Row = (i32)(uv.y * texture.Height);
	i32 Col = (i32)(uv.x * texture.Width);
	return TexelFetch(texture, Row, Col);
}



m256x3 EquirectangularTextureSample(texture texture, m256x3 Directions)
{
	m256x3 Result;
	for (u32 lane = 0; lane < LANE_COUNT; lane++)
	{
		f32x3 Direction;
		Direction.x = Directions.x.m256_f32[lane];
		Direction.y = Directions.y.m256_f32[lane];
		Direction.z = Directions.z.m256_f32[lane];

		f32x2 invAtan = f32x2{ 0.1591f, 0.3183f };
		f32x2 uv = f32x2{ atan2(Direction.z, Direction.x), asin(Direction.y) };
		uv *= invAtan;
		uv = uv + 0.5f;
		uv.x -= (f32)(i32)(uv.x);
		uv.y -= (f32)(i32)(uv.y);

		// Todo: this might not be necessary, use to use texture.height instead of texture.height-1 etc,
		// in the assumption uv coords were in range [0, 1). but they should in fact be [0, 1], so should use height-1
		// when converting to rows and cols, see Gather version
		if (uv.x >= 0.f && uv.x < 1.f && uv.y >= 0.f && uv.y < 1.f)
		{
			i32 Row = (i32)(uv.y * (texture.Height-1));
			i32 Col = (i32)(uv.x * (texture.Width -1));
			f32x3 rgb = TexelFetch(texture, Row, Col);
			Result.x.m256_f32[lane] = rgb.x;
			Result.y.m256_f32[lane] = rgb.y;
			Result.z.m256_f32[lane] = rgb.z;
		}
		else
		{
			Result.x.m256_f32[lane] = 0.f;
			Result.y.m256_f32[lane] = 0.f;
			Result.z.m256_f32[lane] = 0.f;
		}

	}
	return Result;
}

m256x3 EquirectangularTextureSampleGather(texture texture, m256x3 Directions)
{
	m256x3 Result;
	{

		m256x2 invAtan = m256x2{ set1_ps(0.1591f), set1_ps(0.3183f) };
		m256x2 uvs = m256x2
		{
			atan2_ps(Directions.z, Directions.x),
			asin_ps(Directions.y)
		};
		
		uvs *= invAtan;
		uvs = uvs + 0.5f;
		uvs.x -= round_floor(uvs.x);
		uvs.y -= round_floor(uvs.y);
		uvs.x = saturate(uvs.x);
		uvs.y = saturate(uvs.y);
		__m256i Rows = _mm256_cvtps_epi32(uvs.y * ((f32)texture.Height-1));
		__m256i Cols = _mm256_cvtps_epi32(uvs.x * ((f32)texture.Width-1));
		m256x3 rgb = TexelFetchGather(texture, Rows, Cols);
		Result = rgb;
	}
	return Result;
}

m256x3 EquirectangularTextureSampleBilinear(texture texture, m256x3 Directions)
{
	m256x3 Result;
	{

		m256x2 invAtan = m256x2{ set1_ps(0.1591f), set1_ps(0.3183f) };
		m256x2 uvs = m256x2
		{
			atan2_ps(Directions.z, Directions.x),
			asin_ps(Directions.y)
		};

		uvs *= invAtan;
		uvs = uvs + 0.5f;
		uvs.x -= round_floor(uvs.x);
		uvs.y -= round_floor(uvs.y);
		uvs.x = saturate(uvs.x);
		uvs.y = saturate(uvs.y);
		m256x3 rgb = TexelSampleBilinear(texture, uvs);
		Result = rgb;
	}
	return Result;
}


void BilinearResampleRGB32(i32 InWidth, i32 InHeight, i32 OutWidth, i32 OutHeight, f32x3* InBuffer, f32x3* OutBuffer)
{
	for (i32 OutRow = 0; OutRow < OutHeight; OutRow++)
	{
		f32 V  = (OutRow + 0.5f) / OutHeight;
		i32 V0 = (i32)(V * InHeight);
		i32 V1 = (i32)(V * InHeight + 1.f);

		for (i32 OutCol = 0; OutCol < OutWidth; OutCol++)
		{
			f32 U = (OutCol + 0.5f) / OutWidth;

			i32 U0 = (i32)(U * InWidth);
			i32 U1 = (i32)(U * InWidth + 1.f);

			i32 I00 = U0 + V0 * (InWidth-1);
			i32 I10 = U1 + V0 * (InWidth-1);
			i32 I01 = U0 + V1 * (InWidth-1);
			i32 I11 = U1 + V1 * (InWidth-1);

			f32x3 C00 = InBuffer[I00];
			f32x3 C10 = InBuffer[I10];
			f32x3 C01 = InBuffer[I01];
			f32x3 C11 = InBuffer[I11];

			f32x3 C0 = lerp(C00, C10, U - U0);
			f32x3 C1 = lerp(C01, C11, U - U0);
			f32x3 C  = lerp(C0,   C1, V - V0);

			OutBuffer[OutRow + OutCol * OutWidth] = C;
		}
	}
}

void BilinearResampleRGB32(i32 InWidth, i32 InHeight, i32 OutWidth, i32 OutHeight, m256x3* InBuffer, m256x3* OutBuffer)
{
	__m256 UOffsets = _mm256_setr_ps(0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f);

	__m256i GreenChannelOffset = _mm256_set1_epi32(8);
	__m256i BlueChannelOffset  = _mm256_set1_epi32(16);

	for (i32 OutRow = 0; OutRow < OutHeight; OutRow++)
	{
		__m256 V = set1_ps((OutRow + 0.5f) / OutHeight);
		__m256 V0 = round_floor(V * (f32)InHeight);
		__m256 V1 = round_floor(V * (f32)InHeight + 1.f);

		for (i32 OutCol = 0; OutCol < OutWidth / LANE_COUNT; OutCol++)
		{
			__m256 U = UOffsets + set1_ps((OutCol + 0.5f) / OutWidth);

			__m256 U0 = round_floor(U * (f32)InWidth);
			__m256 U1 = round_floor(U * (f32)InWidth + 1.f);

			__m256i I00 = _mm256_cvtps_epi32(U0 + V0 * (f32)(InWidth-1));
			__m256i I10 = _mm256_cvtps_epi32(U1 + V0 * (f32)(InWidth-1));
			__m256i I01 = _mm256_cvtps_epi32(U0 + V1 * (f32)(InWidth-1));
			__m256i I11 = _mm256_cvtps_epi32(U1 + V1 * (f32)(InWidth-1));

			const i32 scale = sizeof(f32);
			const f32 * const base_addr = (f32*)InBuffer;
			m256x3 C00 = GatherRGB(base_addr, I00);
			m256x3 C10 = GatherRGB(base_addr, I10);
			m256x3 C01 = GatherRGB(base_addr, I01);
			m256x3 C11 = GatherRGB(base_addr, I11);
				
			m256x3 C0 = lerp(C00, C10, U - U0);
			m256x3 C1 = lerp(C01, C11, U - U0);
			m256x3 C = lerp(C0, C1, V - V0);

			OutBuffer[OutRow + OutCol * OutWidth] = C;
		}
	}
}