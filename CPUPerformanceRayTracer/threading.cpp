#include "threading.h"

#include "emmintrin.h"
#include "xmmintrin.h"
#include "intrin.h"

//#include <ppl.h>

#include "work_queue.h"

// https://docs.microsoft.com/en-us/cpp/parallel/concrt/concurrency-runtime?view=vs-2019

// Write barrier compiler intrinsic marker (not function calls)

// memory fence ( processor should not reorder across a fence )
// _mm_mfence() mem   // (x64 might retire writes in order anyway)
// _mm_lfence() load
// _mm_sfence() store // (x64 might retire writes in order anyway)
// https://community.intel.com/t5/Intel-C-Compiler/what-is-the-effects-of-mm-lfence-and-mm-sfence/td-p/871883

// Specifically:
/*
"The _mm_?fence therefore serves to purposes: 1) inform the compiler of the requirement of pending reads or writes not to be moved before or after the specified fence statement.
 And 2) the compiler is to insert an appropriate processor fence instruction, or lacking that a function call to perform the equivalent fencing behavior."
*/

/*
// https://stackoverflow.com/questions/4537753/when-should-i-use-mm-sfence-mm-lfence-and-mm-mfence
All of this is because x86 has a strong memory model, but C++ has a weak memory model. Preventing compile-time reordering is all you need to do.
Inserting lfence or sfence may not hurt performance much, but they're not necessary if you haven't used weakly-ordered MOVNT loads or stores

NOTE: dubious info: according to handmade hero day 125 the memory model has been *weakened*
*/
#define CompletePastWritesBeforeFutureWrites _WriteBarrier();  _mm_sfence();
#define CompletePastReadsBeforeFutureReads _ReadBarrier();    _mm_lfence();



//// Handmade Hero day 123
//#if 0
//struct work_queue_entry
//{
//	char* StringToPrint;
//};
//
//static u32 NextEntryToDo;
//static u32 EntryCount;
//static work_queue_entry Entries[256];
//
//struct win32_thread_info
//{
//	i32 logicalthreadindex;
//
//};
//
//DWORD WINAPI
//ThreadProc(LPVOID lpParameter)
//{
//	win32_thread_info* ThreadInfo = (win32_thread_info*)lpParameter;
//
//	for (;;)
//	{
//		if (NextEntryToDo < EntryCount)
//		{
//			// TODO: This line is not interlocked, so two threads could see the same value
//			// TODO: Compiler doesn't know that multiple threads could write this value ( volatile )
//			i32 EntryIndex = NextEntryToDo++;
//
//			// TODO: These reads are not in order same problems as the writes
//			work_queue_entry* Entry = Entries + EntryIndex;
//
//			char Buffer[256];
//
//			wsprintf(Buffer, "Thread %u: %s\n", ThreadInfo->logicalthreadindex, Entry->StringToPrint);
//
//			OutputDebugStringA(Entry->StringToPrint); // This has internal sync
//		}
//
//
//		//Sleep(1000);
//	}
//
//}
//
//static void PushString(char* String)
//{
//	// TODO: These writes are not in order
//	// E.g when entrycount is incremented, another thread might see it
//	// and use it in the MakeSomeThreads() loop
//	// If that is the case, had better make sure StringToPrint is already filled out,
//	// otherwise it is going to go and get garbage
//	work_queue_entry* Entry = Entries + EntryCount++;
//	Entry->StringToPrint = String;
//}
//
//#define NUM_THREADS 4
//
//void MakeSomeThreads()
//{
//	win32_thread_info ThreadInfo[NUM_THREADS] = {};
//	for (i32 ThreadIndex = 0; ThreadIndex < NUM_THREADS; ThreadIndex++)
//	{
//		win32_thread_info* Info = ThreadInfo + ThreadIndex;
//		Info->logicalthreadindex = ThreadIndex;
//
//		DWORD ThreadID;
//		HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, &Info, 0, &ThreadID);
//	}
//
//	PushString("String 0");
//	PushString("String 1");
//	PushString("String 2");
//	PushString("String 3");
//	PushString("String 4");
//	PushString("String 5");
//	PushString("String 6");
//	PushString("String 7");
//	PushString("String 8");
//	PushString("String 9");
//}
//#endif
//
//// Handmade Hero day 124
//#if 0
//struct work_queue_entry
//{
//	char* StringToPrint;
//};
//
//static u32 volatile NextEntryToDo;
//static u32 volatile EntryCount;
//
//static u32 volatile EntryCompletionCount;
//
//static work_queue_entry Entries[256];
//
//struct win32_thread_info
//{
//	HANDLE SemaphoreHandle;
//	i32 logicalthreadindex;
//
//};
//
//DWORD WINAPI
//ThreadProc(LPVOID lpParameter)
//{
//	win32_thread_info* ThreadInfo = (win32_thread_info*)lpParameter;
//
//	for (;;)
//	{
//		// NextEntryToDo may not be reloaded each time
//		// should be volatile to make sure it is always reloaded
//		if (NextEntryToDo < EntryCount)
//		{
//			// TODO: This line is not interlocked, so two threads could see the same value
//			// TODO: Compiler doesn't know that multiple threads could write this value ( volatile )
//			// Fix: use interlockedadd (interlockedcompareexchange)
//			// InterlockedIncrement actually functions as a sync fence
//			i32 EntryIndex = InterlockedIncrement((LONG volatile*) &NextEntryToDo)-1;
//
//			// TODO: These reads are not in order same problems as the writes
//			// Fix: make sure past reads are completed before performing this
//			CompletePastReadsBeforeFutureReads; // Probably redundant because of interlockedincrement
//			work_queue_entry* Entry = Entries + EntryIndex;
//
//			char Buffer[256];
//
//			wsprintf(Buffer, "Thread %u: %s\n", ThreadInfo->logicalthreadindex, Entry->StringToPrint);
//
//			OutputDebugStringA(Buffer); // This has internal sync
//
//			InterlockedIncrement((LONG volatile*)&EntryCompletionCount);
//		}
//		else
//		{
//			// TODO: how to wait here (we are done doing work, please suspend)
//			WaitForSingleObjectEx(ThreadInfo->SemaphoreHandle, INFINITE, FALSE);
//		}
//	}
//
//}
//
//
//
//static void PushString(HANDLE SemaphoreHandle, char* String)
//{
//	// TODO: These writes are not in order
//	// (Compiler issue)
//	// E.g when entrycount is incremented, another thread might see it
//	// and use it in the MakeSomeThreads() loop
//	// If that is the case, had better make sure StringToPrint is already filled out,
//	// otherwise it is going to go and get garbage
//
//	// Fix: Make sure to write to StringToPrint before incrementing EntryCount
//	// Must also ensure that the optimizing compiler  does not rearrange the increment etc
//	work_queue_entry* Entry = Entries + EntryCount;
//	Entry->StringToPrint = String;
//
//	CompletePastWritesBeforeFutureWrites; //(WriteBarrier)
//
//	EntryCount++;
//
//	// TODO: Some way to wake up our threads
//	// Need to be careful here, if notify happens before wait, we could end up
//	// not notifying anything that wasn't already running, and then risking waiting for ever later
//	// Fix: use semaphore
//	// TODO: Caveat here - all threads about to sleep, etc
//	/*_In_	 */   LONG lReleaseCount = 1;
//	/*_Out_opt_*/ LONG lpPreviousCount;
//	ReleaseSemaphore(SemaphoreHandle, lReleaseCount, &lpPreviousCount);
//}
//
//#define NUM_THREADS 4
//
//void MakeSomeThreads()
//{
//
//	/*_In_opt_   */ LPSECURITY_ATTRIBUTES lpSemaphoreAttributes = 0;
//	/*_In_       */ LONG lInitialCount = 0;
//	/*_In_       */ LONG lMaximumCount = NUM_THREADS;
//	/*_In_opt_   */ LPCSTR lpName = 0;
//	/*_Reserved_ */ DWORD dwFlags = 0;
//	/*_In_       */ DWORD dwDesiredAccess = SEMAPHORE_ALL_ACCESS;
//	HANDLE SemaphoreHandle = CreateSemaphoreExA(
//		lpSemaphoreAttributes,
//		lInitialCount,
//		lMaximumCount,
//		lpName,
//		dwFlags,
//		dwDesiredAccess
//	);
//
//	win32_thread_info ThreadInfo[NUM_THREADS] = {};
//	for (i32 ThreadIndex = 0; ThreadIndex < NUM_THREADS; ThreadIndex++)
//	{
//		win32_thread_info* Info = ThreadInfo + ThreadIndex;
//		Info->SemaphoreHandle = SemaphoreHandle;
//		Info->logicalthreadindex = ThreadIndex;
//
//		DWORD ThreadID;
//		HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Info, 0, &ThreadID);
//	}
//
//	PushString(SemaphoreHandle, "String A0");
//	PushString(SemaphoreHandle, "String A1");
//	PushString(SemaphoreHandle, "String A2");
//	PushString(SemaphoreHandle, "String A3");
//	PushString(SemaphoreHandle, "String A4");
//	PushString(SemaphoreHandle, "String A5");
//	PushString(SemaphoreHandle, "String A6");
//	PushString(SemaphoreHandle, "String A7");
//	PushString(SemaphoreHandle, "String A8");
//	PushString(SemaphoreHandle, "String A9");
//
//	//Sleep(5000);
//
//	PushString(SemaphoreHandle, "String B0");
//	PushString(SemaphoreHandle, "String B1");
//	PushString(SemaphoreHandle, "String B2");
//	PushString(SemaphoreHandle, "String B3");
//	PushString(SemaphoreHandle, "String B4");
//	PushString(SemaphoreHandle, "String B5");
//	PushString(SemaphoreHandle, "String B6");
//	PushString(SemaphoreHandle, "String B7");
//	PushString(SemaphoreHandle, "String B8");
//	PushString(SemaphoreHandle, "String B9");
//
//	// TODO: replace with wait
//	// TODO turn this into something waitable
//	while (EntryCount != EntryCompletionCount); // spinlock
//
//}
//#endif

//// Handmade Hero day 125
//#if 1
//
//
//
//
//struct work_queue_entry
//{
//	bool IsValid;
//	void* Data;
//};
//static work_queue_entry CompleteAndGetNextWorkQueueEntry(work_queue* Queue, work_queue_entry Completed)
//{
//
//	if (Completed.IsValid)
//	{
//		InterlockedIncrement((LONG volatile*)&Queue->EntryCompletionCount);
//	}
//
//	work_queue_entry Entry;
//	Entry.IsValid = Queue->NextEntryToDo < Queue->EntryCount;
//	if (Entry.IsValid)
//	{
//		u32 Index = InterlockedIncrement((LONG volatile*)&Queue->NextEntryToDo) - 1;
//		Entry.Data = Queue->Entries[Index].UserPointer;
//		_ReadBarrier();    _mm_lfence();
//	}
//
//	return Entry;
//}
//
////static void MarkQueueEntryCompleted(work_queue* Queue, work_queue_entry Item)
////{
//////	InterlockedIncrement((LONG volatile*)&Queue->NextEntryToDo) - 1;
////	//InterlockedIncrement((LONG volatile*)&EntryCompletionCount);
////}
//
//static bool QueueWorkStillInProgress(work_queue* Queue)
//{
//	return Queue->EntryCount != Queue->EntryCompletionCount;// { DoWorkerWork(NUM_THREADS); }
//}
//
//#include <assert.h>
//inline void DoWorkerWork(work_queue_entry Entry, i32 LogicalThreadIndex)
//{
//	assert(Entry.IsValid);
//
//	char Buffer[256];
//	wsprintf(Buffer, "Thread %u: %s\n", LogicalThreadIndex, (char*)Entry.Data);	
//	OutputDebugStringA(Buffer); // This has internal sync
//}
//
//struct win32_thread_info
//{
//	HANDLE SemaphoreHandle;
//	work_queue *Queue;
//	i32 logicalthreadindex;
//};
//DWORD WINAPI
//ThreadProc(LPVOID lpParameter)
//{
//	win32_thread_info* ThreadInfo = (win32_thread_info*)lpParameter;
//
//	work_queue_entry Entry{};
//	for (;;)
//	{
//		// NextEntryToDo may not be reloaded each time
//		// should be volatile to make sure it is always reloaded
//
//		Entry = CompleteAndGetNextWorkQueueEntry(ThreadInfo->Queue, Entry);
//		if (Entry.IsValid)
//		{
//			DoWorkerWork(Entry, ThreadInfo->logicalthreadindex);
//		}
//		else
//		{
//		/*{
//
//		}
//		if (!DoWorkerWork(ThreadInfo->Queue, ThreadInfo->logicalthreadindex))
//		{*/
//			// TODO: how to wait here (we are done doing work, please suspend)
//			WaitForSingleObjectEx(ThreadInfo->Queue->SemaphoreHandle, INFINITE, FALSE);
//		}
//	}
//}
//
//static u32 GetNextAvailableWorkQueueIndex(work_queue *Queue)
//{
//	return Queue->EntryCount;
//}
//
//static void AddWorkQueueEntry(work_queue *Queue, void *Pointer) // former PushString
//{
//	Queue->Entries[Queue->EntryCount].UserPointer = Pointer;
//
//	_WriteBarrier();
//	_mm_sfence();
//	Queue->EntryCount++;
//	LONG lReleaseCount = 1;
//	LONG lpPreviousCount;
//	ReleaseSemaphore(Queue->SemaphoreHandle, lReleaseCount, &lpPreviousCount);
//}
//
//static void PushString(work_queue* Queue, char* String)
//{
//	AddWorkQueueEntry(Queue, String);
//}
//
//
//#define NUM_THREADS 4
//void MakeSomeThreads()
//{
//	work_queue Queue{};
//	/*_In_opt_   */ LPSECURITY_ATTRIBUTES lpSemaphoreAttributes = 0;
//	/*_In_       */ LONG lInitialCount = 0;
//	/*_In_       */ LONG lMaximumCount = NUM_THREADS;
//	/*_In_opt_   */ LPCSTR lpName = 0;
//	/*_Reserved_ */ DWORD dwFlags = 0;
//	/*_In_       */ DWORD dwDesiredAccess = SEMAPHORE_ALL_ACCESS;
//	Queue.SemaphoreHandle = CreateSemaphoreExA(
//		lpSemaphoreAttributes,
//		lInitialCount,
//		lMaximumCount,
//		lpName,
//		dwFlags,
//		dwDesiredAccess
//	);
//
//	win32_thread_info ThreadInfo[NUM_THREADS] = {};
//	for (i32 ThreadIndex = 0; ThreadIndex < NUM_THREADS; ThreadIndex++)
//	{
//		win32_thread_info* Info = ThreadInfo + ThreadIndex;
//		Info->logicalthreadindex = ThreadIndex;
//		Info->Queue = &Queue;
//		DWORD ThreadID;
//		HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Info, 0, &ThreadID);
//	}
//
//	PushString(&Queue, "String A0");
//	PushString(&Queue, "String A1");
//	PushString(&Queue, "String A2");
//	PushString(&Queue, "String A3");
//	PushString(&Queue, "String A4");
//	PushString(&Queue, "String A5");
//	PushString(&Queue, "String A6");
//	PushString(&Queue, "String A7");
//	PushString(&Queue, "String A8");
//	PushString(&Queue, "String A9");
//	PushString(&Queue, "String B0");
//	PushString(&Queue, "String B1");
//	PushString(&Queue, "String B2");
//	PushString(&Queue, "String B3");
//	PushString(&Queue, "String B4");
//	PushString(&Queue, "String B5");
//	PushString(&Queue, "String B6");
//	PushString(&Queue, "String B7");
//	PushString(&Queue, "String B8");
//	PushString(&Queue, "String B9");
//
//	work_queue_entry Entry = {};
//	while (QueueWorkStillInProgress(&Queue));
//	{
//		Entry = CompleteAndGetNextWorkQueueEntry(&Queue, Entry);
//		if (Entry.IsValid)
//		{
//			DoWorkerWork(Entry, NUM_THREADS);
//		}
//	}
//
//}
//#endif

// Handmade Hero day 126
#if 1

//#define NUM_THREADS 4
//
//static bool DoNextWorkQueueEntry(work_queue* Queue)
//{
//
//	u32 OriginalNextEntryToRead = Queue->NextEntryToRead;
//
//	u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % Queue->MaxEntryCount;
//	bool IsValid = OriginalNextEntryToRead != Queue->NextEntryToWrite;
//	bool bShouldWait = !IsValid;
//	if (IsValid)
//	{
//		u32 Index = InterlockedCompareExchange((LONG volatile*)&Queue->NextEntryToRead,
//			NewNextEntryToRead,
//			OriginalNextEntryToRead);
//
//		// If this holds, we are ok, else some other thread has beaten us to getting the work
//		if (Index == OriginalNextEntryToRead)
//		{
//			work_queue_entry Entry = Queue->Entries[Index];
//			Entry.Callback(Queue, Entry.Data);
//			// interlocked increment does the correct fence //_ReadWriteBarrier();    _mm_mfence();
//			InterlockedIncrement((LONG volatile*)&Queue->EntryCompletionCount);
//		}
//	}
//
//	return bShouldWait;
//}
//
//
//// TODO make this return a bool if it fails and queue is full
//void AddWorkQueueEntry(work_queue* Queue, work_queue_callback* Callback, void* Data)
//{
//	//TODO: for multiple producers add interlocked compare exchange to entrycount
//	// TODO: Switch to interlockedcompare exchange eventually so that any thread can add?
//	u32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % Queue->MaxEntryCount;
//	assert(NewNextEntryToWrite != Queue->NextEntryToRead);
//	work_queue_entry* Entry = Queue->Entries + Queue->NextEntryToWrite;
//	Entry->Data = Data;
//	Entry->Callback = Callback;
//	Queue->CompletionGoal++;
//	_WriteBarrier(); // Redundant if using sfence, VS will treat mm_sfence as a writebarrier
//	// _mm_sfence();    // Unneccesary for x64
//	Queue->NextEntryToWrite = NewNextEntryToWrite;
//	LONG lReleaseCount = 1;
//	LONG lpPreviousCount;
//	ReleaseSemaphore(Queue->SemaphoreHandle, lReleaseCount, &lpPreviousCount);
//}
//
//static bool QueueWorkStillInProgress(work_queue* Queue)
//{
//	return Queue->CompletionGoal != Queue->EntryCompletionCount;// { DoWorkerWork(NUM_THREADS); }
//}
//void CompleteAllWork(work_queue* Queue)
//{
//	while (QueueWorkStillInProgress(Queue));
//	{
//		DoNextWorkQueueEntry(Queue);
//	}
//	Queue->EntryCompletionCount = 0;
//	Queue->CompletionGoal = 0;
//}
//
//struct win32_thread_info
//{
//	work_queue* Queue;
//	i32 logicalthreadindex;
//};
//DWORD WINAPI
//ThreadProc(LPVOID lpParameter)
//{
//	win32_thread_info* ThreadInfo = (win32_thread_info*)lpParameter;
//	for (;;)
//	{
//		if(DoNextWorkQueueEntry(ThreadInfo->Queue))
//		{
//			WaitForSingleObjectEx(ThreadInfo->Queue->SemaphoreHandle, INFINITE, FALSE);
//		}
//	}
//}
//
//work_queue MakeWorkQueue()
//{
//	work_queue Queue{};
//	LPSECURITY_ATTRIBUTES SemaphoreAttributes = 0;
//	LONG InitialCount = 0;
//	LONG MaximumCount = NUM_THREADS;
//	LPCSTR Name = 0;
//	DWORD Flags = 0;
//	DWORD DesiredAccess = SEMAPHORE_ALL_ACCESS;
//	Queue.SemaphoreHandle = CreateSemaphoreExA(
//		SemaphoreAttributes,
//		InitialCount,
//		MaximumCount,
//		Name,
//		Flags,
//		DesiredAccess
//	);
//
//	win32_thread_info ThreadInfo[NUM_THREADS] = {};
//	for (i32 ThreadIndex = 0; ThreadIndex < NUM_THREADS; ThreadIndex++)
//	{
//		win32_thread_info* Info = ThreadInfo + ThreadIndex;
//		Info->logicalthreadindex = ThreadIndex;
//		Info->Queue = &Queue;
//		DWORD ThreadID;
//		HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Info, 0, &ThreadID);
//	}
//
//	return Queue;
//}

// Test code

static WORK_QUEUE_CALLBACK(DoWorkerWork) // test callback function
{
	char Buffer[256];
	wsprintf(Buffer, "Thread %u: %s\n", GetCurrentThreadId(), (char*)Data);
	OutputDebugStringA(Buffer); // This has internal sync
}

void MakeSomeThreads()
{
	work_queue* Queue = MakeWorkQueue();

	for(i32 i = 0; i < 4; i++)
	{
		AddWorkQueueEntry(Queue, DoWorkerWork, "String A0");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String A1");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String A2");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String A3");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String A4");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String A5");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String A6");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String A7");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String A8");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String A9");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String B0");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String B1");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String B2");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String B3");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String B4");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String B5");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String B6");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String B7");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String B8");
		AddWorkQueueEntry(Queue, DoWorkerWork, "String B9");

		CompleteAllWork(Queue);
		OutputDebugStringA("-------------------------------------------\n");
	}

}
#endif

/*

Example use:

struct tile_render_work
{
	render_group * RenderGroup;
	loaded_bitmap *OutputTarget;
	rectangle2i ClipRect;
}

static WORK_QUEUE_CALLBACK(DoTiledRenderWork)
{
	tile_render_work * Work = (tiel_render_work *) Data;

	RenderGroupToOutput(...,...,...,..);
}


TiledRenderGroupToOutput(work_queue *RenderQueue)
{
	tile_render_work WorkArray[TileCountX * TileCountY];

	i32 workindex = 0;
	for tileY
		for tileX
			tile_render_work *Work = WorkArray + WorkIndex++;

			{
				fill work struct
			}
			//AddWorkQueueEntry(RenderQueue, DoTiledRenderWork, &Work);
			// could do RenderQueueue->AddEntry(RenderQueue, DoTiledRenderWork, &Work); for platform abstraction
			CompleteAllWork(RenderQueue);

	// RenderQeuue->CompleteAllWork(RenderQueue); for platforma abstraction
	CompleteAllWork(RenderQueue);
}


*/


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