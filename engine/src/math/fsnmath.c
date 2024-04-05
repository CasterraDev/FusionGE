#include "fsnmath.h"
#include "platform/platform.h"

#include <math.h>
#include <stdlib.h>

static b8 randSeeded = false;

/**
 * Note that these are here in order to prevent having to import the
 * entire <math.h> everywhere.
 */
f32 fsnSin(f32 x) {
    return sinf(x);
}

f32 fsnCos(f32 x) {
    return cosf(x);
}

f32 fsnTan(f32 x) {
    return tanf(x);
}

f32 fsnAcos(f32 x) {
    return acosf(x);
}

f32 fsnSqrt(f32 x) {
    return sqrtf(x);
}

f32 fsnAbs(f32 x) {
    return fabsf(x);
}

void fsnSRandWithTime(){
    srand((u32)platformGetAbsoluteTime());
}

void fsnSRand(u32 i){
    srand(i);
    randSeeded = true;
}

i32 fsnRandom() {
    return rand();
}

i32 fsnRandomRange(i32 min, i32 max) {
    return (rand() % (max - min + 1)) + min;
}

f32 fsnRandomf() {
    return (float)fsnRandom() / (f32)RAND_MAX;
}

f32 fsnRandomRangef(f32 min, f32 max) {
    return min + ((float)fsnRandom() / ((f32)RAND_MAX / (max - min)));
}