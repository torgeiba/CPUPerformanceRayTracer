#define WIN32_LEAN_AND_MEAN
#include "windows.h"

#include "utils.h"
#include "Application.h"

i32 CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, char* CommandLine, i32 ShowCode)
{
	InitializeQueryPerformanceCounter();
	App.RunApp(Instance, ShowCode);
	return 0;
}