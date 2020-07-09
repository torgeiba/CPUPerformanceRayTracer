#define NOMINMAX

#include "Application.h"

#include "dwmapi.h" // For frame/vsync, DwmFlush();

#pragma comment(lib, "Dwmapi.lib")

#include <iostream>   // std::cout
#include <string>     // std::string, std::to_string
#include <cstdio>
//#include "WindowsX.h" // Some unused macros

#include <emmintrin.h>

#include "mathlib.h"

#include "rendering.h"

#include "demofox_path_tracing_scalar.h"
#include "demofox_path_tracing_scalar_branchless.h"
#include "demofox_path_tracing_simt.h"
#include "demofox_path_tracing_simt_pooled.h"

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

struct Ray
{
	f32x3 Origin;
	f32x3 Direction;
};

struct Sphere
{
	f32x3 Center;
	f32 Radius;
};

bool RaySphereIntersects(Ray ray, Sphere sphere, f32& t0)
{
	f32x3 L = sphere.Center - ray.Origin;
	f32 tca = dot(L, ray.Direction);
	f32 d2 = dot(L, L) - tca * tca;
	f32 r2 = sphere.Radius* sphere.Radius;
	if (d2 > r2) return false;
	f32 thc = sqrtf(r2 - d2);
	t0 = tca - thc;
	f32 t1 = tca + thc;
	if (t0 < 0) t0 = t1;
	if (t0 < 0) return false;
	return true;
}

f32x3 CastRay(Ray r, Sphere s)
{
	float dst = std::numeric_limits<float>::max();
	if (RaySphereIntersects(r, s, dst))
	{
		return{ 0.4f, 0.4f, 0.3f };
	}
	return{ 0.2f, 0.7f, 0.3f };
}

void ApplicationState::Render()
{
	win32_offscreen_buffer* Buffer = &BackBuffer;
	if (HasRenderedThisFrame) return;

	i32 Width = Buffer->Width;
	i32 Height = Buffer->Height;

	f32* RenderTarget = Buffer->RenderTarget;
	//::Render(RenderTarget, Width, Height, 3);
	//DemofoxRenderScalar(RenderTarget, Width, Height, 3);
	//DemofoxRenderScalarBranchless(RenderTarget, Width, Height, 3);


	i32 NumTilesX = 2;
	i32 NumTilesY = 4;
	i32 TileWidth = Width / NumTilesX;
	i32 TileHeight = Height / NumTilesY;
	DemofoxRenderSimtPooled(RenderTarget, Width, Height, NumTilesX, NumTilesY, TileWidth, TileHeight, 3);
	/*
	Sphere sphere = { {-3.f, 0.f, -16.f }, 4.f};

	f32 tanFov = tan(90 / 2.f); // 1

	f32 y_const_term = - tanFov / (f32)Height + tanFov;
	f32 y_const_factor = -tanFov * 2.f / (f32)Height;

	f32 x_const_term = (tanFov / (f32)Height) * (1.f - Width);
	f32 x_const_factor = 2.f * tanFov / (f32)Height;
	
	
	for (i32 Y = 0; Y < Height; ++Y)
	{
		f32 y = Y * y_const_factor + y_const_term;
		
		for (i32 X = 0; X < Width; ++X)
		{
			
			f32 x = X * x_const_factor + x_const_term;
			f32x3 udir = { x, y, -1.f };
			f32x3 dir = normalize(udir);
			f32x3 color = CastRay({{ 0.f, 0.f, 0.f }, dir}, sphere);
			
			RenderTarget[(Y * Width + X) * 3 + 0] = color.x;
			RenderTarget[(Y * Width + X) * 3 + 1] = color.y;
			RenderTarget[(Y * Width + X) * 3 + 2] = color.z;
		}
	}
	*/
	// Wide Copy out
#if 0
	auto clamp = [](f32 x, f32 a, f32 b) { return (x < a) ? a : (x > b ? b : x); };
	u8 *Row = (u8*)Buffer->Memory;
	for (u32 Y = 0; Y < Height; ++Y)
	{
		u32 *Pixel = (u32*)Row;
		for (u32 X = 0; X < Width / LANE_COUNT; ++X)
		{
			u32 RegisterIndex = (Y * Width + X * LANE_COUNT) * 3;
			for (u32 Z = 0; Z < LANE_COUNT; Z++)
			{
				u32 PixelIndex = RegisterIndex + Z;
				u8 R = (u8)((i32)(clamp(RenderTarget[PixelIndex + 0 * LANE_COUNT], 0.f, 1.f) * 255) & 0xFF);
				u8 G = (u8)((i32)(clamp(RenderTarget[PixelIndex + 1 * LANE_COUNT], 0.f, 1.f) * 255) & 0xFF);
				u8 B = (u8)((i32)(clamp(RenderTarget[PixelIndex + 2 * LANE_COUNT], 0.f, 1.f) * 255) & 0xFF);
				Pixel[Z] = (((u32)R) << 16) | (((u32)G) << 8) | (u32)B | 0;
			}

			Pixel += LANE_COUNT;
		}
		Row += Buffer->Pitch;
	}
#elif 1

	auto clamp = [](f32 x, f32 a, f32 b) { return (x < a) ? a : (x > b ? b : x); };
	for (i32 TileX = 0; TileX < NumTilesX; TileX++)
	{
		i32 TileMinX = TileX * TileWidth;
		i32 TileMaxX = TileMinX + (TileWidth - 1);
		for (i32 TileY = 0; TileY < NumTilesY; TileY++)
		{
			i32 TileMinY = TileY * TileHeight;
			i32 TileMaxY = TileMinY + (TileHeight - 1);

			i32 TileSize = TileHeight * TileWidth * 3;
			i32 TileYOffset = TileY * TileHeight * Width * 3;
			i32 TileXOffset = TileSize * TileX;
			i32 BufferTileOffset = TileXOffset + TileYOffset;

			f32* BufferPos = RenderTarget + BufferTileOffset;

			for (i32 Y = TileMinY; Y <= TileMaxY; Y++)
			{
				for (i32 X = TileMinX; X <= TileMaxX; X += LANE_COUNT)
				{
					f32* R = &BufferPos[0];
					f32* G = &BufferPos[1 * LANE_COUNT];
					f32* B = &BufferPos[2 * LANE_COUNT];

					m256x3 FrameColor = m256x3{ load_ps(R), load_ps(G), load_ps(B) };
					u32* Pixel = (u32*)(Buffer->Memory) + ((u64)Y * (u64)Width + (u64)X);
					for (u32 Z = 0; Z < LANE_COUNT; Z++)
					{
						u8 R = (u8)((i32)(clamp(FrameColor.x.m256_f32[Z], 0.f, 1.f) * 255) & 0xFF);
						u8 G = (u8)((i32)(clamp(FrameColor.y.m256_f32[Z], 0.f, 1.f) * 255) & 0xFF);
						u8 B = (u8)((i32)(clamp(FrameColor.z.m256_f32[Z], 0.f, 1.f) * 255) & 0xFF);
						Pixel[Z] = (((u32)R) << 16) | (((u32)G) << 8) | (u32)B | 0;
					}
					BufferPos += LANE_COUNT * 3;
				}
			}
		}
	}

#else
	// Scalar copy out

	auto clamp = [](f32 x, f32 a, f32 b) { return x < a ? a : (x > b ? b : x); };
	u8* Row = (u8*)Buffer->Memory;
	for (i32 Y = 0; Y < Height; ++Y)
	{
		u32* Pixel = (u32*)Row;
		for (i32 X = 0; X < Width; ++X)
		{
			i32 PixelIndex = (Y * Width + X) * 3;
			u8 R = (u8)((i32)(clamp(RenderTarget[PixelIndex + 0], 0.f, 1.f) * 255) & 0xFF);
			u8 G = (u8)((i32)(clamp(RenderTarget[PixelIndex + 1], 0.f, 1.f) * 255) & 0xFF);
			u8 B = (u8)((i32)(clamp(RenderTarget[PixelIndex + 2], 0.f, 1.f) * 255) & 0xFF);
			*Pixel = (((u32)R) << 16) | (((u32)G) << 8) | (u32)B | 0;
			Pixel += 1;
		}
		Row += Buffer->Pitch;
	}
#endif
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