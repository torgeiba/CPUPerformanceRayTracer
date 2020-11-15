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

texture LoadCubemapTexture(char* filename[6])
{
	texture cubemap;
	i32 Width, Height, Components;
	f32* Data[6];
	for (i32 i = 0; i < 6; i++)
	{
		bool bIsHDR = stbi_is_hdr(filename[i]);
		stbi_set_flip_vertically_on_load(true);
		Data[i] = stbi_loadf(filename[i], &Width, &Height, &Components, 0);
	}
	
	u64 faceSize = (u64)Width * (u64)Height * (u64)Components;
	cubemap.Width = Width;
	cubemap.Height = Height * 6;
	cubemap.Components = Components;
	cubemap.Data = (f32*)malloc(faceSize * 6 * sizeof(f32));

	f32* DataPos = cubemap.Data;
	for (u64 i = 0; i < 6; i++)
	{
		memcpy(DataPos + (i * faceSize), Data[i], faceSize * sizeof(f32));
		free(Data[i]);
	}

	return cubemap;
}



void WriteImage(char* filename, i32 width, i32 height, i32 components, void* data)
{
	//stbi_write_png(filename, width, height, components, data, );
	//stbi_write_jpg(filename, width, height, components, data, 100);
	stbi_write_bmp(filename, width, height, components, data);
	//stbi_write_hdr(filename, width, height, components, (f32*)data);
}

void WriteHDRImage(char* filename, i32 width, i32 height, i32 components, const float* data)
{
	stbi_write_hdr(filename, width, height, components, data);
}