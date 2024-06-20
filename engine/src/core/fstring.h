#pragma once

#include "defines.h"
#include "math/matrixMath.h"

// Returns the length of the given str.
FSNAPI u64 strLen(const char* str);

FSNAPI char* strDup(const char* str);

FSNAPI char* strSub(const char* str, const char* sub);

FSNAPI b8 strEqual(const char* str0, const char* str1);

FSNAPI b8 strEqualI(const char* str0, const char* str1);

FSNAPI i32 strFmtV(char* dest, const char* format, void* vaListp);

FSNAPI i32 strFmt(char* dest, const char* format, ...);
/**
 * @brief If a negative value is passed, proceed to the end of the str.
 */
FSNAPI void strCut(char* dest, const char* source, i32 start, i32 length);

FSNAPI char* strCpy(char* dest, const char* source);

FSNAPI char* strNCpy(char* txt, const char* src, u64 len);

/**
 * @brief Empties the provided string by setting the first character to 0.
 *
 * @param str The string to be emptied.
 * @return A pointer to str.
 */
FSNAPI char* strEmpty(char* str);

FSNAPI char* strTrim(char* str);

FSNAPI i32 strIdxOf(const char* str, char c);
/**
 * @brief Splits a string into a dinoArray
 *
 * @param str The string to be splitted.
 * @param delimiter char to split the char by.
 * @param strDinoArray The dino array that the string vals will be stored.
 * @param trimIt Trim the whitespace off the sides of each str vals.
 * @param includeZeroCharLines Include zero char lines.
 * @return Count of string vals.
 */
FSNAPI u32 strSplit(const char* str, char delimiter, char*** strDinoArray,
                    b8 trimIt, b8 includeZeroCharLines);

/**
 * @brief Cleans a dinoArray that is filled with strings. Frees all the strings,
 * so you can reuse the dinoArray.
 *
 * @param dinoArray The dinoArray that will be cleaned.
 */
FSNAPI void strCleanDinoArray(char** dinoArray);

FSNAPI b8 strToMat4(const char* str, mat4* outMat);

FSNAPI b8 strToVec4(const char* str, vector4* outVector);

FSNAPI b8 strToVec3(const char* str, vector3* outVector);

FSNAPI b8 strToVec2(const char* str, vector2* outVector);

FSNAPI b8 strToF32(const char* str, f32* f);

FSNAPI b8 strToF64(const char* str, f64* f);

FSNAPI b8 strToI8(const char* str, i8* i);

FSNAPI b8 strToI16(const char* str, i16* i);

FSNAPI b8 strToI32(const char* str, i32* i);

FSNAPI b8 strToI64(const char* str, i64* i);

FSNAPI b8 strToU8(const char* str, u8* u);

FSNAPI b8 strToU16(const char* str, u16* u);

FSNAPI b8 strToU32(const char* str, u32* u);

FSNAPI b8 strToU64(const char* str, u64* u);

FSNAPI b8 strToBool(const char* str, b8* b);
