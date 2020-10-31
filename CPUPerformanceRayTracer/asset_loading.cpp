#include "asset_loading.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

texture LoadTexture(char* filename)
{
	bool bIsHDR = stbi_is_hdr(filename);
	stbi_set_flip_vertically_on_load(true);
	texture Texture;
	Texture.Data = stbi_loadf(filename, &Texture.Width, &Texture.Height, &Texture.Components, 0);
	return Texture;
}



void WriteImage(char* filename, i32 width, i32 height, i32 components, void* data)
{
	//stbi_write_png(filename, width, height, components, data, );
	//stbi_write_jpg(filename, width, height, components, data, 100);
	stbi_write_bmp(filename, width, height, components, data);
	//stbi_write_hdr(filename, width, height, components, (f32*)data);
}