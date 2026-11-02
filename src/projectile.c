/**
 * projectile.c - Player and enemy projectile management
 */
#include "projectile.h"
#include "utils.h"
#include <math.h>

void Projectiles_Init(ProjectilePool *pool) {
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) pool->player[i].active = 0;
    for (int i = 0; i < MAX_ENEMY_BULLETS;  i++) pool->enemy[i].active  = 0;
}

void Projectiles_Update(ProjectilePool *pool, float dt, int screenW, int screenH) {
    /* Player bullets */
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        Projectile *b = &pool->player[i];
        if (!b->active) continue;
        b->prevPos = b->pos;
        b->pos.x += b->vel.x * dt;
        b->pos.y += b->vel.y * dt;
        b->life -= dt;
        if (b->life <= 0.0f ||
            b->pos.x < -20 || b->pos.x > screenW + 20 ||
            b->pos.y < -20 || b->pos.y > screenH + 20) {
            b->active = 0;
        }
    }
    /* Enemy bullets */
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        Projectile *b = &pool->enemy[i];
        if (!b->active) continue;
        b->prevPos = b->pos;
        b->pos.x += b->vel.x * dt;
        b->pos.y += b->vel.y * dt;
        b->life -= dt;
        if (b->life <= 0.0f ||
            b->pos.x < -20 || b->pos.x > screenW + 20 ||
            b->pos.y < -20 || b->pos.y > screenH + 20) {
            b->active = 0;
        }
    }
}

void Projectiles_Draw(const ProjectilePool *pool) {
    /* Player bullets — neon cyan laser */
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        const Projectile *b = &pool->player[i];
        if (!b->active) continue;

        /* Trail line from prevPos to pos */
        DrawLineEx(b->prevPos, b->pos, 3.0f, (Color){0, 200, 255, 120});
        /* Core */
        DrawLineEx(b->prevPos, b->pos, 1.5f, (Color){180, 255, 255, 255});
        /* Tip glow */
        DrawCircleV(b->pos, b->radius, (Color){0, 220, 255, 255});
        DrawCircleV(b->pos, b->radius * 2.0f, (Color){0, 180, 255, 60});
    }

    /* Enemy bullets — orange/red */
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        const Projectile *b = &pool->enemy[i];
        if (!b->active) continue;

        DrawLineEx(b->prevPos, b->pos, 3.0f, b->trailColor);
        DrawLineEx(b->prevPos, b->pos, 1.5f, ColorAlpha(b->color, 0.85f));
        DrawCircleV(b->pos, b->radius, b->color);
        DrawCircleV(b->pos, b->radius * 2.0f, ColorAlpha(b->color, 0.25f));
    }
}

int Projectile_SpawnPlayer(ProjectilePool *pool, Vector2 pos, Vector2 dir, int damage) {
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        Projectile *b = &pool->player[i];
        if (b->active) continue;
        b->pos     = pos;
        b->prevPos = pos;
        b->vel     = Vec2Scale(dir, BULLET_SPEED);
        b->life    = BULLET_LIFETIME;
        b->radius  = BULLET_RADIUS;
        b->damage  = damage;
        b->owner   = PROJ_PLAYER;
        b->color   = (Color){0, 220, 255, 255};
        b->trailColor = (Color){0, 120, 200, 180};
        b->active  = 1;
        return 1;
    }
    return 0; /* pool full */
}

int Projectile_SpawnEnemy(ProjectilePool *pool, Vector2 pos, Vector2 dir,
                          int damage, Color color) {
    return Projectile_SpawnEnemyEx(pool, pos, dir, ENEMY_BULLET_SPEED, ENEMY_BULLET_RADIUS, damage, color);
}

int Projectile_SpawnEnemyEx(ProjectilePool *pool, Vector2 pos, Vector2 dir,
                            float speed, float radius, int damage, Color color) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        Projectile *b = &pool->enemy[i];
        if (b->active) continue;
        b->pos     = pos;
        b->prevPos = pos;
        b->vel     = Vec2Scale(dir, speed);
        b->life    = ENEMY_BULLET_LIFE;
        b->radius  = radius;
        b->damage  = damage;
        b->owner   = PROJ_ENEMY;
        b->color   = color;
        b->trailColor = ColorAlpha(color, 0.5f);
        b->active  = 1;
        return 1;
    }
    return 0;
}

void Projectiles_KillPlayer(ProjectilePool *pool, int index) {
    pool->player[index].active = 0;
}

void Projectiles_KillEnemy(ProjectilePool *pool, int index) {
    pool->enemy[index].active = 0;
}

