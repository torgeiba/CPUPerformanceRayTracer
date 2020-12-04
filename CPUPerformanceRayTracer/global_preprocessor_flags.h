#pragma once

#define PERFORMANCE_PROFILING_BUILD 0

// Application
#if PERFORMANCE_PROFILING_BUILD
	#define RENDER_OFFLINE 1
#else 
	#define RENDER_OFFLINE 0
#endif

#if RENDER_OFFLINE
	#define USE_VSYNC 0
#else
	#define USE_VSYNC 1
#endif


#define SHOW_FRAMETIMES !RENDER_OFFLINE

#if PERFORMANCE_PROFILING_BUILD
	#define OUTPUT_MODE_SILENT 1              // Toggles on / off writes to console for Offline rendering mode ( useful for profiling )
#else
	#if RENDER_OFFLINE
		#define OUTPUT_MODE_SILENT 0
	#endif
#endif

#if RENDER_OFFLINE
	#define NUM_SAMPLES_PER_FRAME 1
	#define NUM_FRAMES_TO_RENDER 600
#else
	#define NUM_SAMPLES_PER_FRAME 1
	#define NUM_FRAMES_TO_RENDER 1
#endif

#define USE_CLIENT_RESOLUTION_AS_BACKBUFFER_RESOLUTION 1 // always set this to true for now, since output BMP buffer and f32 rendertarget is assumed to be same size, but actual client ( window ) size can vary

#define RENDER_BUFFER_PIXEL_HEIGHT 720
#define RENDER_BUFFER_PIXEL_WIDTH  1280


//#define RENDER_BUFFER_PIXEL_HEIGHT 1080
//#define RENDER_BUFFER_PIXEL_WIDTH  1920

//#define RENDER_BUFFER_PIXEL_HEIGHT 2160
//#define RENDER_BUFFER_PIXEL_WIDTH  3840

#define WINDOW_CLIENT_PIXEL_HEIGHT 720
#define WINDOW_CLIENT_PIXEL_WIDTH  1280

//#define WINDOW_CLIENT_PIXEL_HEIGHT 1080
//#define WINDOW_CLIENT_PIXEL_WIDTH  1920

// Rendering
#define USE_ENV_MAP 1
#define USE_ENV_CUBEMAP 0
#define OUTPUT_TO_SCREEN !RENDER_OFFLINE
#define USE_NON_TEMPORAL_STORE 0
#define ACCUMULATE_FRAMES 1

#define USE_FAST_APPROXIMATE_GAMMA 1
#define USE_FAST_APPROXIMATE_ACES_TONEMAP 1
#define USE_FAST_APPROXIMATE_EXP 1
#define USE_UNIT_VECTOR_REJECTION_SAMPLING 1
#define USE_RANDOM_JITTER_TEXTURE_SAMPLING 1

// Threading / Work Queue
#define NUM_THREADS 8

// Tiling
#define VISUALIZE_TILES 0
//#define NUM_TILES_X 8	
//#define NUM_TILES_Y 12

// 2 x m256 = 3 cache lines
// 1280px / (2 * 8px) = 80
// factors(80) = 2 * 2 * 2 * 2 * 5

// factors(80) = 2 * 2 * 2 * 2 * 5
// factors(720) = 2 * 2 * 2 * 2 * * 3 * 3 * 5
// common_factors(80, 720) = 2 * 2 * 2 * 2 * 5
// multiply odd factors to retain even pow2 tile sizes
// 5 and 3*3*5
#define NUM_TILES_X 10	
#define NUM_TILES_Y 15




// Memory Alignment settings
// See: https://docs.microsoft.com/en-us/cpp/cpp/align-cpp?view=msvc-160

#define CACHE_LINE_SIZE_BYTES  64
#define CACHE_ALIGN __declspec(align(CACHE_LINE_SIZE_BYTES))

#define THREAD_LOCAL_STORAGE __declspec(thread)

// https://docs.microsoft.com/en-us/cpp/cpp/extension-restrict?view=msvc-160
#define RESTRICT_PTR __restrict

// https://docs.microsoft.com/en-us/cpp/cpp/restrict?view=msvc-160
#define RESTRICT_FUNC __declspec(restrict)


// TODO: This causes memory access violation, figure out why.
// Tested: Does not seem to make a difference between _aligned_malloc and malloc
// VirtualAlloc works fine, but that allocates 4k pages minimum, so it might hide some out of bounds issue somewhere
#define USE_ALIGNED_MALLOC 1