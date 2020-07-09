#include "emmintrin.h"
#include "xmmintrin.h"
#include "intrin.h"


// https://docs.microsoft.com/en-us/windows/win32/sync/what-s-new-in-synchronization



/*
	Thread pooling
*/

// Vista API thread pools
// https://docs.microsoft.com/en-us/windows/win32/procthread/thread-pools

//old thread pooling
// https://docs.microsoft.com/en-us/windows/win32/procthread/thread-pooling


// https://docs.microsoft.com/en-us/windows/win32/sync/synchronization-objects
// https://docs.microsoft.com/en-us/windows/win32/sync/wait-functions

// Suspend and resume thread:
/*

// Wait for a semaphore to be signaled:

SemaphoreHandle = CreateSemaphoreEx( ..... )

if(no_more_work)
	WaitForSingleObjectEx(SemaphoreHandle, INFINITE, false)


*/

/*
 * Cacheability support
 */
/* // From emmintrin.h
extern void _mm_stream_pd(double* _Dp, __m128d _A);
extern void _mm_stream_si128(__m128i* _P, __m128i _A);
extern void _mm_clflush(void const* _P);
extern void _mm_lfence(void);
extern void _mm_mfence(void);
extern void _mm_stream_si32(int* _P, int _I);
extern void _mm_pause(void);
*/

/*
// From xmmintrin.h
extern void _mm_prefetch(char const*_A, int _Sel);
#if defined(_M_IX86)
extern void _mm_stream_pi(__m64 *, __m64);
#endif
extern void _mm_stream_ps(float *, __m128);
extern __m128 _mm_move_ss(__m128 _A, __m128 _B);

extern void _mm_sfence(void);
extern unsigned int _mm_getcsr(void);
extern void _mm_setcsr(unsigned int);
*/

// https://docs.microsoft.com/en-us/cpp/intrinsics/interlockedcompareexchange-intrinsic-functions?view=vs-2019

/*
// from https://docs.microsoft.com/en-us/cpp/intrinsics/readwritebarrier?view=vs-2019
The _ReadBarrier, _WriteBarrier, and _ReadWriteBarrier compiler intrinsics and the MemoryBarrier macro
are all deprecated and should not be used. For inter-thread communication,
use mechanisms such as atomic_thread_fence and std::atomic<T>, which are defined in the C++ Standard Library.
For hardware access, use the /volatile:iso compiler option together with the volatile keyword.
*/

// From intrin.h
/*
__MACHINEX64(short _InterlockedAnd16_np(short volatile * _Value, short _Mask))
__MACHINEX64(__int64 _InterlockedAnd64_np(__int64 volatile * _Value, __int64 _Mask))
__MACHINEX64(char _InterlockedAnd8_np(char volatile * _Value, char _Mask))
__MACHINEX64(long _InterlockedAnd_np(long volatile * _Value, long _Mask))
__MACHINEARM64_X64(unsigned char _InterlockedCompareExchange128(__int64 volatile * _Destination, __int64 _ExchangeHigh, __int64 _ExchangeLow, __int64 * _ComparandResult))
__MACHINEX64(unsigned char _InterlockedCompareExchange128_np(__int64 volatile * _Destination, __int64 _ExchangeHigh, __int64 _ExchangeLow, __int64 * _ComparandResult))
__MACHINEX64(short _InterlockedCompareExchange16_np(short volatile * _Destination, short _Exchange, short _Comparand))
__MACHINEX64(__int64 _InterlockedCompareExchange64_np(__int64 volatile * _Destination, __int64 _Exchange, __int64 _Comparand))
__MACHINE(void * _InterlockedCompareExchangePointer(void * volatile * _Destination, void * _Exchange, void * _Comparand))
__MACHINEX64(void * _InterlockedCompareExchangePointer_np(void * volatile * _Destination, void * _Exchange, void * _Comparand))
__MACHINEX64(long _InterlockedCompareExchange_np(long volatile * _Destination, long _Exchange, long _Comparand))
__MACHINE(void * _InterlockedExchangePointer(void * volatile * _Target, void * _Value))
__MACHINEX64(short _InterlockedOr16_np(short volatile * _Value, short _Mask))
__MACHINEX64(__int64 _InterlockedOr64_np(__int64 volatile * _Value, __int64 _Mask))
__MACHINEX64(char _InterlockedOr8_np(char volatile * _Value, char _Mask))
__MACHINEX64(long _InterlockedOr_np(long volatile * _Value, long _Mask))
__MACHINEX64(short _InterlockedXor16_np(short volatile * _Value, short _Mask))
__MACHINEX64(__int64 _InterlockedXor64_np(__int64 volatile * _Value, __int64 _Mask))
__MACHINEX64(char _InterlockedXor8_np(char volatile * _Value, char _Mask))
__MACHINEX64(long _InterlockedXor_np(long volatile * _Value, long _Mask))
*/

// https://docs.microsoft.com/en-us/cpp/intrinsics/x64-amd64-intrinsics-list?view=vs-2019
// Interlocked intrinsics
//A [1] indicates the intrinsic is available only on AMD processors. A [2] indicates the intrinsic is available only on Intel processors. A [3] indicates the prototype is a macro.
/*
_InterlockedAnd		intrin.h	long _InterlockedAnd(long volatile*, long)
_InterlockedAnd_HLEAcquire	HLE[2]	immintrin.h	long _InterlockedAnd_HLEAcquire(long volatile*, long)
_InterlockedAnd_HLERelease	HLE[2]	immintrin.h	long _InterlockedAnd_HLERelease(long volatile*, long)
_InterlockedAnd_np		intrin.h	long _InterlockedAnd_np(long*, long)
_InterlockedAnd16		intrin.h	short _InterlockedAnd16(short volatile*, short)
_InterlockedAnd16_np		intrin.h	short _InterlockedAnd16_np(short*, short)
_InterlockedAnd64		intrin.h	__int64 _InterlockedAnd64(__int64 volatile*, __int64)
_InterlockedAnd64_HLEAcquire	HLE[2]	immintrin.h	__int64 _InterlockedAnd64_HLEAcquire(__int64 volatile*, __int64)
_InterlockedAnd64_HLERelease	HLE[2]	immintrin.h	__int64 _InterlockedAnd64_HLERelease(__int64 volatile*, __int64)
_InterlockedAnd64_np		intrin.h	__int64 _InterlockedAnd64_np(__int64*, __int64)
_InterlockedAnd8		intrin.h	char _InterlockedAnd8(char volatile*, char)
_InterlockedAnd8_np		intrin.h	char _InterlockedAnd8_np(char*, char)
_interlockedbittestandreset		intrin.h	unsigned char _interlockedbittestandreset(long*, long)
_interlockedbittestandreset_HLEAcquire	HLE[2]	immintrin.h	unsigned char _interlockedbittestandreset_HLEAcquire(long*, long)
_interlockedbittestandreset_HLERelease	HLE[2]	immintrin.h	unsigned char _interlockedbittestandreset_HLERelease(long*, long)
_interlockedbittestandreset64		intrin.h	unsigned char _interlockedbittestandreset64(__int64*, __int64)
_interlockedbittestandreset64_HLEAcquire	HLE[2]	immintrin.h	unsigned char _interlockedbittestandreset64_HLEAcquire(__int64*, __int64)
_interlockedbittestandreset64_HLERelease	HLE[2]	immintrin.h	unsigned char _interlockedbittestandreset64_HLERelease(__int64*, __int64)
_interlockedbittestandset		intrin.h	unsigned char _interlockedbittestandset(long*, long)
_interlockedbittestandset_HLEAcquire	HLE[2]	immintrin.h	unsigned char _interlockedbittestandset_HLEAcquire(long*, long)
_interlockedbittestandset_HLERelease	HLE[2]	immintrin.h	unsigned char _interlockedbittestandset_HLERelease(long*, long)
_interlockedbittestandset64		intrin.h	unsigned char _interlockedbittestandset64(__int64*, __int64)
_interlockedbittestandset64_HLEAcquire	HLE[2]	immintrin.h	unsigned char _interlockedbittestandset64_HLEAcquire(__int64*, __int64)
_interlockedbittestandset64_HLERelease	HLE[2]	immintrin.h	unsigned char _interlockedbittestandset64_HLERelease(__int64*, __int64)
_InterlockedCompareExchange		intrin.h	long _InterlockedCompareExchange(long volatile*, long, long)
_InterlockedCompareExchange_HLEAcquire	HLE[2]	immintrin.h	long _InterlockedCompareExchange_HLEAcquire(long volatile*, long, long)
_InterlockedCompareExchange_HLERelease	HLE[2]	immintrin.h	long _InterlockedCompareExchange_HLERelease(long volatile*, long, long)
_InterlockedCompareExchange_np		intrin.h	long _InterlockedCompareExchange_np(long*, long, long)
_InterlockedCompareExchange128		intrin.h	unsigned char _InterlockedCompareExchange128(__int64 volatile*, __int64, __int64, __int64*)
_InterlockedCompareExchange128_np		intrin.h	unsigned char _InterlockedCompareExchange128(__int64 volatile*, __int64, __int64, __int64*)
_InterlockedCompareExchange16		intrin.h	short _InterlockedCompareExchange16(short volatile*, short, short)
_InterlockedCompareExchange16_np		intrin.h	short _InterlockedCompareExchange16_np(short volatile*, short, short)
_InterlockedCompareExchange64		intrin.h	__int64 _InterlockedCompareExchange64(__int64 volatile*, __int64, __int64)
_InterlockedCompareExchange64_HLEAcquire	HLE[2]	immintrin.h	__int64 _InterlockedCompareExchange64_HLEAcquire(__int64 volatile*, __int64, __int64)
_InterlockedCompareExchange64_HLERelease	HLE[2]	immintrin.h	__int64 _InterlockedCompareExchange64_HLERelease(__int64 volatile*, __int64, __int64)
_InterlockedCompareExchange64_np		intrin.h	__int64 _InterlockedCompareExchange64_np(__int64*, __int64, __int64)
_InterlockedCompareExchange8		intrin.h	char _InterlockedCompareExchange8(char volatile*, char, char)
_InterlockedCompareExchangePointer		intrin.h	void* _InterlockedCompareExchangePointer(void* volatile*, void*, void*)
_InterlockedCompareExchangePointer_HLEAcquire	HLE[2]	immintrin.h	void* _InterlockedCompareExchangePointer_HLEAcquire(void* volatile*, void*, void*)
_InterlockedCompareExchangePointer_HLERelease	HLE[2]	immintrin.h	void* _InterlockedCompareExchangePointer_HLERelease(void* volatile*, void*, void*)
_InterlockedCompareExchangePointer_np		intrin.h	void* _InterlockedCompareExchangePointer_np(void**, void*, void*)
_InterlockedDecrement		intrin.h	long _InterlockedDecrement(long volatile*)
_InterlockedDecrement16		intrin.h	short _InterlockedDecrement16(short volatile*)
_InterlockedDecrement64		intrin.h	__int64 _InterlockedDecrement64(__int64 volatile*)
_InterlockedExchange		intrin.h	long _InterlockedExchange(long volatile*, long)
_InterlockedExchange_HLEAcquire	HLE[2]	immintrin.h	long _InterlockedExchange_HLEAcquire(long volatile*, long)
_InterlockedExchange_HLERelease	HLE[2]	immintrin.h	long _InterlockedExchange_HLERelease(long volatile*, long)
_InterlockedExchange16		intrin.h	short _InterlockedExchange16(short volatile*, short)
_InterlockedExchange64		intrin.h	__int64 _InterlockedExchange64(__int64 volatile*, __int64)
_InterlockedExchange64_HLEAcquire	HLE[2]	immintrin.h	__int64 _InterlockedExchange64_HLEAcquire(__int64 volatile*, __int64)
_InterlockedExchange64_HLERelease	HLE[2]	immintrin.h	__int64 _InterlockedExchange64_HLERelease(__int64 volatile*, __int64)
_InterlockedExchange8		intrin.h	char _InterlockedExchange8(char volatile*, char)
_InterlockedExchangeAdd		intrin.h	long _InterlockedExchangeAdd(long volatile*, long)
_InterlockedExchangeAdd_HLEAcquire	HLE[2]	immintrin.h	long _InterlockedExchangeAdd_HLEAcquire(long volatile*, long)
_InterlockedExchangeAdd_HLERelease	HLE[2]	immintrin.h	long _InterlockedExchangeAdd_HLERelease(long volatile*, long)
_InterlockedExchangeAdd16		intrin.h	short _InterlockedExchangeAdd16(short volatile*, short)
_InterlockedExchangeAdd64		intrin.h	__int64 _InterlockedExchangeAdd64(__int64 volatile*, __int64)
_InterlockedExchangeAdd64_HLEAcquire	HLE[2]	immintrin.h	__int64 _InterlockedExchangeAdd64_HLEAcquire(__int64 volatile*, __int64)
_InterlockedExchangeAdd64_HLERelease	HLE[2]	immintrin.h	__int64 _InterlockedExchangeAdd64_HLERelease(__int64 volatile*, __int64)
_InterlockedExchangeAdd8		intrin.h	char _InterlockedExchangeAdd8(char volatile*, char)
_InterlockedExchangePointer		intrin.h	void* _InterlockedExchangePointer(void* volatile*, void*)
_InterlockedExchangePointer_HLEAcquire	HLE[2]	immintrin.h	void* _InterlockedExchangePointer_HLEAcquire(void* volatile*, void*)
_InterlockedExchangePointer_HLERelease	HLE[2]	immintrin.h	void* _InterlockedExchangePointer_HLERelease(void* volatile*, void*)
_InterlockedIncrement		intrin.h	long _InterlockedIncrement(long volatile*)
_InterlockedIncrement16		intrin.h	short _InterlockedIncrement16(short volatile*)
_InterlockedIncrement64		intrin.h	__int64 _InterlockedIncrement64(__int64 volatile*)
_InterlockedOr		intrin.h	long _InterlockedOr(long volatile*, long)
_InterlockedOr_HLEAcquire	HLE[2]	immintrin.h	long _InterlockedOr_HLEAcquire(long volatile*, long)
_InterlockedOr_HLERelease	HLE[2]	immintrin.h	long _InterlockedOr_HLERelease(long volatile*, long)
_InterlockedOr_np		intrin.h	long _InterlockedOr_np(long*, long)
_InterlockedOr16		intrin.h	short _InterlockedOr16(short volatile*, short)
_InterlockedOr16_np		intrin.h	short _InterlockedOr16_np(short*, short)
_InterlockedOr64		intrin.h	__int64 _InterlockedOr64(__int64 volatile*, __int64)
_InterlockedOr64_HLEAcquire	HLE[2]	immintrin.h	__int64 _InterlockedOr64_HLEAcquire(__int64 volatile*, __int64)
_InterlockedOr64_HLERelease	HLE[2]	immintrin.h	__int64 _InterlockedOr64_HLERelease(__int64 volatile*, __int64)
_InterlockedOr64_np		intrin.h	__int64 _InterlockedOr64_np(__int64*, __int64)
_InterlockedOr8		intrin.h	char _InterlockedOr8(char volatile*, char)
_InterlockedOr8_np		intrin.h	char _InterlockedOr8_np(char*, char)
_InterlockedXor		intrin.h	long _InterlockedXor(long volatile*, long)
_InterlockedXor_HLEAcquire	HLE[2]	immintrin.h	long _InterlockedXor_HLEAcquire(long volatile*, long)
_InterlockedXor_HLERelease	HLE[2]	immintrin.h	long _InterlockedXor_HLERelease(long volatile*, long)
_InterlockedXor_np		intrin.h	long _InterlockedXor_np(long*, long)
_InterlockedXor16		intrin.h	short _InterlockedXor16(short volatile*, short)
_InterlockedXor16_np		intrin.h	short _InterlockedXor16_np(short*, short)
_InterlockedXor64		intrin.h	__int64 _InterlockedXor64(__int64 volatile*, __int64)
_InterlockedXor64_HLEAcquire	HLE[2]	immintrin.h	__int64 _InterlockedXor64_HLEAcquire(__int64 volatile*, __int64)
_InterlockedXor64_HLERelease	HLE[2]	immintrin.h	__int64 _InterlockedXor64_HLERelease(__int64 volatile*, __int64)
_InterlockedXor64_np		intrin.h	__int64 _InterlockedXor64_np(__int64*, __int64)
_InterlockedXor8		intrin.h	char _InterlockedXor8(char volatile*, char)
_InterlockedXor8_np
*/