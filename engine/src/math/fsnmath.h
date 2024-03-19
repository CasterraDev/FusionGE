#pragma once

#include "defines.h"
#include "matrixMath.h"

#include "core/fmemory.h"

#define FSN_PI 3.14159265358979323846f
#define FSN_TWICE_PI (2.0f * FSN_PI)
#define FSN_HALF_PI (0.5f * FSN_PI)
#define FSN_QUARTER_PI (0.25f * FSN_PI)
#define FSN_ONE_OVER_PI (1.0f / FSN_PI)
#define FSN_ONE_OVER_TWO_PI (1.0f / FSN_TWICE_PI)
#define FSN_SQRT_TWO 1.41421356237309504880f
#define FSN_SQRT_THREE 1.73205080756887729352f
#define FSN_SQRT_ONE_OVER_TWO 0.70710678118654752440f
#define FSN_SQRT_ONE_OVER_THREE 0.57735026918962576450f
#define FSN_DEG2RAD_MULTIPLIER (FSN_PI / 180.0f)
#define FSN_RAD2DEG_MULTIPLIER (180.0f / FSN_PI)

// The multiplier to convert seconds to milliseconds.
#define FSN_SEC_TO_MS_MULTIPLIER 1000.0f

// The multiplier to convert milliseconds to seconds.
#define FSN_MS_TO_SEC_MULTIPLIER 0.001f

// Taken from math.h
#define FSN_INFINITY ((float)(1e300 * 1e300))

// Smallest positive number where 1.0 + FLOAT_EPSILON != 0
#define FSN_FLOAT_EPSILON 1.192092896e-07f

// ------------------------------------------
// General math functions
// ------------------------------------------
FSNAPI f32 fsnSin(f32 x);
FSNAPI f32 fsnCos(f32 x);
FSNAPI f32 fsnTan(f32 x);
FSNAPI f32 fsnAcos(f32 x);
FSNAPI f32 fsnSqrt(f32 x);
FSNAPI f32 fsnAbs(f32 x);

/**
 * Indicates if the value is a power of 2. 0 is considered Not_ a power of 2.
 * @param value The value to be interpreted.
 * @returns True if a power of 2, otherwise false.
 */
FSNINLINE b8 isPow2(u64 value) {
    return (value != 0) && ((value & (value - 1)) == 0);
}

FSNAPI void fsnSRandWithTime();
FSNAPI void fsnSRand(u32 i);
FSNAPI i32 fsnRandom();
FSNAPI i32 fsnRandomRange(i32 min, i32 max);

FSNAPI f32 fsnRandomf();
FSNAPI f32 fsnRandomRangef(f32 min, f32 max);

// ------------------------------------------
// Vector 2
// ------------------------------------------

/**
 * @brief Creates and returns a new 2-element vector using the supplied values.
 * 
 * @param x The x value.
 * @param y The y value.
 * @return A new 2-element vector.
 */
FSNINLINE vector2 vec2Create(f32 x, f32 y) {
    vector2 outVec;
    outVec.x = x;
    outVec.y = y;
    return outVec;
}

/**
 * @brief Creates and returns a 2-component vector with all components set to 0.0f.
 */
FSNINLINE vector2 vec2Zero() {
    return (vector2){0.0f, 0.0f};
}

/**
 * @brief Creates and returns a 2-component vector with all components set to 1.0f.
 */
FSNINLINE vector2 vec2One() {
    return (vector2){1.0f, 1.0f};
}

/**
 * @brief Creates and returns a 2-component vector pointing up (0, 1).
 */
FSNINLINE vector2 vec2Up() {
    return (vector2){0.0f, 1.0f};
}

/**
 * @brief Creates and returns a 2-component vector pointing down (0, -1).
 */
FSNINLINE vector2 vec2Down() {
    return (vector2){0.0f, -1.0f};
}

/**
 * @brief Creates and returns a 2-component vector pointing left (-1, 0).
 */
FSNINLINE vector2 vec2Left() {
    return (vector2){-1.0f, 0.0f};
}

/**
 * @brief Creates and returns a 2-component vector pointing right (1, 0).
 */
FSNINLINE vector2 vec2Right() {
    return (vector2){1.0f, 0.0f};
}

/**
 * @brief Adds vector1 to vector0 and returns a copy of the result.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The resulting vector. 
 */
FSNINLINE vector2 vec2Add(vector2 vector0, vector2 vector1) {
    return (vector2){
        vector0.x + vector1.x,
        vector0.y + vector1.y};
}

/**
 * @brief Subtracts vector1 from vector0 and returns a copy of the result.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The resulting vector. 
 */
FSNINLINE vector2 vec2Sub(vector2 vector0, vector2 vector1) {
    return (vector2){
        vector0.x - vector1.x,
        vector0.y - vector1.y};
}

/**
 * @brief Multiplies vector0 by vector1 and returns a copy of the result.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The resulting vector. 
 */
FSNINLINE vector2 vec2Mul(vector2 vector0, vector2 vector1) {
    return (vector2){
        vector0.x * vector1.x,
        vector0.y * vector1.y};
}

/**
 * @brief Divides vector0 by vector1 and returns a copy of the result.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The resulting vector. 
 */
FSNINLINE vector2 vec2Div(vector2 vector0, vector2 vector1) {
    return (vector2){
        vector0.x / vector1.x,
        vector0.y / vector1.y};
}

/**
 * @brief Returns the squared length of the provided vector.
 * 
 * @param vector The vector to retrieve the squared length of.
 * @return The squared length.
 */
FSNINLINE f32 vec2LengthSquared(vector2 vector) {
    return vector.x * vector.x + vector.y * vector.y;
}

/**
 * @brief Returns the length of the provided vector.
 * 
 * @param vector The vector to retrieve the length of.
 * @return The length.
 */
FSNINLINE f32 vec2Length(vector2 vector) {
    return fsnSqrt(vec2LengthSquared(vector));
}

/**
 * @brief Normalizes the provided vector in place to a unit vector.
 * 
 * @param vector A pointer to the vector to be normalized.
 */
FSNINLINE void vec2Normalize(vector2* vector) {
    const f32 length = vec2Length(*vector);
    vector->x /= length;
    vector->y /= length;
}

/**
 * @brief Returns a normalized copy of the supplied vector.
 * 
 * @param vector The vector to be normalized.
 * @return A normalized copy of the supplied vector 
 */
FSNINLINE vector2 vec2Normalized(vector2 vector) {
    vec2Normalize(&vector);
    return vector;
}

/**
 * @brief Compares all elements of vector0 and vector1 and ensures the difference
 * is less than tolerance.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @param tolerance The difference tolerance. Typically FSN_FLOAT_EPSILON or similar.
 * @return True if within tolerance; otherwise false. 
 */
FSNINLINE b8 vec2Compare(vector2 vector0, vector2 vector1, f32 tolerance) {
    if (fsnAbs(vector0.x - vector1.x) > tolerance) {
        return false;
    }

    if (fsnAbs(vector0.y - vector1.y) > tolerance) {
        return false;
    }

    return true;
}

/**
 * @brief Returns the distance between vector0 and vector1.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The distance between vector0 and vector1.
 */
FSNINLINE f32 vec2Distance(vector2 vector0, vector2 vector1) {
    vector2 d = (vector2){
        vector0.x - vector1.x,
        vector0.y - vector1.y};
    return vec2Length(d);
}

// ------------------------------------------
// Vector 3
// ------------------------------------------

/**
 * @brief Creates and returns a new 3-element vector using the supplied values.
 * 
 * @param x The x value.
 * @param y The y value.
 * @param z The z value.
 * @return A new 3-element vector.
 */
FSNINLINE vector3 vec3Create(f32 x, f32 y, f32 z) {
    return (vector3){x, y, z};
}

/**
 * @brief Returns a new vector3 containing the x, y and z components of the 
 * supplied vector4, essentially dropping the w component.
 * 
 * @param vector The 4-component vector to extract from.
 * @return A new vector3 
 */
FSNINLINE vector3 vec3FromVec4(vector4 vector) {
    return (vector3){vector.x, vector.y, vector.z};
}

/**
 * @brief Returns a new vector4 using vector as the x, y and z components and w for w.
 * 
 * @param vector The 3-component vector.
 * @param w The w component.
 * @return A new vector4 
 */
FSNINLINE vector4 vec3ToVec4(vector3 vector, f32 w) {
    return (vector4){vector.x, vector.y, vector.z, w};
}

/**
 * @brief Creates and returns a 3-component vector with all components set to 0.0f.
 */
FSNINLINE vector3 vec3Zero() {
    return (vector3){0.0f, 0.0f, 0.0f};
}

/**
 * @brief Creates and returns a 3-component vector with all components set to 1.0f.
 */
FSNINLINE vector3 vec3One() {
    return (vector3){1.0f, 1.0f, 1.0f};
}

/**
 * @brief Creates and returns a 3-component vector pointing up (0, 1, 0).
 */
FSNINLINE vector3 vec3Up() {
    return (vector3){0.0f, 1.0f, 0.0f};
}

/**
 * @brief Creates and returns a 3-component vector pointing down (0, -1, 0).
 */
FSNINLINE vector3 vec3Down() {
    return (vector3){0.0f, -1.0f, 0.0f};
}

/**
 * @brief Creates and returns a 3-component vector pointing left (-1, 0, 0).
 */
FSNINLINE vector3 vec3Left() {
    return (vector3){-1.0f, 0.0f, 0.0f};
}

/**
 * @brief Creates and returns a 3-component vector pointing right (1, 0, 0).
 */
FSNINLINE vector3 vec3Right() {
    return (vector3){1.0f, 0.0f, 0.0f};
}

/**
 * @brief Creates and returns a 3-component vector pointing forward (0, 0, -1).
 */
FSNINLINE vector3 vec3Forward() {
    return (vector3){0.0f, 0.0f, -1.0f};
}

/**
 * @brief Creates and returns a 3-component vector pointing backward (0, 0, 1).
 */
FSNINLINE vector3 vec3Back() {
    return (vector3){0.0f, 0.0f, 1.0f};
}

/**
 * @brief Adds vector1 to vector0 and returns a copy of the result.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The resulting vector. 
 */
FSNINLINE vector3 vec3Add(vector3 vector0, vector3 vector1) {
    return (vector3){
        vector0.x + vector1.x,
        vector0.y + vector1.y,
        vector0.z + vector1.z};
}

/**
 * @brief Subtracts vector1 from vector0 and returns a copy of the result.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The resulting vector. 
 */
FSNINLINE vector3 vec3Sub(vector3 vector0, vector3 vector1) {
    return (vector3){
        vector0.x - vector1.x,
        vector0.y - vector1.y,
        vector0.z - vector1.z};
}

/**
 * @brief Multiplies vector0 by vector1 and returns a copy of the result.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The resulting vector. 
 */
FSNINLINE vector3 vec3Mul(vector3 vector0, vector3 vector1) {
    return (vector3){
        vector0.x * vector1.x,
        vector0.y * vector1.y,
        vector0.z * vector1.z};
}

/**
 * @brief Multiplies all elements of vector0 by scalar and returns a copy of the result.
 * 
 * @param vector0 The vector to be multiplied.
 * @param scalar The scalar value.
 * @return A copy of the resulting vector.
 */
FSNINLINE vector3 vec3MulScalar(vector3 vector0, f32 scalar) {
    return (vector3){
        vector0.x * scalar,
        vector0.y * scalar,
        vector0.z * scalar};
}

/**
 * @brief Divides vector0 by vector1 and returns a copy of the result.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The resulting vector. 
 */
FSNINLINE vector3 vec3Div(vector3 vector0, vector3 vector1) {
    return (vector3){
        vector0.x / vector1.x,
        vector0.y / vector1.y,
        vector0.z / vector1.z};
}

/**
 * @brief Returns the squared length of the provided vector.
 * 
 * @param vector The vector to retrieve the squared length of.
 * @return The squared length.
 */
FSNINLINE f32 vec3LengthSquared(vector3 vector) {
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
}

/**
 * @brief Returns the length of the provided vector.
 * 
 * @param vector The vector to retrieve the length of.
 * @return The length.
 */
FSNINLINE f32 vec3Length(vector3 vector) {
    return fsnSqrt(vec3LengthSquared(vector));
}

/**
 * @brief Normalizes the provided vector in place to a unit vector.
 * 
 * @param vector A pointer to the vector to be normalized.
 */
FSNINLINE void vec3Normalize(vector3* vector) {
    const f32 length = vec3Length(*vector);
    vector->x /= length;
    vector->y /= length;
    vector->z /= length;
}

/**
 * @brief Returns a normalized copy of the supplied vector.
 * 
 * @param vector The vector to be normalized.
 * @return A normalized copy of the supplied vector 
 */
FSNINLINE vector3 vec3Normalized(vector3 vector) {
    vec3Normalize(&vector);
    return vector;
}

/**
 * @brief Returns the dot product between the provided vectors. Typically used
 * to calculate the difference in direction.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The dot product. 
 */
FSNINLINE f32 vec3Dot(vector3 vector0, vector3 vector1) {
    f32 p = 0;
    p += vector0.x * vector1.x;
    p += vector0.y * vector1.y;
    p += vector0.z * vector1.z;
    return p;
}

/**
 * @brief Calculates and returns the cross product of the supplied vectors.
 * The cross product is a new vector which is orthoganal to both provided vectors.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The cross product. 
 */
FSNINLINE vector3 vec3Cross(vector3 vector0, vector3 vector1) {
    return (vector3){
        vector0.y * vector1.z - vector0.z * vector1.y,
        vector0.z * vector1.x - vector0.x * vector1.z,
        vector0.x * vector1.y - vector0.y * vector1.x};
}

/**
 * @brief Compares all elements of vector0 and vector1 and ensures the difference
 * is less than tolerance.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @param tolerance The difference tolerance. Typically FSN_FLOAT_EPSILON or similar.
 * @return True if within tolerance; otherwise false. 
 */
FSNINLINE const b8 vec3Compare(vector3 vector0, vector3 vector1, f32 tolerance) {
    if (fsnAbs(vector0.x - vector1.x) > tolerance) {
        return false;
    }

    if (fsnAbs(vector0.y - vector1.y) > tolerance) {
        return false;
    }

    if (fsnAbs(vector0.z - vector1.z) > tolerance) {
        return false;
    }

    return true;
}

/**
 * @brief Returns the distance between vector0 and vector1.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The distance between vector0 and vector1.
 */
FSNINLINE f32 vec3Distance(vector3 vector0, vector3 vector1) {
    vector3 d = (vector3){
        vector0.x - vector1.x,
        vector0.y - vector1.y,
        vector0.z - vector1.z};
    return vec3Length(d);
}


// ------------------------------------------
// Vector 4
// ------------------------------------------

/**
 * @brief Creates and returns a new 4-element vector using the supplied values.
 * 
 * @param x The x value.
 * @param y The y value.
 * @param z The z value.
 * @param w The w value.
 * @return A new 4-element vector.
 */
FSNINLINE vector4 vec4Create(f32 x, f32 y, f32 z, f32 w) {
    vector4 outVec;
#if defined(KUSE_SIMD)
    outVec.data = MmSetrPs(x, y, z, w);
#else
    outVec.x = x;
    outVec.y = y;
    outVec.z = z;
    outVec.w = w;
#endif
    return outVec;
}

/**
 * @brief Returns a new vector3 containing the x, y and z components of the 
 * supplied vector4, essentially dropping the w component.
 * 
 * @param vector The 4-component vector to extract from.
 * @return A new vector3 
 */
FSNINLINE vector3 vec4ToVec3(vector4 vector) {
    return (vector3){vector.x, vector.y, vector.z};
}

/**
 * @brief Returns a new vector4 using vector as the x, y and z components and w for w.
 * 
 * @param vector The 3-component vector.
 * @param w The w component.
 * @return A new vector4 
 */
FSNINLINE vector4 vec4FromVec3(vector3 vector, f32 w) {
#if defined(KUSE_SIMD)
    vector4 outVec;
    outVec.data = MmSetrPs(x, y, z, w);
    return outVec;
#else
    return (vector4){vector.x, vector.y, vector.z, w};
#endif
}

/**
 * @brief Creates and returns a 3-component vector with all components set to 0.0f.
 */
FSNINLINE vector4 vec4Zero() {
    return (vector4){0.0f, 0.0f, 0.0f, 0.0f};
}

/**
 * @brief Creates and returns a 3-component vector with all components set to 1.0f.
 */
FSNINLINE vector4 vec4One() {
    return (vector4){1.0f, 1.0f, 1.0f, 1.0f};
}

/**
 * @brief Adds vector1 to vector0 and returns a copy of the result.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The resulting vector. 
 */
FSNINLINE vector4 vec4Add(vector4 vector0, vector4 vector1) {
    vector4 result;
     for (u64 i = 0; i < 4; ++i) {
        result.elements[i] = vector0.elements[i] + vector1.elements[i];
    }
    return result;
}

/**
 * @brief Subtracts vector1 from vector0 and returns a copy of the result.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The resulting vector. 
 */
FSNINLINE vector4 vec4Sub(vector4 vector0, vector4 vector1) {
    vector4 result;
    for (u64 i = 0; i < 4; ++i) {
        result.elements[i] = vector0.elements[i] - vector1.elements[i];
    }
    return result;
}

/**
 * @brief Multiplies vector0 by vector1 and returns a copy of the result.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The resulting vector. 
 */
FSNINLINE vector4 vec4Mul(vector4 vector0, vector4 vector1) {
    vector4 result;
    for (u64 i = 0; i < 4; ++i) {
        result.elements[i] = vector0.elements[i] * vector1.elements[i];
    }
    return result;
}

/**
 * @brief Divides vector0 by vector1 and returns a copy of the result.
 * 
 * @param vector0 The first vector.
 * @param vector1 The second vector.
 * @return The resulting vector. 
 */
FSNINLINE vector4 vec4Div(vector4 vector0, vector4 vector1) {
    vector4 result;
    for (u64 i = 0; i < 4; ++i) {
        result.elements[i] = vector0.elements[i] / vector1.elements[i];
    }
    return result;
}

/**
 * @brief Returns the squared length of the provided vector.
 * 
 * @param vector The vector to retrieve the squared length of.
 * @return The squared length.
 */
FSNINLINE f32 vec4LengthSquared(vector4 vector) {
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w;
}

/**
 * @brief Returns the length of the provided vector.
 * 
 * @param vector The vector to retrieve the length of.
 * @return The length.
 */
FSNINLINE f32 vec4Length(vector4 vector) {
    return fsnSqrt(vec4LengthSquared(vector));
}

/**
 * @brief Normalizes the provided vector in place to a unit vector.
 * 
 * @param vector A pointer to the vector to be normalized.
 */
FSNINLINE void vec4Normalize(vector4* vector) {
    const f32 length = vec4Length(*vector);
    vector->x /= length;
    vector->y /= length;
    vector->z /= length;
    vector->w /= length;
}

/**
 * @brief Returns a normalized copy of the supplied vector.
 * 
 * @param vector The vector to be normalized.
 * @return A normalized copy of the supplied vector 
 */
FSNINLINE vector4 vec4Normalized(vector4 vector) {
    vec4Normalize(&vector);
    return vector;
}

FSNINLINE f32 vec4DotF32(
    f32 a0, f32 a1, f32 a2, f32 a3,
    f32 b0, f32 b1, f32 b2, f32 b3) {
    f32 p;
    p =
        a0 * b0 +
        a1 * b1 +
        a2 * b2 +
        a3 * b3;
    return p;
}

/**
 * @brief Creates and returns an identity matrix:
 * 
 * {
 *   {1, 0, 0, 0},
 *   {0, 1, 0, 0},
 *   {0, 0, 1, 0},
 *   {0, 0, 0, 1}
 * }
 * 
 * @return A new identity matrix 
 */
FSNINLINE mat4 mat4Identity() {
    mat4 outMatrix;
    fzeroMemory(outMatrix.data, sizeof(f32) * 16);
    outMatrix.data[0] = 1.0f;
    outMatrix.data[5] = 1.0f;
    outMatrix.data[10] = 1.0f;
    outMatrix.data[15] = 1.0f;
    return outMatrix;
}

/**
 * @brief Returns the result of multiplying matrix_0 and matrix_1.
 * 
 * @param matrix_0 The first matrix to be multiplied.
 * @param matrix_1 The second matrix to be multiplied.
 * @return The result of the matrix multiplication.
 */
FSNINLINE mat4 mat4Mul(mat4 matrix_0, mat4 matrix_1) {
    mat4 outMatrix = mat4Identity();

    const f32* m1Ptr = matrix_0.data;
    const f32* m2Ptr = matrix_1.data;
    f32* dstPtr = outMatrix.data;

    for (i32 i = 0; i < 4; ++i) {
        for (i32 j = 0; j < 4; ++j) {
            *dstPtr =
                m1Ptr[0] * m2Ptr[0 + j] +
                m1Ptr[1] * m2Ptr[4 + j] +
                m1Ptr[2] * m2Ptr[8 + j] +
                m1Ptr[3] * m2Ptr[12 + j];
            dstPtr++;
        }
        m1Ptr += 4;
    }
    return outMatrix;
}

/**
 * @brief Creates and returns an orthographic projection matrix. Typically used to
 * render flat or 2D scenes.
 * 
 * @param left The left side of the view frustum.
 * @param right The right side of the view frustum.
 * @param bottom The bottom side of the view frustum.
 * @param top The top side of the view frustum.
 * @param nearClip The near clipping plane distance.
 * @param farClip The far clipping plane distance.
 * @return A new orthographic projection matrix. 
 */
FSNINLINE mat4 mat4Orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 nearClip, f32 farClip) {
    mat4 outMatrix = mat4Identity();

    f32 lr = 1.0f / (left - right);
    f32 bt = 1.0f / (bottom - top);
    f32 nf = 1.0f / (nearClip - farClip);

    outMatrix.data[0] = -2.0f * lr;
    outMatrix.data[5] = -2.0f * bt;
    outMatrix.data[10] = 2.0f * nf;

    outMatrix.data[12] = (left + right) * lr;
    outMatrix.data[13] = (top + bottom) * bt;
    outMatrix.data[14] = (farClip + nearClip) * nf;
    return outMatrix;
}

/**
 * @brief Creates and returns a perspective matrix. Typically used to render 3d scenes.
 * 
 * @param fovRadians The field of view in radians.
 * @param aspectRatio The aspect ratio.
 * @param nearClip The near clipping plane distance.
 * @param farClip The far clipping plane distance.
 * @return A new perspective matrix. 
 */
FSNINLINE mat4 mat4Perspective(f32 fovRadians, f32 aspectRatio, f32 nearClip, f32 farClip) {
    f32 halfTanFov = fsnTan(fovRadians * 0.5f);
    mat4 outMatrix;
    fzeroMemory(outMatrix.data, sizeof(f32) * 16);
    outMatrix.data[0] = 1.0f / (aspectRatio * halfTanFov);
    outMatrix.data[5] = 1.0f / halfTanFov;
    outMatrix.data[10] = -((farClip + nearClip) / (farClip - nearClip));
    outMatrix.data[11] = -1.0f;
    outMatrix.data[14] = -((2.0f * farClip * nearClip) / (farClip - nearClip));
    return outMatrix;
}

/**
 * @brief Creates and returns a look-at matrix, or a matrix looking 
 * at target from the perspective of position.
 * 
 * @param position The position of the matrix.
 * @param target The position to "look at".
 * @param up The up vector.
 * @return A matrix looking at target from the perspective of position. 
 */
FSNINLINE mat4 mat4LookAt(vector3 position, vector3 target, vector3 up) {
    mat4 outMatrix;
    vector3 zAxis;
    zAxis.x = target.x - position.x;
    zAxis.y = target.y - position.y;
    zAxis.z = target.z - position.z;

    zAxis = vec3Normalized(zAxis);
    vector3 xAxis = vec3Normalized(vec3Cross(zAxis, up));
    vector3 yAxis = vec3Cross(xAxis, zAxis);

    outMatrix.data[0] = xAxis.x;
    outMatrix.data[1] = yAxis.x;
    outMatrix.data[2] = -zAxis.x;
    outMatrix.data[3] = 0;
    outMatrix.data[4] = xAxis.y;
    outMatrix.data[5] = yAxis.y;
    outMatrix.data[6] = -zAxis.y;
    outMatrix.data[7] = 0;
    outMatrix.data[8] = xAxis.z;
    outMatrix.data[9] = yAxis.z;
    outMatrix.data[10] = -zAxis.z;
    outMatrix.data[11] = 0;
    outMatrix.data[12] = -vec3Dot(xAxis, position);
    outMatrix.data[13] = -vec3Dot(yAxis, position);
    outMatrix.data[14] = vec3Dot(zAxis, position);
    outMatrix.data[15] = 1.0f;

    return outMatrix;
}

/**
 * @brief Returns a transposed copy of the provided matrix (rows->colums)
 * 
 * @param matrix The matrix to be transposed.
 * @return A transposed copy of of the provided matrix.
 */
FSNINLINE mat4 mat4Transposed(mat4 matrix) {
    mat4 outMatrix = mat4Identity();
    outMatrix.data[0] = matrix.data[0];
    outMatrix.data[1] = matrix.data[4];
    outMatrix.data[2] = matrix.data[8];
    outMatrix.data[3] = matrix.data[12];
    outMatrix.data[4] = matrix.data[1];
    outMatrix.data[5] = matrix.data[5];
    outMatrix.data[6] = matrix.data[9];
    outMatrix.data[7] = matrix.data[13];
    outMatrix.data[8] = matrix.data[2];
    outMatrix.data[9] = matrix.data[6];
    outMatrix.data[10] = matrix.data[10];
    outMatrix.data[11] = matrix.data[14];
    outMatrix.data[12] = matrix.data[3];
    outMatrix.data[13] = matrix.data[7];
    outMatrix.data[14] = matrix.data[11];
    outMatrix.data[15] = matrix.data[15];
    return outMatrix;
}

/**
 * @brief Creates and returns an inverse of the provided matrix.
 * 
 * @param matrix The matrix to be inverted.
 * @return A inverted copy of the provided matrix. 
 */
FSNINLINE mat4 mat4Inverse(mat4 matrix) {
    const f32* m = matrix.data;

    f32 t0 = m[10] * m[15];
    f32 t1 = m[14] * m[11];
    f32 t2 = m[6] * m[15];
    f32 t3 = m[14] * m[7];
    f32 t4 = m[6] * m[11];
    f32 t5 = m[10] * m[7];
    f32 t6 = m[2] * m[15];
    f32 t7 = m[14] * m[3];
    f32 t8 = m[2] * m[11];
    f32 t9 = m[10] * m[3];
    f32 t10 = m[2] * m[7];
    f32 t11 = m[6] * m[3];
    f32 t12 = m[8] * m[13];
    f32 t13 = m[12] * m[9];
    f32 t14 = m[4] * m[13];
    f32 t15 = m[12] * m[5];
    f32 t16 = m[4] * m[9];
    f32 t17 = m[8] * m[5];
    f32 t18 = m[0] * m[13];
    f32 t19 = m[12] * m[1];
    f32 t20 = m[0] * m[9];
    f32 t21 = m[8] * m[1];
    f32 t22 = m[0] * m[5];
    f32 t23 = m[4] * m[1];

    mat4 outMatrix;
    f32* o = outMatrix.data;

    o[0] = (t0 * m[5] + t3 * m[9] + t4 * m[13]) - (t1 * m[5] + t2 * m[9] + t5 * m[13]);
    o[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) - (t0 * m[1] + t7 * m[9] + t8 * m[13]);
    o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) - (t3 * m[1] + t6 * m[5] + t11 * m[13]);
    o[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9]) - (t4 * m[1] + t9 * m[5] + t10 * m[9]);

    f32 d = 1.0f / (m[0] * o[0] + m[4] * o[1] + m[8] * o[2] + m[12] * o[3]);

    o[0] = d * o[0];
    o[1] = d * o[1];
    o[2] = d * o[2];
    o[3] = d * o[3];
    o[4] = d * ((t1 * m[4] + t2 * m[8] + t5 * m[12]) - (t0 * m[4] + t3 * m[8] + t4 * m[12]));
    o[5] = d * ((t0 * m[0] + t7 * m[8] + t8 * m[12]) - (t1 * m[0] + t6 * m[8] + t9 * m[12]));
    o[6] = d * ((t3 * m[0] + t6 * m[4] + t11 * m[12]) - (t2 * m[0] + t7 * m[4] + t10 * m[12]));
    o[7] = d * ((t4 * m[0] + t9 * m[4] + t10 * m[8]) - (t5 * m[0] + t8 * m[4] + t11 * m[8]));
    o[8] = d * ((t12 * m[7] + t15 * m[11] + t16 * m[15]) - (t13 * m[7] + t14 * m[11] + t17 * m[15]));
    o[9] = d * ((t13 * m[3] + t18 * m[11] + t21 * m[15]) - (t12 * m[3] + t19 * m[11] + t20 * m[15]));
    o[10] = d * ((t14 * m[3] + t19 * m[7] + t22 * m[15]) - (t15 * m[3] + t18 * m[7] + t23 * m[15]));
    o[11] = d * ((t17 * m[3] + t20 * m[7] + t23 * m[11]) - (t16 * m[3] + t21 * m[7] + t22 * m[11]));
    o[12] = d * ((t14 * m[10] + t17 * m[14] + t13 * m[6]) - (t16 * m[14] + t12 * m[6] + t15 * m[10]));
    o[13] = d * ((t20 * m[14] + t12 * m[2] + t19 * m[10]) - (t18 * m[10] + t21 * m[14] + t13 * m[2]));
    o[14] = d * ((t18 * m[6] + t23 * m[14] + t15 * m[2]) - (t22 * m[14] + t14 * m[2] + t19 * m[6]));
    o[15] = d * ((t22 * m[10] + t16 * m[2] + t21 * m[6]) - (t20 * m[6] + t23 * m[10] + t17 * m[2]));

    return outMatrix;
}

FSNINLINE mat4 mat4Translation(vector3 position) {
    mat4 outMatrix = mat4Identity();
    outMatrix.data[12] = position.x;
    outMatrix.data[13] = position.y;
    outMatrix.data[14] = position.z;
    return outMatrix;
}

/**
 * @brief Returns a scale matrix using the provided scale.
 * 
 * @param scale The 3-component scale.
 * @return A scale matrix.
 */
FSNINLINE mat4 mat4Scale(vector3 scale) {
    mat4 outMatrix = mat4Identity();
    outMatrix.data[0] = scale.x;
    outMatrix.data[5] = scale.y;
    outMatrix.data[10] = scale.z;
    return outMatrix;
}

FSNINLINE mat4 mat4EulerX(f32 angleRadians) {
    mat4 outMatrix = mat4Identity();
    f32 c = fsnCos(angleRadians);
    f32 s = fsnSin(angleRadians);

    outMatrix.data[5] = c;
    outMatrix.data[6] = s;
    outMatrix.data[9] = -s;
    outMatrix.data[10] = c;
    return outMatrix;
}
FSNINLINE mat4 mat4EulerY(f32 angleRadians) {
    mat4 outMatrix = mat4Identity();
    f32 c = fsnCos(angleRadians);
    f32 s = fsnSin(angleRadians);

    outMatrix.data[0] = c;
    outMatrix.data[2] = -s;
    outMatrix.data[8] = s;
    outMatrix.data[10] = c;
    return outMatrix;
}
FSNINLINE mat4 mat4EulerZ(f32 angleRadians) {
    mat4 outMatrix = mat4Identity();

    f32 c = fsnCos(angleRadians);
    f32 s = fsnSin(angleRadians);

    outMatrix.data[0] = c;
    outMatrix.data[1] = s;
    outMatrix.data[4] = -s;
    outMatrix.data[5] = c;
    return outMatrix;
}
FSNINLINE mat4 mat4EulerXyz(f32 xRadians, f32 yRadians, f32 zRadians) {
    mat4 rx = mat4EulerX(xRadians);
    mat4 ry = mat4EulerY(yRadians);
    mat4 rz = mat4EulerZ(zRadians);
    mat4 outMatrix = mat4Mul(rx, ry);
    outMatrix = mat4Mul(outMatrix, rz);
    return outMatrix;
}

/**
 * @brief Returns a forward vector relative to the provided matrix.
 * 
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
FSNINLINE vector3 mat4Forward(mat4 matrix) {
    vector3 forward;
    forward.x = -matrix.data[2];
    forward.y = -matrix.data[6];
    forward.z = -matrix.data[10];
    vec3Normalize(&forward);
    return forward;
}

/**
 * @brief Returns a backward vector relative to the provided matrix.
 * 
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
FSNINLINE vector3 mat4Backward(mat4 matrix) {
    vector3 backward;
    backward.x = matrix.data[2];
    backward.y = matrix.data[6];
    backward.z = matrix.data[10];
    vec3Normalize(&backward);
    return backward;
}

/**
 * @brief Returns a upward vector relative to the provided matrix.
 * 
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
FSNINLINE vector3 mat4Up(mat4 matrix) {
    vector3 up;
    up.x = matrix.data[1];
    up.y = matrix.data[5];
    up.z = matrix.data[9];
    vec3Normalize(&up);
    return up;
}

/**
 * @brief Returns a downward vector relative to the provided matrix.
 * 
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
FSNINLINE vector3 mat4Down(mat4 matrix) {
    vector3 down;
    down.x = -matrix.data[1];
    down.y = -matrix.data[5];
    down.z = -matrix.data[9];
    vec3Normalize(&down);
    return down;
}

/**
 * @brief Returns a left vector relative to the provided matrix.
 * 
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
FSNINLINE vector3 mat4Left(mat4 matrix) {
    vector3 right;
    right.x = -matrix.data[0];
    right.y = -matrix.data[4];
    right.z = -matrix.data[8];
    vec3Normalize(&right);
    return right;
}

/**
 * @brief Returns a right vector relative to the provided matrix.
 * 
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
FSNINLINE vector3 mat4Right(mat4 matrix) {
    vector3 left;
    left.x = matrix.data[0];
    left.y = matrix.data[4];
    left.z = matrix.data[8];
    vec3Normalize(&left);
    return left;
}

// ------------------------------------------
// Quaternion
// ------------------------------------------

FSNINLINE quat quatIdentity() {
    return (quat){0, 0, 0, 1.0f};
}

FSNINLINE f32 quatNormal(quat q) {
    return fsnSqrt(
        q.x * q.x +
        q.y * q.y +
        q.z * q.z +
        q.w * q.w);
}

FSNINLINE quat quatNormalize(quat q) {
    f32 normal = quatNormal(q);
    return (quat){
        q.x / normal,
        q.y / normal,
        q.z / normal,
        q.w / normal};
}

FSNINLINE quat quatConjugate(quat q) {
    return (quat){
        -q.x,
        -q.y,
        -q.z,
        q.w};
}

FSNINLINE quat quatInverse(quat q) {
    return quatNormalize(quatConjugate(q));
}

FSNINLINE quat quatMul(quat q_0, quat q_1) {
    quat outQuaternion;

    outQuaternion.x = q_0.x * q_1.w +
                       q_0.y * q_1.z -
                       q_0.z * q_1.y +
                       q_0.w * q_1.x;

    outQuaternion.y = -q_0.x * q_1.z +
                       q_0.y * q_1.w +
                       q_0.z * q_1.x +
                       q_0.w * q_1.y;

    outQuaternion.z = q_0.x * q_1.y -
                       q_0.y * q_1.x +
                       q_0.z * q_1.w +
                       q_0.w * q_1.z;

    outQuaternion.w = -q_0.x * q_1.x -
                       q_0.y * q_1.y -
                       q_0.z * q_1.z +
                       q_0.w * q_1.w;

    return outQuaternion;
}

FSNINLINE f32 quatDot(quat q_0, quat q_1) {
    return q_0.x * q_1.x +
           q_0.y * q_1.y +
           q_0.z * q_1.z +
           q_0.w * q_1.w;
}

FSNINLINE mat4 quatToMat4(quat q) {
    mat4 outMatrix = mat4Identity();

    // https://stackoverflow.com/questions/1556260/convert-quaternion-rotation-to-rotation-matrix

    quat n = quatNormalize(q);

    outMatrix.data[0] = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;
    outMatrix.data[1] = 2.0f * n.x * n.y - 2.0f * n.z * n.w;
    outMatrix.data[2] = 2.0f * n.x * n.z + 2.0f * n.y * n.w;

    outMatrix.data[4] = 2.0f * n.x * n.y + 2.0f * n.z * n.w;
    outMatrix.data[5] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;
    outMatrix.data[6] = 2.0f * n.y * n.z - 2.0f * n.x * n.w;

    outMatrix.data[8] = 2.0f * n.x * n.z - 2.0f * n.y * n.w;
    outMatrix.data[9] = 2.0f * n.y * n.z + 2.0f * n.x * n.w;
    outMatrix.data[10] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;

    return outMatrix;
}

// Calculates a rotation matrix based on the quaternion and the passed in center point.
FSNINLINE mat4 quatToRotationMatrix(quat q, vector3 center) {
    mat4 outMatrix;

    f32* o = outMatrix.data;
    o[0] = (q.x * q.x) - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[1] = 2.0f * ((q.x * q.y) + (q.z * q.w));
    o[2] = 2.0f * ((q.x * q.z) - (q.y * q.w));
    o[3] = center.x - center.x * o[0] - center.y * o[1] - center.z * o[2];

    o[4] = 2.0f * ((q.x * q.y) - (q.z * q.w));
    o[5] = -(q.x * q.x) + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[6] = 2.0f * ((q.y * q.z) + (q.x * q.w));
    o[7] = center.y - center.x * o[4] - center.y * o[5] - center.z * o[6];

    o[8] = 2.0f * ((q.x * q.z) + (q.y * q.w));
    o[9] = 2.0f * ((q.y * q.z) - (q.x * q.w));
    o[10] = -(q.x * q.x) - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
    o[11] = center.z - center.x * o[8] - center.y * o[9] - center.z * o[10];

    o[12] = 0.0f;
    o[13] = 0.0f;
    o[14] = 0.0f;
    o[15] = 1.0f;
    return outMatrix;
}

FSNINLINE quat quatFromAxisAngle(vector3 axis, f32 angle, b8 normalize) {
    const f32 halfAngle = 0.5f * angle;
    f32 s = fsnSin(halfAngle);
    f32 c = fsnCos(halfAngle);

    quat q = (quat){s * axis.x, s * axis.y, s * axis.z, c};
    if (normalize) {
        return quatNormalize(q);
    }
    return q;
}

FSNINLINE quat quatSlerp(quat q_0, quat q_1, f32 percentage) {
    quat outQuaternion;
    // Source: https://en.wikipedia.org/wiki/Slerp
    // Only unit quaternions are valid rotations.
    // Normalize to avoid undefined behavior.
    quat v0 = quatNormalize(q_0);
    quat v1 = quatNormalize(q_1);

    // Compute the cosine of the angle between the two vectors.
    f32 dot = quatDot(v0, v1);

    // If the dot product is negative, slerp won't take
    // the shorter path. Note that v1 and -v1 are equivalent when
    // the negation is applied to all four components. Fix by
    // reversing one quaternion.
    if (dot < 0.0f) {
        v1.x = -v1.x;
        v1.y = -v1.y;
        v1.z = -v1.z;
        v1.w = -v1.w;
        dot = -dot;
    }

    const f32 DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD) {
        // If the inputs are too close for comfort, linearly interpolate
        // and normalize the result.
        outQuaternion = (quat){
            v0.x + ((v1.x - v0.x) * percentage),
            v0.y + ((v1.y - v0.y) * percentage),
            v0.z + ((v1.z - v0.z) * percentage),
            v0.w + ((v1.w - v0.w) * percentage)};

        return quatNormalize(outQuaternion);
    }

    // Since dot is in range [0, DOT_THRESHOLD], acos is safe
    f32 theta_0 = fsnAcos(dot);          // theta_0 = angle between input vectors
    f32 theta = theta_0 * percentage;  // theta = angle between v0 and result
    f32 sinTheta = fsnSin(theta);       // compute this value only once
    f32 sinTheta_0 = fsnSin(theta_0);   // compute this value only once

    f32 s0 = fsnCos(theta) - dot * sinTheta / sinTheta_0;  // == sin(theta_0 - theta) / sin(theta_0)
    f32 s1 = sinTheta / sinTheta_0;

    return (quat){
        (v0.x * s0) + (v1.x * s1),
        (v0.y * s0) + (v1.y * s1),
        (v0.z * s0) + (v1.z * s1),
        (v0.w * s0) + (v1.w * s1)};
}

/**
 * @brief Converts provided degrees to radians.
 * 
 * @param degrees The degrees to be converted.
 * @return The amount in radians.
 */
FSNINLINE f32 degToRad(f32 degrees) {
    return degrees * FSN_DEG2RAD_MULTIPLIER;
}

/**
 * @brief Converts provided radians to degrees.
 * 
 * @param radians The radians to be converted.
 * @return The amount in degrees.
 */
FSNINLINE f32 radToDeg(f32 radians) {
    return radians * FSN_RAD2DEG_MULTIPLIER;
}