#pragma once
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "utils.h"

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void *Memory;
	i32 Width;
	i32 Height;
	i32 BytesPerChannel;
	i32 NumChannels;
	i32 BytesPerPixel;
	i32 Pitch;

	f32* RenderTarget;

	void Resize(i32 Width, i32 Height);
};

struct mouse_coords
{
	i32 XPos;
	i32 YPos;
};

struct ApplicationState
{
	HWND Window = nullptr;

	bool Running;
	i32 anim_offset;
	win32_offscreen_buffer BackBuffer;
	i64 Global_QPC_Frequency;
	bool HasRenderedThisFrame;
	mouse_coords MouseMoveCoords;
	mouse_coords MouseLDownCoords;
	mouse_coords MouseLUpCoords;
	mouse_coords MouseMDownCoords;
	mouse_coords MouseMUpCoords;
	mouse_coords MouseRDownCoords;
	mouse_coords MouseRUpCoords;
	mouse_coords MouseXDownCoords;
	mouse_coords MouseXUpCoords;

	i32 CurrentWindowWidth;
	i32 CurrentWindowHeight;
	i32 Global_Wheel_Value;

	void Render();
	LRESULT Win32MainWindowCallback(HWND Window, u32 Message, u64 WParam, i64 LParam);

	void RunApp(HINSTANCE Instance, i32 ShowCode);

private:
	void RunMessageLoop();
};

extern ApplicationState App;