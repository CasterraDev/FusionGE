#include "filesystem.h"

#include "core/logger.h"
#include "core/fmemory.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

b8 fsExists(const char* path){
    struct stat x;
    return stat(path,&x);
}

b8 fsOpen(const char* path, fileModes mode, b8 binary, fileHandle* outHandle){
    //Set the outHandle to false/0 incase it fails
    outHandle->isValid = false;
    outHandle->handle = 0;
    //Turn the filemode enum into the correct string
    const char* str;
    if ((mode & FILE_MODE_WRITE) != 0 && (mode & FILE_MODE_READ) != 0){
        str = binary ? "w+b" : "w+";
    }else if ((mode & FILE_MODE_WRITE) == 0 && (mode & FILE_MODE_READ) != 0){
        str = binary ? "rb" : "r";
    }else if ((mode & FILE_MODE_WRITE) != 0 && (mode & FILE_MODE_READ) == 0){
        str = binary ? "wb" : "w";
    }
    //Open the file
    FILE* file = fopen(path, str);
    if (!file){
        FERROR("FS: Failed to open file");
        return false;
    }
    //Set the outHandle
    outHandle->handle = file;
    outHandle->isValid = true;
    return true;
}

void fsClose(fileHandle* fh){
    if (fh->handle){
        fclose((FILE*)fh->handle);
        fh->handle = 0;
        fh->isValid = false;
    }
}

b8 fsReadLine(fileHandle* handle, u64 maxLen, char** lineBuffer, u64* outLineLen) {
    if (handle->handle && lineBuffer && outLineLen && maxLen > 0) {
        // Since we are reading a single line, it should be safe to assume this is enough characters.
        char* buf = *lineBuffer;
        if (fgets(buf, maxLen, (FILE*)handle->handle) != 0) {
            *outLineLen = strlen(*lineBuffer);
            return true;
        }
    }
    return false;
}

b8 fsWriteLine(fileHandle* fh, const char* text){
    if (fh->handle){
        i32 res = fputs(text,(FILE*)fh->handle);
        if (res != EOF){
            res = fputc('\n',(FILE*)fh->handle);
        }
        fflush((FILE*)fh->handle);
        return true;
    }
    return false;
}

b8 fsRead(fileHandle* fh, u64 dataSize, void* outData, u64* outBytesRead){
    if (fh->handle && outData && outBytesRead){
        *outBytesRead = fread(outData,1,dataSize,(FILE*)fh->handle);
        if (dataSize != *outBytesRead){
            //Something went wrong
            return false;
        }
        return true;
    }
    return false;
}

b8 fsSize(fileHandle* handle, u64* outSize){
    if (handle->handle) {
        fseek((FILE*)handle->handle, 0, SEEK_END);
        *outSize = ftell((FILE*)handle->handle);
        rewind((FILE*)handle->handle);
        return true;
    }
    return false;
}

b8 fsReadFileBytes(fileHandle* fh, u8* outData, u64* outBytesRead){
    if (fh->handle && outData && outBytesRead) {
        // File size
        u64 size = 0;
        if(!fsSize(fh, &size)) {
            return false;
        }

        *outBytesRead = fread(outData, 1, size, (FILE*)fh->handle);
        return *outBytesRead == size;
    }
    return false;
}

b8 fsReadFileChars(fileHandle* handle, char* outChars, u64* outBytesRead){
    if (handle->handle && outChars && outBytesRead) {
        // File size
        u64 size = 0;
        if(!fsSize(handle, &size)) {
            return false;
        }

        *outBytesRead = fread(outChars, 1, size, (FILE*)handle->handle);
        return *outBytesRead == size;
    }
    return false;
}

b8 fsWrite(fileHandle* fh, u64 dataSize, const void* data, u64* outBytesWritten){
    if (fh->handle){
        *outBytesWritten = fwrite(data,1,dataSize,(FILE*)fh->handle);
        if (*outBytesWritten != dataSize){
            //Something went wrong
            return false;
        }
        fflush((FILE*)fh->handle);
        return true;
    }
    return false;
}

