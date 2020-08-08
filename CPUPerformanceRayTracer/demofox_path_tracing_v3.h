#pragma once

#pragma once

#include "utils.h"

#include "mathlib.h"
#include "texture.h"

void DemofoxRenderV3(f32* BufferOut, i32 BufferWidth, i32 BufferHeight, i32 NumTilesX, i32 NumTilesY, i32 TileWidth, i32 TileHeight, i32 NumChannels,
    texture Texture
);

m256x3 ACESFilm(m256x3 X);
m256x3 LinearToSRGB(m256x3 rgb);