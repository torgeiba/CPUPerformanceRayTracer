#define NOMINMAX

#include "Application.h"

#include "dwmapi.h" // For frame/vsync, DwmFlush();

#pragma comment(lib, "Dwmapi.lib")

#include <iostream>   // std::cout
#include <string>     // std::string, std::to_string
#include <cstdio>
//#include "WindowsX.h" // Some unused macros

#include <emmintrin.h>

#include <intrin.h>

#include "mathlib.h"

#include "rendering.h"

//#include "demofox_path_tracing_scalar.h"
//#include "demofox_path_tracing_scalar_branchless.h"
//#include "demofox_path_tracing_simt.h"
//#include "demofox_path_tracing_simt_textured.h"
//#include "demofox_path_tracing_v3_redo.h"
#include "demofox_path_tracing_optimization_v2.h"

#include "global_preprocessor_flags.h"

// Global instance
ApplicationState App;

bool ApplicationState::CheckValidSettings()
{
	bool isValid = true;

	i32 TileWidth  = BackBuffer.Width / NUM_TILES_X;
	i32 TileHeight = BackBuffer.Height / NUM_TILES_Y;
	bool validTileWidthX = (TileWidth % 8) == 0;

	bool validTilesY = (BackBuffer.Height % NUM_TILES_Y) == 0;
	bool validTilesX = (BackBuffer.Width  % NUM_TILES_X) == 0;

	bool validBufferWidth = (BackBuffer.Width % 8) == 0;

	isValid = validTileWidthX && validTilesY && validTilesX;
	if (!isValid)
	{
		if (!validTileWidthX)
		{
			std::cout << "Invalid tile width detected. Must be multiple of 8 wide because of SIMD lane width" << "\n"
				<< "Image width: " << BackBuffer.Width << "\n"
				<< "Number of tile columns: " << NUM_TILES_X << "\n"
				<< "Computed tile-width: " << f64(BackBuffer.Width)/f64(NUM_TILES_X) << "\n"
				;
			__debugbreak();
		}

		if (!validTilesY)
		{
			std::cout << "Invalid number of tile rows detected. " << "\n"
				<< "Image height: " << BackBuffer.Height << "\n"
				<< "Number of tile rows: " << NUM_TILES_Y << "\n"
				<< "Computed tile-height: " << f64(BackBuffer.Height) / f64(NUM_TILES_Y) << "\n"
				;
			__debugbreak();
		}

		if (!validTilesX)
		{
			std::cout << "Invalid number of tile columns detected. " << "\n"
				<< "Image width: " << BackBuffer.Width << "\n"
				<< "Number of tile columns: " << NUM_TILES_X << "\n"
				<< "Computed tile-height: " << f64(BackBuffer.Width) / f64(NUM_TILES_X) << "\n"
				;
			__debugbreak();
		}
		
		if (!validBufferWidth)
		{
			std::cout << "Invalid image width detected. Must be multiple of 8 because of SIMD " << "\n"
				<< "Image width: " << BackBuffer.Width << "\n"
				<< "Number of tile columns: " << NUM_TILES_X << "\n"
				<< "Computed tile-height: " << f64(BackBuffer.Width) / f64(NUM_TILES_X) << "\n"
				;
			__debugbreak();
		}
	}

	return isValid;
}

static void Win32GetWindowClientDimensions(HWND Window, i32& WidthOut, i32& HeightOut)
{
	RECT ClientRect;
	GetClientRect(Window, &ClientRect); // Drawable area of window
	WidthOut = ClientRect.right - ClientRect.left;
	HeightOut = ClientRect.bottom - ClientRect.top;
}

void win32_offscreen_buffer::Resize(i32 NewWidth, i32 NewHeight)
{
	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE); // MEM_DECOMMIT
	}

	if (RenderTarget)
	{
		VirtualFree(RenderTarget, 0, MEM_RELEASE);
	}

	Width = NewWidth;
	Height = NewHeight;
	BytesPerChannel = 1;
	NumChannels = 4;
	BytesPerPixel = BytesPerChannel * NumChannels;

	i32 yDirectionUp = -1; // top down

	{
		Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
		Info.bmiHeader.biWidth = Width;
		Info.bmiHeader.biHeight = yDirectionUp * Height;
		Info.bmiHeader.biPlanes = 1;
		Info.bmiHeader.biBitCount = NumChannels * BytesPerChannel * 8; // for DWORD alignment
		Info.bmiHeader.biCompression = BI_RGB;
	}

	i32 BitmapMemorySize = Width * Height * BytesPerPixel;
	i32 RenderTargetSize = Width * Height * 3 * sizeof(f32);
	Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	RenderTarget = (f32*)VirtualAlloc(0, RenderTargetSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	memset(RenderTarget, 0, RenderTargetSize);
	Pitch = Width * BytesPerPixel;
}

static void Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer, HDC DeviceContext, i32 WindowWidth, i32 WindowHeight)
{
	i32 res = StretchDIBits(
		DeviceContext,
		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory, &Buffer->Info,
		DIB_RGB_COLORS, SRCCOPY
	);
}

static void GetMouseCoordinates(i64 LParam, i32& XPosOut, i32& YPosOut)
{
	XPosOut = (i32)(i16)((u16)(((u64)LParam) & 0xffff)); //GET_X_LPARAM(LParam);
	YPosOut = (i32)(i16)((u16)((((u64)LParam) >> 16) & 0xffff)); //GET_Y_LPARAM(LParam);
}

LRESULT CALLBACK Win32MainWindowCallback_Wrapper(HWND Window, u32 Message, u64 WParam, i64 LParam)
{
	return App.Win32MainWindowCallback(Window, Message, WParam, LParam);
}

inline void ApplicationState::RunMessageLoop() {
	MSG Message = {};
	while (BOOL bRet = PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
	{
		Running = Running && (Message.message != WM_QUIT);
		TranslateMessage(&Message); // processes message to be sent out
		DispatchMessageA(&Message); //	Each time the program calls the DispatchMessage function, it indirectly causes Windows to invoke the WindowProc function, once for each message.
	}
}

#include "asset_loading.h"
texture Texture; // todo, get rid of this global variable

#include "intrinsic_utils.h"

//static f64 RenderTime_inner = 0; // Used to time the inner rendering function (excluding the copy out from rendertarget)

void ApplicationState::RunApp(HINSTANCE Instance, i32 ShowCode)
{

	//char* texturefilePath = "E:\\Visual Studio Projects\\CPUPerformanceRayTracer\\Textures\\Delta_2k.hdr";
	//char* texturefilePath = "E:\\Visual Studio Projects\\CPUPerformanceRayTracer\\Textures\\chinese_garden_2k.hdr";//
	//char* texturefilePath = "E:\\Visual Studio Projects\\CPUPerformanceRayTracer\\Textures\\HDR_040_Field.hdr";
	char* texturefilePath = "E:\\Visual Studio Projects\\CPUPerformanceRayTracer\\Textures\\HDR_040_Field_Env.hdr";

	Texture = LoadTexture(texturefilePath);

	WNDCLASSA WindowClass = {};
	{
		WindowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		WindowClass.lpfnWndProc = Win32MainWindowCallback_Wrapper;
		WindowClass.hInstance = Instance;
		WindowClass.lpszClassName = "WindowsDisplay";
	}

	if (RegisterClassA(&WindowClass) == 0) return;

	DWORD WindowExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD WindowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

	SetProcessDPIAware(); // In order to not report wrong monitor resolution when going fullscreen

	bool FullScreen = false;
	bool UseFullWindowResolutionBackbuffer = true;
	bool UseSpecifiedClientSize = true;

	i32 WindowWidth = CW_USEDEFAULT, WindowHeight = CW_USEDEFAULT;
	i32 WindowTopLeftX = CW_USEDEFAULT, WindowTopLeftY = CW_USEDEFAULT;
	i32 ClientWidth = WINDOW_CLIENT_PIXEL_WIDTH, ClientHeight = WINDOW_CLIENT_PIXEL_HEIGHT;
	i32 BackbufferResolutionX = RENDER_BUFFER_PIXEL_WIDTH, BackbufferResolutionY = RENDER_BUFFER_PIXEL_HEIGHT;

	if (FullScreen)
	{
		WindowExStyle = 0;
		WindowStyle = WS_POPUP | WS_VISIBLE;
		HMONITOR hmon = MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi = { sizeof(mi) };
		if (!GetMonitorInfoA(hmon, &mi)) return;
		WindowWidth = mi.rcMonitor.right;
		WindowHeight = mi.rcMonitor.bottom;
		WindowTopLeftX = 0; WindowTopLeftY = 0;
	}

	// Keep client size as requested
	if (UseSpecifiedClientSize)
	{
		RECT WindowRect = { 0, 0, ClientWidth, ClientHeight };
		AdjustWindowRectEx(&WindowRect, WindowStyle, FALSE, WindowExStyle);
		WindowWidth = WindowRect.right - WindowRect.left;
		WindowHeight = WindowRect.bottom - WindowRect.top;
	}

	Window = CreateWindowExA(WindowExStyle, WindowClass.lpszClassName, "WindowDisplay", WindowStyle,
		WindowTopLeftX, WindowTopLeftY, WindowWidth, WindowHeight, 0, 0, Instance, 0
	);

	if (Window == 0) return;

	Win32GetWindowClientDimensions(Window, CurrentWindowWidth, CurrentWindowHeight);
	if (UseFullWindowResolutionBackbuffer)
	{
		BackbufferResolutionX = CurrentWindowWidth;
		BackbufferResolutionY = CurrentWindowHeight;
	}
	// Initialize with fixed backbuffer resolution
	BackBuffer.Resize(BackbufferResolutionX, BackbufferResolutionY);

	CheckValidSettings();

	InitializeGlobalRenderResources();

	Render();// TEMP NOTE TODO: do not render here while resize code is unsafe for threads

	std::string fps_str;

	// Init end

	// Start message loop and run app
	Running = true;
	i64 FrameCount = 0, AggregateFrames = 0;
	i64 PerfFrameStart = GetPerformanceCounter();
	i64 PerfSyncStart = PerfFrameStart, PerfSyncEnd = PerfFrameStart;
	f64 RenderTimeSeconds = 0., MsgTimeSeconds = 0., PresentTimeSeconds = 0.;

	i64 PerfMsgStart, PerfRenderStart, PerfPresentStart;

	ShowWindow(Window, ShowCode);

	HDC DeviceContext = GetDC(Window);

	while (Running)
	{
#if SHOW_FRAMETIMES
		PerfSyncEnd = GetPerformanceCounter();
		f64 SyncTimeSeconds = GetPerformanceCounterIntervalSeconds(PerfSyncStart, PerfSyncEnd);
#endif
		HasRenderedThisFrame = false;

		// Compute framerate
#if SHOW_FRAMETIMES
		if (FrameCount % 30 == 0)
		{
			i64 PerfPreviousFrameStart = PerfFrameStart;
			PerfFrameStart = GetPerformanceCounter();
			f64 TimeSeconds = GetPerformanceCounterIntervalSeconds(PerfPreviousFrameStart, PerfFrameStart);
			TimeSeconds /= (f64)AggregateFrames;
			AggregateFrames = 0;

			fps_str =
				std::string("FrameTime: ") + std::to_string(TimeSeconds * 1000.) +
				" ms, Sync: " + std::to_string(SyncTimeSeconds * 1000.) +
				" ms, MsgProcess: " + std::to_string(MsgTimeSeconds * 1000.) +
				//" ms, Render (inner): " + std::to_string(/*RenderTimeSeconds*/ RenderTime_inner * 1000.) +
				" ms, Render (total): " + std::to_string(RenderTimeSeconds * 1000.) +
				" ms, Present: " + std::to_string(PresentTimeSeconds * 1000.) + " ms"
				//+ ". X: " + std::to_string(MouseCoords.XPos) + ", Y: " + std::to_string(MouseCoords.YPos)
				;

			SetWindowTextA(Window, fps_str.c_str());
		}
#endif

#if SHOW_FRAMETIMES
		PerfMsgStart = GetPerformanceCounter();
#endif
			RunMessageLoop();

#if SHOW_FRAMETIMES
		MsgTimeSeconds = GetPerformanceCounterIntervalSeconds(PerfMsgStart, GetPerformanceCounter());
#endif

#if SHOW_FRAMETIMES
		PerfRenderStart = GetPerformanceCounter();
#endif
			Render();
#if SHOW_FRAMETIMES
		RenderTimeSeconds = GetPerformanceCounterIntervalSeconds(PerfRenderStart, GetPerformanceCounter());
#endif

		// Present
#if SHOW_FRAMETIMES
		PerfPresentStart = GetPerformanceCounter();
#endif
			Win32DisplayBufferInWindow(&BackBuffer, DeviceContext, CurrentWindowWidth, CurrentWindowHeight);
#if SHOW_FRAMETIMES
		PresentTimeSeconds = GetPerformanceCounterIntervalSeconds(PerfPresentStart, GetPerformanceCounter());
#endif

		// Util counters
		FrameCount++;
		AggregateFrames++;
		// Refreshrate sync
#if SHOW_FRAMETIMES
		PerfSyncStart = GetPerformanceCounter();
#endif

#if USE_VSYNC
		DwmFlush();
#endif
	}
	// Main app loop end

	ReleaseDC(Window, DeviceContext);
}

f64 ApplicationState::RenderOffline()
{
	//char* texturefilePath = "E:\\Visual Studio Projects\\CPUPerformanceRayTracer\\Textures\\chinese_garden_2k.hdr";
	//char* texturefilePath = "E:\\Visual Studio Projects\\CPUPerformanceRayTracer\\Textures\\HDR_040_Field.hdr";
	char* texturefilePath = "E:\\Visual Studio Projects\\CPUPerformanceRayTracer\\Textures\\HDR_040_Field_Env.hdr";
	Texture = LoadTexture(texturefilePath);

	i32 BackbufferResolutionX = RENDER_BUFFER_PIXEL_WIDTH;
	i32 BackbufferResolutionY = RENDER_BUFFER_PIXEL_HEIGHT;
	BackBuffer.Resize(BackbufferResolutionX, BackbufferResolutionY); // Initialize with fixed backbuffer resolution

	Running = true;

	InitializeGlobalRenderResources();

	CheckValidSettings();

	i64 NumFramesToRender = NUM_FRAMES_TO_RENDER;
	HasRenderedThisFrame = false;
	Render();
	Render();
	i64 TimerStart = GetPerformanceCounter();
	f64 CompletionProgress = 0.;
	f64 LastPrintProgress = 0.f;
	for(i64 iFrame = 0; iFrame < NumFramesToRender; iFrame++)
	{
		HasRenderedThisFrame = false;
		Render();
		CompletionProgress = (f64)(iFrame + 1) / (f64)NumFramesToRender;
		if (CompletionProgress >= LastPrintProgress + 0.01)
		{
			LastPrintProgress = CompletionProgress;
			std::cout << (CompletionProgress * 100.) << "\%" << "\n";
		}
	}
	i64 TimerEnd = GetPerformanceCounter();
	i64 FrequencySeconds = GetPerformanceCounterFrequency();
	i64 SecondsToMillisecondsFactor = 1000;
	i64 TotalTicks = TimerEnd - TimerStart;
	i64 TotalTimeTicks = (SecondsToMillisecondsFactor * TotalTicks);
	f64 FrameTimeMs = (TotalTimeTicks / (f64)FrequencySeconds) / (f64)NumFramesToRender;
	std::cout << TotalTimeTicks << "\n" << FrameTimeMs << "\n";


	std::cout << "Writing to file..." << "\n";
	{
		win32_offscreen_buffer* Buffer = &BackBuffer;

		i32 Width = Buffer->Width;
		i32 Height = Buffer->Height;

		f32* RenderTarget = Buffer->RenderTarget;
		i32 NumTilesX = NUM_TILES_X;
		i32 NumTilesY = NUM_TILES_Y;
		i32 TileWidth = Width / NumTilesX;
		i32 TileHeight = Height / NumTilesY;
		CopyOutputToFile(RenderTarget, Width, Height, NumTilesX, NumTilesY, TileWidth, TileHeight, 3, Texture, Buffer->Memory);
	}

	WriteImage("output_image.bmp", BackbufferResolutionX, BackbufferResolutionY, 4, BackBuffer.Memory);

	return FrameTimeMs;
}

void ApplicationState::Render()
{
	win32_offscreen_buffer* Buffer = &BackBuffer;
	if (HasRenderedThisFrame) return;

	i32 Width = Buffer->Width;
	i32 Height = Buffer->Height;

	f32* RenderTarget = Buffer->RenderTarget;

	i32 NumTilesX = NUM_TILES_X;
	i32 NumTilesY = NUM_TILES_Y;
	i32 TileWidth = Width / NumTilesX;
	i32 TileHeight = Height / NumTilesY;
	DemofoxRenderOptV2(RenderTarget, Width, Height, NumTilesX, NumTilesY, TileWidth, TileHeight, 3, Texture, Buffer->Memory);

	HasRenderedThisFrame = true;
}

LRESULT ApplicationState::Win32MainWindowCallback(HWND Window, u32 Message, u64 WParam, i64 LParam)
{
	LRESULT Result = 0;

	switch (Message)
	{
	case WM_SIZE: // Resize window
	{
		Win32GetWindowClientDimensions(Window, CurrentWindowWidth, CurrentWindowHeight);
		BackBuffer.Resize(CurrentWindowWidth, CurrentWindowHeight);
		
		// TODO: NOTE: currently pooled threaded rendering does not handle this well
		Render();
	} break;
	case WM_DESTROY: // Should be handled as an error
	{
		Running = false; // Alternatively: PostQuitMessage(0);
	} break;
	case WM_CLOSE: // Should be handled as user quitting
	{
		Running = false; // Alternatively: PostQuitMessage(0); or DestroyWindow(0);
	} break;
	case WM_ACTIVATEAPP: // Window got focus
	{
	} break;
	case WM_SETCURSOR: // TODO: fix this
	{
		//SetCursor(0);  // To avoid wait cursor //Result = DefWindowProc(Window, Message, WParam, LParam);
	} break;
	case WM_PAINT:
	{
		PAINTSTRUCT Paint = {};
		HDC DeviceContext = BeginPaint(Window, &Paint);
		i32 WindowClientWidth, WindowClientHeight;
		Win32GetWindowClientDimensions(Window, WindowClientWidth, WindowClientHeight);
		Win32DisplayBufferInWindow(&BackBuffer, DeviceContext, WindowClientWidth, WindowClientHeight);

		EndPaint(Window, &Paint);
	} break;

	// Mouse buttons
	case WM_LBUTTONDOWN:
	{
		GetMouseCoordinates(LParam, MouseLDownCoords.XPos, MouseLDownCoords.YPos);
	} break;
	case WM_LBUTTONUP:
	{
		GetMouseCoordinates(LParam, MouseLUpCoords.XPos, MouseLUpCoords.YPos);
	} break;
	case WM_MBUTTONDOWN:
	{
		GetMouseCoordinates(LParam, MouseMDownCoords.XPos, MouseMDownCoords.YPos);
	} break;
	case WM_MBUTTONUP:
	{
		GetMouseCoordinates(LParam, MouseMUpCoords.XPos, MouseMUpCoords.YPos);
	} break;
	case WM_RBUTTONDOWN:
	{
		GetMouseCoordinates(LParam, MouseRDownCoords.XPos, MouseRDownCoords.YPos);
	} break;
	case WM_RBUTTONUP:
	{
		GetMouseCoordinates(LParam, MouseRUpCoords.XPos, MouseRUpCoords.YPos);
	} break;
	case WM_XBUTTONDOWN:
	{
		GetMouseCoordinates(LParam, MouseXDownCoords.XPos, MouseXDownCoords.YPos);
	} break;
	case WM_XBUTTONUP:
	{
		GetMouseCoordinates(LParam, MouseXUpCoords.XPos, MouseXUpCoords.YPos);
	} break;


	// Mouse move
	// The WM_MOUSEMOVE message contains the same parameters as the messages for mouse clicks
	// The wParam parameter contains a bitwise OR of flags, indicating the state of the other mouse buttons plus the SHIFT and CTRL keys.
	case WM_MOUSEMOVE:
	{
		GetMouseCoordinates(LParam, MouseMoveCoords.XPos, MouseMoveCoords.YPos);
	} break;

	case WM_MOUSEWHEEL:
	{
		i32 WheelDelta = 120;
		i32 Delta = (i32)((i16)((u16)((((u64)(WParam)) >> 16) & 0xffff)));
		Global_Wheel_Value += Delta;
	} break;

	// Keyboard
	case WM_SYSKEYDOWN:
	{
		u64 KeyCode = WParam;
	} break;

	case WM_SYSCHAR:
	{
		wchar_t Syscharacter = (wchar_t)WParam;
	} break;

	case WM_SYSKEYUP:
	{
		u64 KeyCode = WParam;
	} break;

	case WM_KEYDOWN:
	{
		u64 KeyCode = WParam;
	} break;

	case WM_KEYUP:
	{
		u64 KeyCode = WParam;
	} break;
	case WM_CHAR:
	{
		wchar_t Character = (wchar_t)WParam;
	} break;

	default:
	{
		Result = DefWindowProc(Window, Message, WParam, LParam);
	} break;
	}

	return Result;
}