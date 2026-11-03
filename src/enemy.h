/**
 * enemy.h - Enemy spacecraft types and state
 */
#ifndef SPACEJAM_ENEMY_H
#define SPACEJAM_ENEMY_H

#include "raylib.h"
#include "config.h"

typedef enum {
    ENEMY_NONE = 0,
    ENEMY_SCOUT,
    ENEMY_INTERCEPTOR,
    ENEMY_TANK,
    ENEMY_BOMBER,
    ENEMY_BOSS,
    CIVILIAN_SHUTTLE,
    ENEMY_TYPE_COUNT
} EnemyType;

typedef enum {
    ESTATE_IDLE = 0,
    ESTATE_APPROACH,
    ESTATE_STRAFE,
    ESTATE_RETREAT,
    ESTATE_ATTACK,
    ESTATE_DYING
} EnemyState;

typedef struct {
    Vector2    pos;
    Vector2    vel;
    float      rotation;
    float      targetAngle;

    EnemyType  type;
    EnemyState state;
    int        health;
    int        healthMax;
    float      radius;
    int        score;
    int        active;

    float      stateTimer;
    float      fireTimer;
    float      strafeDir;   /* +1 or -1 */
    float      deathTimer;

    /* Boss-specific multi-phase mechanics */
    int        bossPhase;
    float      bossAttackTimer;
    float      bossMovAngle;     /* oscillation angle for boss pattern */
    float      beamChargeTimer;  /* warning telegraph timer */
    float      beamActiveTimer;  /* sweeping thermal lance timer */
    float      beamAngle;        /* current beam angle in degrees */
    int        hasShieldTurrets; /* orbital shield sub-nodes active */
} Enemy;

typedef struct {
    Enemy enemies[MAX_ENEMIES];
    int   count;  /* active count */
} EnemyPool;

void Enemies_Init(EnemyPool *pool);
void Enemies_InitTextures(void);
void Enemies_ShutdownTextures(void);
void Enemies_Update(EnemyPool *pool, Vector2 playerPos, float dt,
                    int screenW, int screenH);
void Enemies_Draw(const EnemyPool *pool);
void Enemies_Clear(EnemyPool *pool);

int  Enemy_Spawn(EnemyPool *pool, EnemyType type, int screenW, int screenH);
void Enemy_SpawnBoss(EnemyPool *pool, int screenW, int screenH);
void Enemy_SpawnSectorBoss(EnemyPool *pool, int sector, int screenW, int screenH);
int  Enemy_SpawnShuttle(EnemyPool *pool, int screenW, int screenH);
void Enemy_TakeDamage(Enemy *e, int dmg);

/* Returns slot index of boss, or -1 */
int  Enemies_FindBoss(const EnemyPool *pool);
const char *Enemy_GetBossName(int sector);

/* Drawing helpers (also used by boss rendering) */
void DrawEnemy(const Enemy *e);

#endif /* SPACEJAM_ENEMY_H */

