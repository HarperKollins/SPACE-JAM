/**
 * asteroid.c - Asteroid field implementation
 */
#include "asteroid.h"
#include "utils.h"
#include "raylib.h"
#include <math.h>
#include <string.h>

/* ─── Init ───────────────────────────────────────────── */

static void InitAsteroidShape(Asteroid *a) {
    for (int i = 0; i < 8; i++) {
        a->shapeOffsets[i] = RandFloat(0.65f, 1.35f);
    }
    /* Slightly muted grey-brown palette */
    int grey = GetRandomValue(90, 155);
    a->color     = (Color){ (unsigned char)(grey), (unsigned char)(grey - 10),
                            (unsigned char)(grey - 20), 255 };
    a->colorEdge = (Color){ (unsigned char)MIN(255, grey + 50),
                            (unsigned char)MIN(255, grey + 40),
                            (unsigned char)MIN(255, grey + 30), 255 };
}

static void SetupAsteroidBySize(Asteroid *a, AsteroidSize size) {
    a->size = size;
    switch (size) {
        case ASTEROID_LARGE:
            a->radius  = ASTEROID_LARGE_RADIUS;
            a->health  = ASTEROID_LARGE_HP;
            break;
        case ASTEROID_MEDIUM:
            a->radius  = ASTEROID_MEDIUM_RADIUS;
            a->health  = ASTEROID_MEDIUM_HP;
            break;
        case ASTEROID_SMALL:
        default:
            a->radius  = ASTEROID_SMALL_RADIUS;
            a->health  = ASTEROID_SMALL_HP;
            break;
    }
    InitAsteroidShape(a);
}

/* ─── Pool management ────────────────────────────────── */

void Asteroids_Init(AsteroidField *af) {
    memset(af, 0, sizeof(*af));
    af->spawnInterval = ASTEROID_SPAWN_INTERVAL;
}

void Asteroids_Clear(AsteroidField *af) {
    for (int i = 0; i < MAX_ASTEROIDS; i++) af->asteroids[i].active = 0;
}

void Asteroids_SetSpawnRate(AsteroidField *af, int wave) {
    float factor = 1.0f - (float)(wave - 1) * 0.08f;
    af->spawnInterval = MAX(0.6f, ASTEROID_SPAWN_INTERVAL * CLAMP(factor, 0.3f, 1.0f));
}

static int FindFreeAsteroid(AsteroidField *af) {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!af->asteroids[i].active) return i;
    }
    return -1;
}

int Asteroid_Spawn(AsteroidField *af, AsteroidSize size, int screenW, int screenH) {
    int idx = FindFreeAsteroid(af);
    if (idx < 0) return 0;

    Asteroid *a = &af->asteroids[idx];
    a->active = 1;
    SetupAsteroidBySize(a, size);

    /* Spawn on a random edge */
    int edge = GetRandomValue(0, 3);
    float margin = a->radius + 10.0f;
    switch (edge) {
        case 0:
            a->pos = (Vector2){ RandFloat(0, (float)screenW), -margin };
            break;
        case 1:
            a->pos = (Vector2){ (float)screenW + margin, RandFloat(0, (float)screenH) };
            break;
        case 2:
            a->pos = (Vector2){ RandFloat(0, (float)screenW), (float)screenH + margin };
            break;
        default:
            a->pos = (Vector2){ -margin, RandFloat(0, (float)screenH) };
            break;
    }

    /* Random angle toward screen center-ish */
    float centerX = (float)screenW * 0.5f;
    float centerY = (float)screenH * 0.5f;
    Vector2 toCenter = Vec2Norm((Vector2){ centerX - a->pos.x, centerY - a->pos.y });
    float baseAngle  = RAD2DEG(atan2f(toCenter.x, -toCenter.y));
    float randAngle  = baseAngle + RandFloat(-35.0f, 35.0f);

    float speed = 0.0f;
    switch (size) {
        case ASTEROID_LARGE:  speed = RandFloat(ASTEROID_LARGE_SPEED * 0.6f,  ASTEROID_LARGE_SPEED * 1.4f);  break;
        case ASTEROID_MEDIUM: speed = RandFloat(ASTEROID_MEDIUM_SPEED * 0.6f, ASTEROID_MEDIUM_SPEED * 1.4f); break;
        case ASTEROID_SMALL:
        default:              speed = RandFloat(ASTEROID_SMALL_SPEED * 0.6f,  ASTEROID_SMALL_SPEED * 1.4f);  break;
    }

    a->vel      = Vec2Scale(AngleToDir(randAngle), speed);
    a->rotation = RandFloat(0.0f, 360.0f);
    a->rotSpeed = RandFloat(-60.0f, 60.0f);
    return 1;
}

/* ─── Update ─────────────────────────────────────────── */

void Asteroids_Update(AsteroidField *af, float dt, int screenW, int screenH, int wave) {
    /* Auto-spawn */
    af->spawnTimer += dt;
    Asteroids_SetSpawnRate(af, wave);

    if (af->spawnTimer >= af->spawnInterval) {
        af->spawnTimer = 0.0f;
        /* Weighted random: more large in low waves, more small later */
        int r = GetRandomValue(0, 9);
        AsteroidSize sz;
        if      (r < 4)  sz = ASTEROID_LARGE;
        else if (r < 7)  sz = ASTEROID_MEDIUM;
        else             sz = ASTEROID_SMALL;
        Asteroid_Spawn(af, sz, screenW, screenH);
    }

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        Asteroid *a = &af->asteroids[i];
        if (!a->active) continue;

        a->pos.x += a->vel.x * dt;
        a->pos.y += a->vel.y * dt;
        a->rotation += a->rotSpeed * dt;

        /* Remove if far off-screen */
        float margin = a->radius + 80.0f;
        if (a->pos.x < -margin || a->pos.x > screenW + margin ||
            a->pos.y < -margin || a->pos.y > screenH + margin) {
            a->active = 0;
        }
    }
}

/* ─── Draw ───────────────────────────────────────────── */

static void DrawAsteroid(const Asteroid *a) {
    int sides = 8;
    float rot = DEG2RAD(a->rotation);
    Vector2 pts[8];

    for (int i = 0; i < sides; i++) {
        float angle = rot + DEG2RAD((float)i * (360.0f / sides));
        float r = a->radius * a->shapeOffsets[i];
        pts[i] = (Vector2){ a->pos.x + cosf(angle) * r,
                            a->pos.y + sinf(angle) * r };
    }

    /* Fill base rock faceted triangles from center */
    for (int i = 0; i < sides; i++) {
        /* Subtle shading variation per facet */
        Color facetCol = (i % 2 == 0) ? a->color : ColorAlpha(a->color, 0.85f);
        DrawTriangle(a->pos, pts[i], pts[(i + 1) % sides], facetCol);
    }

    /* Rock craters for texture & depth */
    if (a->radius >= ASTEROID_MEDIUM_RADIUS) {
        float cosR = cosf(rot);
        float sinR = sinf(rot);
        Color craterShade = { (unsigned char)MAX(0, a->color.r - 35),
                              (unsigned char)MAX(0, a->color.g - 35),
                              (unsigned char)MAX(0, a->color.b - 35), 220 };
        Color craterRim   = { (unsigned char)MIN(255, a->colorEdge.r + 20),
                              (unsigned char)MIN(255, a->colorEdge.g + 20),
                              (unsigned char)MIN(255, a->colorEdge.b + 20), 180 };

        /* Crater 1 */
        float c1x = a->radius * 0.32f, c1y = -a->radius * 0.22f;
        Vector2 cr1 = { a->pos.x + c1x * cosR - c1y * sinR,
                        a->pos.y + c1x * sinR + c1y * cosR };
        DrawCircleV(cr1, a->radius * 0.24f, craterShade);
        DrawCircleLinesV(cr1, a->radius * 0.24f, craterRim);

        /* Crater 2 */
        float c2x = -a->radius * 0.28f, c2y = a->radius * 0.25f;
        Vector2 cr2 = { a->pos.x + c2x * cosR - c2y * sinR,
                        a->pos.y + c2x * sinR + c2y * cosR };
        DrawCircleV(cr2, a->radius * 0.18f, craterShade);
        DrawCircleLinesV(cr2, a->radius * 0.18f, craterRim);
    }

    /* Outer structural ridge lines */
    for (int i = 0; i < sides; i++) {
        DrawLineV(pts[i], pts[(i + 1) % sides], a->colorEdge);
    }
}

void Asteroids_Draw(const AsteroidField *af) {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (af->asteroids[i].active) DrawAsteroid(&af->asteroids[i]);
    }
}

/* ─── Hit / Split ────────────────────────────────────── */

int Asteroid_Hit(AsteroidField *af, int index, int damage,
                 int screenW, int screenH) {
    Asteroid *a = &af->asteroids[index];
    if (!a->active) return 0;

    a->health -= damage;
    if (a->health > 0) return 0;

    int score = 0;
    /* Split large → 2 medium, medium → 2 small */
    if (a->size == ASTEROID_LARGE) {
        score = ASTEROID_LARGE_SCORE;
        for (int i = 0; i < 2; i++) {
            int idx = FindFreeAsteroid(af);
            if (idx < 0) break;
            Asteroid *child = &af->asteroids[idx];
            child->active = 1;
            SetupAsteroidBySize(child, ASTEROID_MEDIUM);
            child->pos = a->pos;
            float ang = RandFloat(0.0f, 360.0f) + (float)i * 180.0f;
            child->vel = Vec2Scale(AngleToDir(ang), ASTEROID_MEDIUM_SPEED * RandFloat(0.7f, 1.3f));
            child->rotation = RandFloat(0.0f, 360.0f);
            child->rotSpeed = RandFloat(-70.0f, 70.0f);
        }
    } else if (a->size == ASTEROID_MEDIUM) {
        score = ASTEROID_MEDIUM_SCORE;
        for (int i = 0; i < 2; i++) {
            int idx = FindFreeAsteroid(af);
            if (idx < 0) break;
            Asteroid *child = &af->asteroids[idx];
            child->active = 1;
            SetupAsteroidBySize(child, ASTEROID_SMALL);
            child->pos = a->pos;
            float ang = RandFloat(0.0f, 360.0f) + (float)i * 180.0f;
            child->vel = Vec2Scale(AngleToDir(ang), ASTEROID_SMALL_SPEED * RandFloat(0.7f, 1.3f));
            child->rotation = RandFloat(0.0f, 360.0f);
            child->rotSpeed = RandFloat(-90.0f, 90.0f);
        }
    } else {
        score = ASTEROID_SMALL_SCORE;
    }

    a->active = 0;
    (void)screenW; (void)screenH;
    return score;
}

