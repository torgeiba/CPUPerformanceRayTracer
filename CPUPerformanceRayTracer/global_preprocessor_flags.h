#pragma once

#define PERFORMANCE_PROFILING_BUILD 1

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
#define USE_ENV_CUBEMAP 1
#define OUTPUT_TO_SCREEN !RENDER_OFFLINE
#define USE_NON_TEMPORAL_STORE 0

#define VISUALIZE_TILES 0

#define USE_FAST_APPROXIMATE_GAMMA 1
#define USE_FAST_APPROXIMATE_EXP 1
#define USE_UNIT_VECTOR_REJECTION_SAMPLING 1

// Threading / Work Queue
#define NUM_THREADS 8

#define NUM_TILES_X 8	
#define NUM_TILES_Y 12

