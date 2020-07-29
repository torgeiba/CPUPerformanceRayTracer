#define WIN32_LEAN_AND_MEAN
#include "windows.h"

#include "utils.h"
#include "Application.h"

#include "asset_loading.h"
#include "stb_image.h"

i32 CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, char* CommandLine, i32 ShowCode)
{
	InitializeQueryPerformanceCounter();

	// Temp, remove
	char* filename = "C:\\Visual Studio Projects\\CPUPerformanceRayTracer\\Textures\\chinese_garden_2k.hdr";
	bool bIsHDR = stbi_is_hdr(filename);

	int x = 0;
	int y = 0;
	int comp = 0;
	float* data = stbi_loadf(filename, &x, &y, &comp, 0);

	App.RunApp(Instance, ShowCode);
	return 0;
}