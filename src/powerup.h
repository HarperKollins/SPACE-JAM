/**
 * powerup.h - Power-up spawn, collection and state
 */
#ifndef SPACEJAM_POWERUP_H
#define SPACEJAM_POWERUP_H

#include "raylib.h"
#include "config.h"

typedef enum {
    POWERUP_HEALTH = 0,
    POWERUP_SHIELD,
    POWERUP_RAPIDFIRE,
    POWERUP_DOUBLESHOT,
    POWERUP_SPREAD,      /* three-way spread shot */
    POWERUP_INVINCIBLE,
    POWERUP_TYPE_COUNT
} PowerupType;

typedef struct {
    Vector2     pos;
    PowerupType type;
    float       life;      /* remaining display time */
    float       bobTimer;  /* for floating animation */
    int         active;
} Powerup;

typedef struct {
    Powerup powerups[MAX_POWERUPS];
} PowerupPool;

void Powerups_Init(PowerupPool *pool);
void Powerups_Update(PowerupPool *pool, float dt);
void Powerups_Draw(const PowerupPool *pool);
void Powerups_Clear(PowerupPool *pool);

/* Try to spawn at pos with given type; 0 if pool full */
int  Powerup_Spawn(PowerupPool *pool, Vector2 pos, PowerupType type);
/* Spawn a random powerup (used after enemy/asteroid destruction) */
int  Powerup_SpawnRandom(PowerupPool *pool, Vector2 pos);

void Powerup_Kill(PowerupPool *pool, int index);

/* Returns colour associated with power-up type */
Color Powerup_GetColor(PowerupType type);
const char *Powerup_GetName(PowerupType type);

#endif /* SPACEJAM_POWERUP_H */

