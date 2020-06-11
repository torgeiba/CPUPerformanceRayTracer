#include "utils.h"

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "dwmapi.h" // For frame/vsync, DwmFlush();

static i64 Global_QPC_Frequency;

i64 GetPerformanceCounter()
{
	LARGE_INTEGER PerformanceCounter;
	QueryPerformanceCounter(&PerformanceCounter);
	return PerformanceCounter.QuadPart;
}

f64 GetPerformanceCounterIntervalSeconds(i64 Start, i64 End)
{
	return ((f64)End - (f64)Start) / (f64)Global_QPC_Frequency;
}

void InitializeQueryPerformanceCounter()
{
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	Global_QPC_Frequency = Frequency.QuadPart;
}

i64 GetPerformanceCounterFrequency()
{
	return Global_QPC_Frequency;
}

f64 GetRefreshRate()
{
	DWM_TIMING_INFO TimingInfo = {};
	TimingInfo.cbSize = sizeof(TimingInfo); // The cbSize member of this structure must be set before DwmGetCompositionTimingInfo is called.
	if (FAILED(DwmGetCompositionTimingInfo(NULL, &TimingInfo))) { /* Error */ } // Note HWND must be null according to docs

	UNSIGNED_RATIO RefreshRateRatio = TimingInfo.rateRefresh;

	u32 RefreshNumerator = RefreshRateRatio.uiNumerator;
	u32 RefreshDenominator = RefreshRateRatio.uiDenominator;

	f64 RefreshRateFloat = (f64)RefreshNumerator / (f64)RefreshDenominator;
	return RefreshRateFloat;
}