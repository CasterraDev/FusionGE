#include "logger.h"
#include "asserts.h"
#include "platform/platform.h"
#include "platform/filesystem.h"
#include "helpers/dinoArray.h"

// TODO: temporary
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

typedef struct loggerState{
    char* logQueue;
    u64 fileLogQueueCnt;
    fileHandle fileHandle;
} loggerState;

static loggerState* systemPtr;

void sendTextToFile(const char* m){
    u64 l = strlen(m);
    u64 written = 0;
    if (!fsWrite(&systemPtr->fileHandle, l, m, &written)){
        FERROR("Failed to write to log file");
    }
}

b8 loggerInit(u64* memoryRequirement, void* state) {
    *memoryRequirement = sizeof(loggerState);
    if (state == 0){
        return true;
    }
    systemPtr = state;

    if (!fsOpen("appLogger.log", FILE_MODE_WRITE, false, &systemPtr->fileHandle)){
        FERROR("Couldn't open appLogger.log to write logs.");
        return false;
    }
    return true;
}

void loggerShutdown() {
    systemPtr = 0;
    // TODO: cleanup logging/write queued entries.
}

void logToFile(logLevel level, b8 logToConsole, const char* message, ...){
    const char* levelStr[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: "};
    b8 isError = level < 2;

    // Technically imposes a 32k character limit on a single log entry, but...
    // DON'T DO THAT!
    char outMessage[32000];
    memset(outMessage, 0, sizeof(outMessage));

    // Format original message.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    vsnprintf(outMessage, 32000, message, arg_ptr);
    va_end(arg_ptr);

    char finalMessage[32000];
    sprintf(finalMessage, "%s%s\n", levelStr[level], outMessage);

    if (logToConsole){
        if(isError){
            platformConsoleWriteError(finalMessage,level);
        }else{
            platformConsoleWrite(finalMessage,level);
        }
    }

    sendTextToFile(finalMessage);
}

void logOutput(logLevel level, const char* message, ...) {
    const char* levelStr[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: "};
    b8 isError = level < 2;

    // Technically imposes a 32k character limit on a single log entry, but...
    // DON'T DO THAT!
    char outMessage[32000];
    memset(outMessage, 0, sizeof(outMessage));

    // Format original message.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    vsnprintf(outMessage, 32000, message, arg_ptr);
    va_end(arg_ptr);

    char finalMessage[32000];
    sprintf(finalMessage, "%s%s\n", levelStr[level], outMessage);

    // TODO: platform-specific output.
    if(isError){
        platformConsoleWriteError(finalMessage,level);
    }else{
        platformConsoleWrite(finalMessage,level);
    }
}

void reportAssertFailure(const char* expression, const char* message, const char* file, i32 line) {
    logOutput(LOG_LEVEL_FATAL, "Assertion Failure: %s, message: '%s', in file: %s, line: %d\n", expression, message, file, line);
}
