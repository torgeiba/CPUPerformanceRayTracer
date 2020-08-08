#include "asset_loading.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

texture LoadTexture(char* filename)
{
	bool bIsHDR = stbi_is_hdr(filename);
	stbi_set_flip_vertically_on_load(true);
	texture Texture;
	Texture.Data = stbi_loadf(filename, &Texture.Width, &Texture.Height, &Texture.Components, 0);
	return Texture;
}