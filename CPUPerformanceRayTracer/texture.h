#pragma once

#include "utils.h"
#include "mathlib.h"

struct texture
{
	f32* Data = 0;
	i32 Width = 0;
	i32 Height = 0;
	i32 Components = 3;
};

struct cubemap_texture
{
	f32* data;
	i32 Width = 0;
	i32 Height = 0;
	i32 Components = 3;
};

f32x3 TexelFetch(texture texture, i32 Row, i32 Col);
m256x3 TexelFetchGather(texture texture, __m256i Rows, __m256i Cols);
f32x3 EquirectangularTextureSample(texture texture, f32x3 Direction);
m256x3 EquirectangularTextureSample(texture texture, m256x3 Directions);
m256x3 EquirectangularTextureSampleGather(texture texture, m256x3 Directions);
m256x3 EquirectangularTextureSampleBilinear(texture texture, m256x3 Directions);

m256x3 CubemapTextureSampleBilinear(texture cubemap, m256x3 Directions);
m256x3 CubemapTextureSampleRandom(texture cubemap, m256x3 Directions, __m256i& randstate);