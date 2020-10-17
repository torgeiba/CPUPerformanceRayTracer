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

#define USE_VSYNC 1
#define SHOW_FRAMETIMES 1

// Global instance
ApplicationState App;

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
texture Texture;

#include "intrinsic_utils.h"

static f64 RenderTime_inner = 0; // Used to time the inner rendering function (excluding the copy out from rendertarget)

void ApplicationState::RunApp(HINSTANCE Instance, i32 ShowCode)
{

	//char* texturefilePath = "E:\\Visual Studio Projects\\CPUPerformanceRayTracer\\Textures\\Delta_2k.hdr";
	char* texturefilePath = "E:\\Visual Studio Projects\\CPUPerformanceRayTracer\\Textures\\chinese_garden_2k.hdr";

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
	i32 ClientWidth = 640*2, ClientHeight = 360*2;
	i32 BackbufferResolutionX = 360, BackbufferResolutionY = 360;

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
				" ms, Render (inner): " + std::to_string(/*RenderTimeSeconds*/ RenderTime_inner * 1000.) +
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

void ApplicationState::Render()
{
	win32_offscreen_buffer* Buffer = &BackBuffer;
	if (HasRenderedThisFrame) return;

	i32 Width = Buffer->Width;
	i32 Height = Buffer->Height;

	f32* RenderTarget = Buffer->RenderTarget;

	i32 NumTilesX = 2 * 8;
	i32 NumTilesY = 4 * 8;
	i32 TileWidth = Width / NumTilesX;
	i32 TileHeight = Height / NumTilesY;
	i64 PerfRenderStart_inner = GetPerformanceCounter();
	DemofoxRenderOptV2(RenderTarget, Width, Height, NumTilesX, NumTilesY, TileWidth, TileHeight, 3, Texture, Buffer->Memory);
	RenderTime_inner = GetPerformanceCounterIntervalSeconds(PerfRenderStart_inner, GetPerformanceCounter());

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