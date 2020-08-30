#include "tiles.h"
#include "utils.h"

TileSet MakeTiles(i32 BufferWidth, i32 BufferHeight, i32 TileWidth, i32 TileHeight)
{
    TileSet Tiles;
    Tiles.NumTilesX = (BufferWidth + TileWidth - 1) / TileWidth;
    Tiles.NumTilesY = (BufferHeight + TileHeight - 1) / TileHeight;
    Tiles.TileWidth = TileWidth;
    Tiles.TileHeight = TileHeight;
    for (i32 TileX = 0; TileX < Tiles.NumTilesX; TileX++)
    {
        i32 TileMinX = TileX * TileWidth;
        i32 TileMaxX = TileMinX + (TileWidth - 1);
        for (i32 TileY = 0; TileY < Tiles.NumTilesY; TileY++)
        {
            // render tile (TileX, TileY)
            i32 TileMinY = TileY * TileHeight;
            i32 TileMaxY = TileMinY + (TileHeight - 1);

            RenderTileInfo TileInfo;
            {
                TileInfo.TileX = TileX;
                TileInfo.TileY = TileY;

                TileInfo.TileMinX = TileMinX;
                TileInfo.TileMaxX = TileMaxX < BufferWidth ? TileMaxX : (BufferWidth - 1);

                TileInfo.TileMinY = TileMinY;
                TileInfo.TileMaxY = TileMaxY < BufferHeight ? TileMaxY : (BufferHeight - 1);
            }

            i32 FlatTileIndex = TileX + Tiles.NumTilesX * TileY;
        }
    }
    return Tiles;
}

//void TiledRendering(f32* BufferOut, i32 BufferWidth, i32 BufferHeight, i32 NumTilesX, i32 NumTilesY, i32 TileWidth, i32 TileHeight, i32 NumChannels,
//    texture Texture
//)
//{
//    RenderBufferInfo BufferInfo;
//    {
//        BufferInfo.BufferDataPtr = BufferOut;
//        BufferInfo.BufferWidth = BufferWidth;
//        BufferInfo.BufferHeight = BufferHeight;
//        BufferInfo.NumChannels = NumChannels;
//    }
//
//    i32 NumThreads = NumTilesX * NumTilesY;
//
//    // If first time we render, initialize threadpool. TODO: do this at app initialization instead
//    if (threadpooluninitialized)
//    {
//        Queue = MakeWorkQueue();
//        threadpooluninitialized = false;
//    }
//
//    iFrame += 1.0f;
//
//    for (i32 TileX = 0; TileX < NumTilesX; TileX++)
//    {
//        i32 TileMinX = TileX * TileWidth;
//        i32 TileMaxX = TileMinX + (TileWidth - 1);
//        for (i32 TileY = 0; TileY < NumTilesY; TileY++)
//        {
//            // render tile (TileX, TileY)
//            i32 TileMinY = TileY * TileHeight;
//            i32 TileMaxY = TileMinY + (TileHeight - 1);
//
//            RenderTileInfo TileInfo;
//            {
//                TileInfo.TileX = TileX;
//                TileInfo.TileY = TileY;
//
//                TileInfo.TileHeight = TileHeight;
//                TileInfo.TileWidth = TileWidth;
//
//                TileInfo.TileMinX = TileMinX;
//                TileInfo.TileMaxX = TileMaxX < BufferWidth ? TileMaxX : (BufferWidth - 1);
//
//                TileInfo.TileMinY = TileMinY;
//                TileInfo.TileMaxY = TileMaxY < BufferHeight ? TileMaxY : (BufferHeight - 1);
//
//                TileInfo.TileHeight = TileInfo.TileMaxY - TileInfo.TileMinY + 1;
//                TileInfo.TileWidth = TileInfo.TileMaxX - TileInfo.TileMinX + 1;
//            }
//
//            i32 FlatTileIndex = TileX + NumTilesX * TileY;
//            WorkData[FlatTileIndex].BufferInfo = BufferInfo;
//            WorkData[FlatTileIndex].TileInfo = TileInfo;
//            WorkData[FlatTileIndex].Texture = Texture;
//            void* DataPointer = &WorkData[FlatTileIndex];
//            AddWorkQueueEntry(Queue, DoWorkerThreadWork, DataPointer);
//        }
//    }
//
//    CompleteAllWork(Queue);