#include "Application.h"

#include "dwmapi.h" // For frame/vsync, DwmFlush();

#pragma comment(lib, "Dwmapi.lib")

#include <iostream>   // std::cout
#include <string>     // std::string, std::to_string
#include <cstdio>
//#include "WindowsX.h" // Some unused macros

#include <emmintrin.h>

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
	Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
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

void ApplicationState::RunApp(HINSTANCE Instance, i32 ShowCode)
{
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
	i32 ClientWidth = 1920, ClientHeight = 1080;
	i32 BackbufferResolutionX = 1280, BackbufferResolutionY = 720;

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
	Render();

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
		PerfSyncEnd = GetPerformanceCounter();
		f64 SyncTimeSeconds = GetPerformanceCounterIntervalSeconds(PerfSyncStart, PerfSyncEnd);
		HasRenderedThisFrame = false;

		// Compute framerate
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
				" ms, Render: " + std::to_string(RenderTimeSeconds * 1000.) +
				" ms, Present: " + std::to_string(PresentTimeSeconds * 1000.) + " ms"
				//+ ". X: " + std::to_string(MouseCoords.XPos) + ", Y: " + std::to_string(MouseCoords.YPos)
				;

			SetWindowTextA(Window, fps_str.c_str());
		}

		PerfMsgStart = GetPerformanceCounter();
			RunMessageLoop();
		MsgTimeSeconds = GetPerformanceCounterIntervalSeconds(PerfMsgStart, GetPerformanceCounter());

		PerfRenderStart = GetPerformanceCounter();
			Render();
		RenderTimeSeconds = GetPerformanceCounterIntervalSeconds(PerfRenderStart, GetPerformanceCounter());

		// Present
		PerfPresentStart = GetPerformanceCounter();
			Win32DisplayBufferInWindow(&BackBuffer, DeviceContext, CurrentWindowWidth, CurrentWindowHeight);
		PresentTimeSeconds = GetPerformanceCounterIntervalSeconds(PerfPresentStart, GetPerformanceCounter());

		// Util counters
		FrameCount++;
		AggregateFrames++;
		// Refreshrate sync
		PerfSyncStart = GetPerformanceCounter();
		DwmFlush();
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

	anim_offset++;

	i32 WindowWidth = CurrentWindowWidth;
	i32 WindowHeight = CurrentWindowHeight;

	i32 MouseMoveX = ((Width)* MouseMoveCoords.XPos) / (WindowWidth);
	i32 MouseMoveY = ((Height)* MouseMoveCoords.YPos) / (WindowHeight);

	i32 MousePressX = ((Width)* MouseLDownCoords.XPos) / (WindowWidth);
	i32 MousePressY = ((Height)* MouseLDownCoords.YPos) / (WindowHeight);

	i32 dZ2 = 100 + Global_Wheel_Value;

	u8 *Row = (u8*)Buffer->Memory;
	for (i32 Y = 0; Y < Height; ++Y)
	{
		u32 *Pixel = (u32*)Row;

		// Y Terms
		i32 Y_offset = Y + anim_offset;
		i32 dYMove = Y - MouseMoveY;
		i32 dYPress = Y - MousePressY;
		i32 dY2Move = dYMove * dYMove;
		i32 dY2Press = dYPress * dYPress;

		for (i32 X = 0; X < Width; ++X)
		{
			u8 B = (u8)(X + anim_offset);
			u8 G = (u8)(Y_offset);
			u8 R = 0;

			i32 dXMove = X - MouseMoveX;
			i32 dX2Move = dXMove * dXMove;

			if ((dX2Move + dY2Move) < dZ2)
			{
				B = 0; R = 0; G = 0;
			}

			i32 dXPress = X - MousePressX;
			i32 dX2Press = dXPress * dXPress;
			if ((dX2Press + dY2Press) < 100)
			{
				B = 0; R = 255; G = 0;
			}

			//u32 double_word = ;
			//_mm_stream_si32((int*)Pixel++, double_word);
			*Pixel++ = (((u32)R) << 16) | (((u32)G) << 8) | (u32)B | 0;
		}
		Row += Buffer->Pitch;
	}

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