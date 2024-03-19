#pragma once

#include "defines.h"

b8 platformStartup(
    u64* memoryRequirement,
    void* state,
    const char* appName,
    i32 x,
    i32 y,
    i32 width,
    i32 height
    );

void platformShutdown();

b8 platformPumpMessages();

void* platformAllocate(u64 size, b8 aligned);
void platformFree(void* block, b8 aligned);
void* platformZeroMemory(void* block, u64 size);
void* platformCopyMemory(void* dest,const void* src, u64 size);
void* platformSetMemory(void* dest, i32 val, u64 size);

void platformConsoleWrite(const char* msg, u8 color);
void platformConsoleWriteError(const char* msg, u8 color);

f64 platformGetAbsoluteTime();

void platformSleep(u64 ms);