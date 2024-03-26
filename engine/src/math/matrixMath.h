#pragma once

#include "defines.h"

typedef union vector2_u {
    // An array of x, y
    f32 elements[2];
    struct {
        union {
            // The first element.
            f32 x, r, s, u;
        };
        union {
            // The second element.
            f32 y, g, t, v;
        };
    };
} vector2;

typedef struct vector3_u {
    union {
        // An array of x, y, z
        f32 elements[3];
        struct {
            union {
                // The first element.
                f32 x, r, s, u;
            };
            union {
                // The second element.
                f32 y, g, t, v;
            };
            union {
                // The third element.
                f32 z, b, p, w;
            };
        };
    };
} vector3;

typedef union vector4_u {
    // An array of x, y, z, w
    f32 elements[4];
    union {
        struct {
            union {
                // The first element.
                f32 x, r, s;
            };
            union {
                // The second element.
                f32 y, g, t;
            };
            union {
                // The third element.
                f32 z, b, p;
            };
            union {
                // The fourth element.
                f32 w, a, q;
            };
        };
    };
} vector4;

typedef vector4 quat;

typedef union mat4_u {
    f32 data[16];
} mat4;

typedef struct vertex3D {
    /** @brief The position of the vertex */
    vector3 position;
    /** @brief The texture coordinate of the vertex. */
    vector2 texcoord;
    /** @brief The normal of the vertex. */
    vector3 normal;
    /** @brief The color of the vertex. */
    vector4 color;
    /** @brief The tangent of the vertex. */
    vector3 tangent;
} vertex3D;

typedef struct vertex2D {
    /** @brief The position of the vertex */
    vector2 position;
    /** @brief The texture coordinate of the vertex. */
    vector2 texcoord;
    /** @brief The normal of the vertex. */
    vector2 normal;
    // /** @brief The color of the vertex. */
    // vector4 color;
    // /** @brief The tangent of the vertex. */
    // vector2 tangent;
} vertex2D;