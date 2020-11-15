#include "work_queue.h"

#include "intrin.h" // WriteBarrier

#include "global_preprocessor_flags.h"
//#define NUM_THREADS 8

static bool DoNextWorkQueueEntry(work_queue* Queue)
{

	u32 OriginalNextEntryToRead = Queue->NextEntryToRead;

	u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % Queue->MaxEntryCount;
	bool IsValid = OriginalNextEntryToRead != Queue->NextEntryToWrite;
	bool bShouldWait = !IsValid;
	if (IsValid)
	{
		u32 Index = InterlockedCompareExchange((LONG volatile*)&Queue->NextEntryToRead,
			NewNextEntryToRead,
			OriginalNextEntryToRead);

		// If this holds, we are ok, else some other thread has beaten us to getting the work
		if (Index == OriginalNextEntryToRead)
		{
			work_queue_entry Entry = Queue->Entries[Index];
			Entry.Callback(Queue, Entry.Data);
			// interlocked increment does the correct fence //_ReadWriteBarrier();    _mm_mfence();
			InterlockedIncrement((LONG volatile*)&Queue->EntryCompletionCount);
		}
	}

	return bShouldWait;
}


// TODO make this return a bool if it fails and queue is full
void AddWorkQueueEntry(work_queue* Queue, work_queue_callback* Callback, void* Data)
{
	// TODO: for multiple producers add interlocked compare exchange to entrycount
	// TODO: Switch to interlockedcompare exchange eventually so that any thread can add?
	u32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % Queue->MaxEntryCount;
	assert(NewNextEntryToWrite != Queue->NextEntryToRead);
	work_queue_entry* Entry = Queue->Entries + Queue->NextEntryToWrite;
	Entry->Data = Data;
	Entry->Callback = Callback;
	Queue->CompletionGoal++;
	_WriteBarrier(); // Redundant if using sfence, VS will treat mm_sfence as a writebarrier
	// _mm_sfence();    // Unneccesary for x64
	Queue->NextEntryToWrite = NewNextEntryToWrite;
	LONG lReleaseCount = 1;
	LONG lpPreviousCount;
	ReleaseSemaphore(Queue->SemaphoreHandle, lReleaseCount, &lpPreviousCount);
}

static bool QueueWorkStillInProgress(work_queue* Queue)
{
	return Queue->CompletionGoal != Queue->EntryCompletionCount;// { DoWorkerWork(NUM_THREADS); }
}
void CompleteAllWork(work_queue* Queue)
{
	while (QueueWorkStillInProgress(Queue))
	{
		DoNextWorkQueueEntry(Queue);
	}
	Queue->EntryCompletionCount = 0;
	Queue->CompletionGoal = 0;
}

DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{
	work_queue* Queue = (work_queue*)lpParameter;
	for (;;)
	{
		if (DoNextWorkQueueEntry(Queue))
		{
			WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
		}
	}
}

work_queue* MakeWorkQueue()
{
	work_queue* Queue = new work_queue{};
	LPSECURITY_ATTRIBUTES SemaphoreAttributes = 0;
	LONG InitialCount = 0;
	LONG MaximumCount = NUM_THREADS;
	LPCSTR Name = 0;
	DWORD Flags = 0;
	DWORD DesiredAccess = SEMAPHORE_ALL_ACCESS;
	Queue->SemaphoreHandle = CreateSemaphoreExA(
		SemaphoreAttributes,
		InitialCount,
		MaximumCount,
		Name,
		Flags,
		DesiredAccess
	);

	for (i32 ThreadIndex = 0; ThreadIndex < NUM_THREADS; ThreadIndex++)
	{
		DWORD ThreadID;
		HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Queue, 0, &ThreadID);
		if(ThreadHandle != 0) CloseHandle(ThreadHandle);
	}

	return Queue;
}