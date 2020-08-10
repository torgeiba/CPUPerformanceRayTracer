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

f32x3 TexelFetch(texture texture, i32 Row, i32 Col);
f32x3 EquirectangularTextureSample(texture texture, f32x3 Direction);
m256x3 EquirectangularTextureSample(texture texture, m256x3 Directions);