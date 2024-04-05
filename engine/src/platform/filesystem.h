#pragma once

#include "defines.h"

// Holds a handle to a file.
typedef struct fileHandle {
    // Opaque handle to internal file handle.
    void* handle;
    b8 isValid;
} fileHandle;

typedef enum fileModes {
    FILE_MODE_READ = 0x1,
    FILE_MODE_WRITE = 0x2
} fileModes;

/**
 * Checks if a file with the given path exists.
 * @param path The path of the file to be checked.
 * @returns True if exists; otherwise false.
 */
FSNAPI b8 fsExists(const char* path);

/** 
 * Attempt to open file located at path.
 * @param path The path of the file to be opened.
 * @param mode Mode flags for the file when opened (read/write). See fileModes enum in filesystem.h.
 * @param binary Indicates if the file should be opened in binary mode.
 * @param outHandle A pointer to a fileHandle structure which holds handle information.
 * @returns True if opened successfully; otherwise false.
 */
FSNAPI b8 fsOpen(const char* path, fileModes mode, b8 binary, fileHandle* outHandle);

/** 
 * Closes the provided handle to a file.
 * @param handle A pointer to a fileHandle structure which holds the handle to be closed.
 */
FSNAPI void fsClose(fileHandle* handle);

/** 
 * Reads up to a newline or EOF.
 * Allocates *lineBuf, which must be freed by the caller.
 * @param handle A pointer to a fileHandle structure.
 * @param maxLen max length to read
 * @param lineBuffer A pointer to a character array which will be allocated and populated by this method.
 * @param outLineLen length of line read
 * @returns True if successful; otherwise false.
 */
FSNAPI b8 fsReadLine(fileHandle* handle, u64 maxLen, char** lineBuffer, u64* outLineLen);

/** 
 * Writes text to the provided file, appending a '\\n' afterward.
 * @param handle A pointer to a fileHandle structure.
 * @param text The text to be written.
 * @returns True if successful; otherwise false.
 */
FSNAPI b8 fsWriteLine(fileHandle* handle, const char* text);

/** 
 * Reads up to dataSize bytes of data into outBytesRead. 
 * Allocates *outData, which must be freed by the caller.
 * @param handle A pointer to a fileHandle structure.
 * @param dataSize The number of bytes to read.
 * @param outData A pointer to a block of memory to be populated by this method.
 * @param outBytesRead A pointer to a number which will be populated with the number of bytes actually read from the file.
 * @returns True if successful; otherwise false.
 */
FSNAPI b8 fsRead(fileHandle* handle, u64 dataSize, void* outData, u64* outBytesRead);

/**
 * Attempts to read the size of the file to which handle is attached.
 * @param handle The file handle.
 * @param outSize A pointer to hold the file size.
 * @return True if successful; otherwise false.
 */
FSNAPI b8 fsSize(fileHandle* handle, u64* outSize);

/** 
 * Reads all bytes of the file into outBytesRead. 
 * Allocates *outBytes, which must be freed by the caller.
 * @param handle A pointer to a fileHandle structure.
 * @param outBytes A pointer to a byte array which will be allocated and populated by this method.
 * @param outBytesRead A pointer to a number which will be populated with the number of bytes actually read from the file.
 * @returns True if successful; otherwise false.
 */
FSNAPI b8 fsReadFileBytes(fileHandle* handle, u8* outBytes, u64* outBytesRead);

/** 
 * Reads all bytes of the file into outBytesRead. 
 * Allocates *outChars, which must be freed by the caller.
 * @param handle A pointer to a fileHandle structure.
 * @param outChars A pointer to a char array which will be allocated and populated by this method.
 * @param outBytesRead A pointer to a number which will be populated with the number of bytes actually read from the file.
 * @returns True if successful; otherwise false.
 */
FSNAPI b8 fsReadFileChars(fileHandle* handle, char* outChars, u64* outBytesRead);

/** 
 * Writes provided data to the file.
 * @param handle A pointer to a fileHandle structure.
 * @param dataSize The size of the data in bytes.
 * @param data The data to be written.
 * @param outBytesWritten A pointer to a number which will be populated with the number of bytes actually written to the file.
 * @returns True if successful; otherwise false.
 */
FSNAPI b8 fsWrite(fileHandle* handle, u64 dataSize, const void* data, u64* outBytesWritten);