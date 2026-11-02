/**
 * powerup.c - Power-up logic and rendering
 */
#include "powerup.h"
#include "utils.h"
#include "raylib.h"
#include <string.h>
#include <math.h>

static const Color kColors[POWERUP_TYPE_COUNT] = {
    [POWERUP_HEALTH]     = {0, 255, 100, 255},
    [POWERUP_SHIELD]     = {0, 160, 255, 255},
    [POWERUP_RAPIDFIRE]  = {255, 160, 0, 255},
    [POWERUP_DOUBLESHOT] = {0, 255, 200, 255},
    [POWERUP_SPREAD]     = {255, 220, 0, 255},
    [POWERUP_INVINCIBLE] = {255, 230, 0, 255},
};

static const char *kNames[POWERUP_TYPE_COUNT] = {
    [POWERUP_HEALTH]     = "HEALTH",
    [POWERUP_SHIELD]     = "SHIELD",
    [POWERUP_RAPIDFIRE]  = "RAPID FIRE",
    [POWERUP_DOUBLESHOT] = "DOUBLE SHOT",
    [POWERUP_SPREAD]     = "SPREAD SHOT",
    [POWERUP_INVINCIBLE] = "INVINCIBLE",
};

Color Powerup_GetColor(PowerupType type) {
    if (type < 0 || type >= POWERUP_TYPE_COUNT) return WHITE;
    return kColors[type];
}

const char *Powerup_GetName(PowerupType type) {
    if (type < 0 || type >= POWERUP_TYPE_COUNT) return "UNKNOWN";
    return kNames[type];
}

void Powerups_Init(PowerupPool *pool) {
    memset(pool, 0, sizeof(*pool));
}

void Powerups_Clear(PowerupPool *pool) {
    for (int i = 0; i < MAX_POWERUPS; i++) pool->powerups[i].active = 0;
}

int Powerup_Spawn(PowerupPool *pool, Vector2 pos, PowerupType type) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!pool->powerups[i].active) {
            pool->powerups[i].pos      = pos;
            pool->powerups[i].type     = type;
            pool->powerups[i].life     = POWERUP_LIFETIME;
            pool->powerups[i].bobTimer = RandFloat(0.0f, 6.28f);
            pool->powerups[i].active   = 1;
            return 1;
        }
    }
    return 0;
}

int Powerup_SpawnRandom(PowerupPool *pool, Vector2 pos) {
    PowerupType type = (PowerupType)GetRandomValue(0, POWERUP_TYPE_COUNT - 1);
    return Powerup_Spawn(pool, pos, type);
}

void Powerup_Kill(PowerupPool *pool, int index) {
    pool->powerups[index].active = 0;
}

void Powerups_Update(PowerupPool *pool, float dt) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        Powerup *p = &pool->powerups[i];
        if (!p->active) continue;
        p->life -= dt;
        p->bobTimer += dt;
        if (p->life <= 0.0f) p->active = 0;
    }
}

static void DrawPowerupIcon(Vector2 pos, PowerupType type, float size, Color color) {
    switch (type) {
        case POWERUP_HEALTH: {
            /* Plus sign */
            float t = size * 0.35f;
            DrawRectangle((int)(pos.x - t * 0.5f), (int)(pos.y - size * 0.5f), (int)t, (int)size, color);
            DrawRectangle((int)(pos.x - size * 0.5f), (int)(pos.y - t * 0.5f), (int)size, (int)t, color);
            break;
        }
        case POWERUP_SHIELD: {
            /* Shield circle */
            DrawCircleLinesV(pos, size * 0.5f, color);
            DrawCircleLinesV(pos, size * 0.35f, ColorAlpha(color, 0.6f));
            break;
        }
        case POWERUP_RAPIDFIRE: {
            /* Three parallel lines */
            for (int k = -1; k <= 1; k++) {
                Vector2 a = {pos.x + k * size * 0.25f, pos.y - size * 0.4f};
                Vector2 b = {pos.x + k * size * 0.25f, pos.y + size * 0.4f};
                DrawLineV(a, b, color);
            }
            break;
        }
        case POWERUP_DOUBLESHOT: {
            /* Two dots */
            DrawCircleV((Vector2){pos.x - size * 0.22f, pos.y}, size * 0.2f, color);
            DrawCircleV((Vector2){pos.x + size * 0.22f, pos.y}, size * 0.2f, color);
            break;
        }
        case POWERUP_SPREAD: {
            /* Three diverging lines (fan shape) */
            float h = size * 0.45f;
            Vector2 base = {pos.x, pos.y + h * 0.3f};
            DrawLineV(base, (Vector2){pos.x, pos.y - h}, color);
            DrawLineV(base, (Vector2){pos.x - h * 0.7f, pos.y - h * 0.5f}, color);
            DrawLineV(base, (Vector2){pos.x + h * 0.7f, pos.y - h * 0.5f}, color);
            break;
        }
        case POWERUP_INVINCIBLE: {
            /* Star */
            for (int k = 0; k < 5; k++) {
                float a = DEG2RAD(k * 72.0f - 90.0f);
                float b = DEG2RAD(k * 72.0f - 90.0f + 36.0f);
                Vector2 outer = {pos.x + cosf(a) * size * 0.5f, pos.y + sinf(a) * size * 0.5f};
                Vector2 inner = {pos.x + cosf(b) * size * 0.2f, pos.y + sinf(b) * size * 0.2f};
                DrawLineV(outer, inner, color);
                Vector2 outer2 = {pos.x + cosf(b + DEG2RAD(36.0f)) * size * 0.5f,
                                  pos.y + sinf(b + DEG2RAD(36.0f)) * size * 0.5f};
                DrawLineV(inner, outer2, color);
            }
            break;
        }
        default: break;
    }
}

void Powerups_Draw(const PowerupPool *pool) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        const Powerup *p = &pool->powerups[i];
        if (!p->active) continue;

        Color col = kColors[p->type];
        float pulse = 0.6f + 0.4f * sinf(p->bobTimer * 3.5f);
        float bob   = sinf(p->bobTimer * 2.5f) * 4.0f;
        Vector2 drawPos = { p->pos.x, p->pos.y + bob };

        /* Flicker when about to expire */
        if (p->life < 3.0f) {
            int blink = (int)(p->life * 5.0f) % 2;
            if (blink) continue;
        }

        /* Outer glow */
        DrawCircleV(drawPos, POWERUP_RADIUS + 6.0f * pulse, ColorAlpha(col, 0.15f));
        DrawCircleV(drawPos, POWERUP_RADIUS + 2.0f * pulse, ColorAlpha(col, 0.30f));
        /* Body */
        DrawCircleV(drawPos, POWERUP_RADIUS, ColorAlpha(col, 0.25f));
        DrawCircleLinesV(drawPos, POWERUP_RADIUS, ColorAlpha(col, (unsigned char)(180 + pulse * 75)));
        /* Icon */
        DrawPowerupIcon(drawPos, p->type, POWERUP_RADIUS * 1.1f, col);
    }
}

