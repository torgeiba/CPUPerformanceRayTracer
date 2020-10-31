#pragma once

#include "utils.h"
#include "texture.h"

texture LoadTexture(char* filename);

void WriteImage(char* filename, i32 width, i32 height, i32 components, void* data);