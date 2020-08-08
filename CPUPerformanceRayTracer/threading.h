#pragma once

//#include "utils.h"
//#include "windows.h"
//
//#define WORK_QUEUE_CALLBACK(name) void name(struct work_queue *Queue, void *Data)
//typedef WORK_QUEUE_CALLBACK(work_queue_callback);
//
//struct work_queue_entry
//{
//	work_queue_callback* Callback;
//	void* Data;
//};
//
//#define WORK_QUEUE_MAX_ENTRIES 256
//struct work_queue
//{
//	u32 MaxEntryCount = WORK_QUEUE_MAX_ENTRIES;
//
//	u32 volatile CompletionGoal;
//	u32 volatile EntryCompletionCount;
//	u32 volatile NextEntryToWrite;
//	u32 volatile NextEntryToRead;
//
//	HANDLE SemaphoreHandle;
//
//	work_queue_entry Entries[WORK_QUEUE_MAX_ENTRIES]; // temp down from 256
//};
//
//
//// External interface
//work_queue MakeWorkQueue();
//void AddWorkQueueEntry(work_queue* Queue, work_queue_callback *Callback, void* Pointer);
//void CompleteAllWork(work_queue* Queue);

// Test code
void MakeSomeThreads();