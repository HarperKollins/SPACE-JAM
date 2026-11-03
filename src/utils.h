/**
 * utils.h - Shared utility types, macros, and inline helpers
 */
#ifndef SPACEJAM_UTILS_H
#define SPACEJAM_UTILS_H

#include <math.h>
#include <stdint.h>
#include "raylib.h"

/* -- Numeric helpers ------------------------------------ */
#define MIN(a, b)        ((a) < (b) ? (a) : (b))
#define MAX(a, b)        ((a) > (b) ? (a) : (b))
#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))
#define ABS(a)           ((a) < 0 ? -(a) : (a))

/* -- Angle & Degree / Radian conversion --------------- */
/* In standard raylib, DEG2RAD and RAD2DEG are constant multipliers:
   e.g. angle_rad = angle_deg * DEG2RAD.
   To avoid breaking raylib expressions while providing function-style conversions,
   DegToRad(d) and RadToDeg(r) are provided. */
static inline float DegToRad(float deg) {
    return deg * (PI / 180.0f);
}
static inline float RadToDeg(float rad) {
    return rad * (180.0f / PI);
}

/* Backward-compatible macro aliases: if used as DEG2RAD(d), function-like macro works;
   if undefined, fallback to multipliers. */
#ifdef DEG2RAD
#undef DEG2RAD
#endif
#define DEG2RAD(d) DegToRad(d)

#ifdef RAD2DEG
#undef RAD2DEG
#endif
#define RAD2DEG(r) RadToDeg(r)

/* -- Random helpers ------------------------------------- */
/* Returns float in [lo, hi] */
static inline float RandFloat(float lo, float hi) {
    return lo + (float)GetRandomValue(0, 10000) / 10000.0f * (hi - lo);
}

/* Returns 1 with probability p (0.0 - 1.0) */
static inline int RandChance(float p) {
    return (float)GetRandomValue(0, 10000) / 10000.0f < p;
}

/* -- Vector math ---------------------------------------- */
static inline Vector2 Vec2Add(Vector2 a, Vector2 b) {
    return (Vector2){ a.x + b.x, a.y + b.y };
}
static inline Vector2 Vec2Sub(Vector2 a, Vector2 b) {
    return (Vector2){ a.x - b.x, a.y - b.y };
}
static inline Vector2 Vec2Scale(Vector2 v, float s) {
    return (Vector2){ v.x * s, v.y * s };
}
static inline float Vec2Len(Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}
static inline Vector2 Vec2Norm(Vector2 v) {
    float len = Vec2Len(v);
    if (len < 0.0001f) return (Vector2){0, 0};
    return (Vector2){ v.x / len, v.y / len };
}
static inline float Vec2Dot(Vector2 a, Vector2 b) {
    return a.x * b.x + a.y * b.y;
}
static inline float Vec2Dist(Vector2 a, Vector2 b) {
    return Vec2Len(Vec2Sub(a, b));
}

/* Direction vector from angle in degrees (0 = up, 90 = right, clock-wise) */
static inline Vector2 AngleToDir(float deg) {
    float rad = DegToRad(deg);
    return (Vector2){ sinf(rad), -cosf(rad) };
}

/* -- Color helpers -------------------------------------- */
/* Note: ColorAlpha and ColorLerp are provided by raylib 6.x - no redefinition needed. */
/* Use raylib's ColorAlpha(color, alpha) and ColorLerp(c1, c2, factor) directly.      */

#endif /* SPACEJAM_UTILS_H */
