#define WIN32_LEAN_AND_MEAN
#include "windows.h"

#include "utils.h"
#include "Application.h"

#include "global_preprocessor_flags.h"

#if RENDER_OFFLINE
i32 __cdecl main(int argc, char* argv[])
{
	InitializeQueryPerformanceCounter();
	App.RenderOffline();
	return 0;
}
#else
i32 CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, char* CommandLine, i32 ShowCode)
{
	InitializeQueryPerformanceCounter();
	App.RunApp(Instance, ShowCode);
	return 0;
}
#endif



