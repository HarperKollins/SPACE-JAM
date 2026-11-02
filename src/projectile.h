/**
 * projectile.h - Projectile (bullet) system for player and enemies
 */
#ifndef SPACEJAM_PROJECTILE_H
#define SPACEJAM_PROJECTILE_H

#include "raylib.h"
#include "config.h"

typedef enum {
    PROJ_NONE = 0,
    PROJ_PLAYER,
    PROJ_ENEMY,
    PROJ_BOSS
} ProjectileOwner;

typedef struct {
    Vector2        pos;
    Vector2        vel;
    float          life;
    float          radius;
    int            damage;
    ProjectileOwner owner;
    int            active;
    /* visual */
    Color          color;
    Color          trailColor;
    Vector2        prevPos;  /* for trail rendering */
} Projectile;

typedef struct {
    Projectile player[MAX_PLAYER_BULLETS];
    Projectile enemy[MAX_ENEMY_BULLETS];
} ProjectilePool;

void Projectiles_Init(ProjectilePool *pool);
void Projectiles_Update(ProjectilePool *pool, float dt, int screenW, int screenH);
void Projectiles_Draw(const ProjectilePool *pool);

/* Spawn helpers — return 1 on success, 0 if pool full */
int Projectile_SpawnPlayer(ProjectilePool *pool, Vector2 pos, Vector2 dir, int damage);
int Projectile_SpawnEnemy(ProjectilePool *pool, Vector2 pos, Vector2 dir, int damage, Color color);
int Projectile_SpawnEnemyEx(ProjectilePool *pool, Vector2 pos, Vector2 dir, float speed, float radius, int damage, Color color);

void Projectiles_KillPlayer(ProjectilePool *pool, int index);
void Projectiles_KillEnemy(ProjectilePool *pool, int index);

#endif /* SPACEJAM_PROJECTILE_H */

