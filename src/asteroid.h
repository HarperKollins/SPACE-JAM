/**
 * asteroid.h - Asteroid field system
 */
#ifndef SPACEJAM_ASTEROID_H
#define SPACEJAM_ASTEROID_H

#include "raylib.h"
#include "config.h"

typedef enum {
    ASTEROID_LARGE = 0,
    ASTEROID_MEDIUM,
    ASTEROID_SMALL,
    ASTEROID_SIZE_COUNT
} AsteroidSize;

typedef struct {
    Vector2      pos;
    Vector2      vel;
    float        rotation;
    float        rotSpeed;
    float        radius;
    AsteroidSize size;
    int          health;
    int          active;
    /* Polygon shape - 8 vertices offset from center */
    float        shapeOffsets[8]; /* radial multipliers 0.7 - 1.3 */
    Color        color;
    Color        colorEdge;
} Asteroid;

typedef struct {
    Asteroid asteroids[MAX_ASTEROIDS];
    float    spawnTimer;
    float    spawnInterval; /* decreases with wave progression */
} AsteroidField;

void Asteroids_Init(AsteroidField *af);
void Asteroids_Update(AsteroidField *af, float dt, int screenW, int screenH, int wave);
void Asteroids_Draw(const AsteroidField *af);
void Asteroids_Clear(AsteroidField *af);
void Asteroids_SetSpawnRate(AsteroidField *af, int wave);

int  Asteroid_Spawn(AsteroidField *af, AsteroidSize size, int screenW, int screenH);

/* Call when a bullet hits an asteroid; may spawn 2 smaller ones */
/* Returns score earned */
int  Asteroid_Hit(AsteroidField *af, int index, int damage, int screenW, int screenH);

#endif /* SPACEJAM_ASTEROID_H */

