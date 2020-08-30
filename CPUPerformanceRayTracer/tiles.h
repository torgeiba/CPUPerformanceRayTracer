#pragma once

#include "utils.h"

struct RenderTileInfo
{
    i32 TileX, TileY;
    i32 TileMinX, TileMaxX;
    i32 TileMinY, TileMaxY;
};

struct TileSet
{
	i32 MaxDepth;
    i32 NumTilesX, NumTilesY;
    i32 TileWidth, TileHeight;
};



inline i32 RoundIntegerToNextMultiple(i32 i, i32 Multiple)
{
	return ((i + Multiple - 1) / Multiple) * Multiple;
}