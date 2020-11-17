#pragma once

#include "utils.h"

#include "mathlib.h"
#include "texture.h"

void DemofoxRenderOptV4(f32* BufferOut, i32 BufferWidth, i32 BufferHeight, i32 NumTilesX, i32 NumTilesY, i32 TileWidth, i32 TileHeight, i32 NumChannels,
    texture Texture,
    void* ScreenBufferData
);

void CopyOutputToFile(f32* BufferOut, i32 BufferWidth, i32 BufferHeight, i32 NumTilesX, i32 NumTilesY, i32 TileWidth, i32 TileHeight, i32 NumChannels,
    texture Texture,
    void* ScreenBufferData
);

void InitializeGlobalRenderResources();