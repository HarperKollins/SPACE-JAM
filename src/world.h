/**
 * world.h - Parallax starfield background
 */
#ifndef SPACEJAM_WORLD_H
#define SPACEJAM_WORLD_H

#include "raylib.h"
#include "config.h"

typedef struct {
    Vector2 pos;
    float   brightness;
    float   size;
} Star;

typedef struct {
    Star  layers[STAR_LAYERS][STARS_PER_LAYER];
    float speeds[STAR_LAYERS];
} Starfield;

void Starfield_Init(Starfield *sf, int screenW, int screenH);
void Starfield_Update(Starfield *sf, float dt, int screenW, int screenH);
void Starfield_Draw(const Starfield *sf);

#endif /* SPACEJAM_WORLD_H */

