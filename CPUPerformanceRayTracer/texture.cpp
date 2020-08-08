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
		if (uv.x >= 0.f && uv.x < 1.f && uv.y >= 0.f && uv.y < 1.f)
		{
			i32 Row = (i32)(uv.y * texture.Height);
			i32 Col = (i32)(uv.x * texture.Width);
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