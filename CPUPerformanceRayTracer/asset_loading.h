#pragma once

#include "utils.h"
#include "texture.h"

texture LoadTexture(char* filename);
texture LoadCubemapTexture(char* filename[6]);

void WriteImage(char* filename, i32 width, i32 height, i32 components, void* data);

void WriteHDRImage(char* filename, i32 width, i32 height, i32 components, const float* data);