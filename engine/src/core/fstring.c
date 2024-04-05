#include "core/fstring.h"
#include "core/fmemory.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>  // isspace

#ifndef _MSC_VER
#include <strings.h>
#endif

/**
 * This is all stolen from somewhere.
 * I'm not doing string math. Not yet.
*/

u64 strLen(const char* str) {
    return strlen(str);
}

char* strDup(const char* str) {
    u64 length = strLen(str);
    char* copy = fallocate(length + 1, MEMORY_TAG_STRING);
    fcopyMemory(copy, str, length + 1);
    return copy;
}

char* strEmpty(char* str) {
    if (str) {
        str[0] = 0;
    }

    return str;
}

char* strSub(const char* str, const char* sub){
    return strstr(str,sub);
}

//Case-sensitive
b8 strEqual(const char* str0, const char* str1) {
    return strcmp(str0, str1) == 0;
}

// Case-insensitive string comparison. True if the same, otherwise false.
b8 strEqualI(const char* str0, const char* str1) {
#if defined(__GNUC__)
    return strcasecmp(str0, str1) == 0;
#elif defined(_MSC_VER)
    return _strcmpi(str0, str1) == 0;
#endif
}

char* strCpy(char* dest, const char* source) {
    return strcpy(dest, source);
}

char* strNCpy(char* txt, const char* src, u64 len){
    return strncpy(txt, src, len);    
}

i32 strFmtV(char* dest, const char* format, void* vaListp) {
    if (dest) {
        // Big, but can fit on the stack.
        char buffer[32000];
        i32 written = vsnprintf(buffer, 32000, format, vaListp);
        buffer[written] = 0;
        fcopyMemory(dest, buffer, written + 1);

        return written;
    }
    return -1;
}

i32 strFmt(char* dest, const char* format, ...) {
    if (dest) {
        __builtin_va_list argPtr;
        va_start(argPtr, format);
        i32 written = strFmtV(dest, format, argPtr);
        va_end(argPtr);
        return written;
    }
    return -1;
}

void strCut(char* dest, const char* source, i32 start, i32 length) {
    if (length == 0) {
        return;
    }
    u64 src_length = strLen(source);
    if (start >= src_length) {
        dest[0] = 0;
        return;
    }
    if (length > 0) {
        for (u64 i = start, j = 0; j < length && source[i]; ++i, ++j) {
            dest[j] = source[i];
        }
        dest[start + length] = 0;
    } else {
        // If a negative value is passed, proceed to the end of the string.
        u64 j = 0;
        for (u64 i = start; source[i]; ++i, ++j) {
            dest[j] = source[i];
        }
        dest[start + j] = 0;
    }
}

char* strTrim(char* str) {
    while (isspace((unsigned char)*str)) {
        str++;
    }
    if (*str) {
        char* p = str;
        while (*p) {
            p++;
        }
        while (isspace((unsigned char)*(--p)))
            ;

        p[1] = '\0';
    }

    return str;
}

i32 strIdxOf(const char* str, char c) {
    if (!str) {
        return -1;
    }
    u32 length = strLen(str);
    if (length > 0) {
        for (u32 i = 0; i < length; ++i) {
            if (str[i] == c) {
                return i;
            }
        }
    }

    return -1;
}

b8 strToMat4(const char* str, mat4* outMat) {
    if (!str || !outMat) {
        return false;
    }

    fzeroMemory(outMat, sizeof(mat4));
    i32 result = sscanf(str, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                        &outMat->data[0],
                        &outMat->data[1],
                        &outMat->data[2],
                        &outMat->data[3],
                        &outMat->data[4],
                        &outMat->data[5],
                        &outMat->data[6],
                        &outMat->data[7],
                        &outMat->data[8],
                        &outMat->data[9],
                        &outMat->data[10],
                        &outMat->data[11],
                        &outMat->data[12],
                        &outMat->data[13],
                        &outMat->data[14],
                        &outMat->data[15]);
    return result != -1;
}

b8 strToVec4(const char* str, vector4* outVector) {
    if (!str || !outVector) {
        return false;
    }

    fzeroMemory(outVector, sizeof(vector4));
    i32 result = sscanf(str, "%f %f %f %f", &outVector->x, &outVector->y, &outVector->z, &outVector->w);
    return result != -1;
}

b8 strToVec3(const char* str, vector3* outVector) {
    if (!str || !outVector) {
        return false;
    }

    fzeroMemory(outVector, sizeof(vector3));
    i32 result = sscanf(str, "%f %f %f", &outVector->x, &outVector->y, &outVector->z);
    return result != -1;
}

b8 strToVec2(const char* str, vector2* outVector) {
    if (!str || !outVector) {
        return false;
    }

    fzeroMemory(outVector, sizeof(vector2));
    i32 result = sscanf(str, "%f %f", &outVector->x, &outVector->y);
    return result != -1;
}

b8 strToF32(const char* str, f32* f) {
    if (!str || !f) {
        return false;
    }

    *f = 0;
    i32 result = sscanf(str, "%f", f);
    return result != -1;
}

b8 strToF64(const char* str, f64* f) {
    if (!str || !f) {
        return false;
    }

    *f = 0;
    i32 result = sscanf(str, "%lf", f);
    return result != -1;
}

b8 strToI8(const char* str, i8* i) {
    if (!str || !i) {
        return false;
    }

    *i = 0;
    i32 result = sscanf(str, "%hhi", i);
    return result != -1;
}

b8 strToI16(const char* str, i16* i) {
    if (!str || !i) {
        return false;
    }

    *i = 0;
    i32 result = sscanf(str, "%hi", i);
    return result != -1;
}

b8 strToI32(const char* str, i32* i) {
    if (!str || !i) {
        return false;
    }

    *i = 0;
    i32 result = sscanf(str, "%i", i);
    return result != -1;
}

b8 strToI64(const char* str, i64* i) {
    if (!str || !i) {
        return false;
    }

    *i = 0;
    i32 result = sscanf(str, "%lli", i);
    return result != -1;
}

b8 strToU8(const char* str, u8* u) {
    if (!str || !u) {
        return false;
    }

    *u = 0;
    i32 result = sscanf(str, "%hhu", u);
    return result != -1;
}

b8 strToU16(const char* str, u16* u) {
    if (!str || !u) {
        return false;
    }

    *u = 0;
    i32 result = sscanf(str, "%hu", u);
    return result != -1;
}

b8 strToU32(const char* str, u32* u) {
    if (!str || !u) {
        return false;
    }

    *u = 0;
    i32 result = sscanf(str, "%u", u);
    return result != -1;
}

b8 strToU64(const char* str, u64* u) {
    if (!str || !u) {
        return false;
    }

    *u = 0;
    i32 result = sscanf(str, "%llu", u);
    return result != -1;
}

b8 strToBool(const char* str, b8* b) {
    if (!str || !b) {
        return false;
    }

    *b = strEqual(str, "1") || strEqualI(str, "true");
    return *b;
}

