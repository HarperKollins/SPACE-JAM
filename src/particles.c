/**
 * particles.c - Particle system implementation
 */
#include "particles.h"
#include "utils.h"
#include <math.h>

/* ── Pool management ─────────────────────────────────── */

void Particles_Init(ParticleSystem *ps) {
    for (int i = 0; i < MAX_PARTICLES; i++) ps->particles[i].active = 0;
    ps->count = 0;
}

void Particles_Clear(ParticleSystem *ps) {
    for (int i = 0; i < MAX_PARTICLES; i++) ps->particles[i].active = 0;
    ps->count = 0;
}

/* Find a free slot; returns -1 if pool is full */
static int FindFreeSlot(ParticleSystem *ps) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!ps->particles[i].active) return i;
    }
    return -1;
}

static Particle *AllocParticle(ParticleSystem *ps) {
    int idx = FindFreeSlot(ps);
    if (idx < 0) return NULL;
    ps->count++;
    return &ps->particles[idx];
}

/* ── Update & Draw ────────────────────────────────────── */

void Particles_Update(ParticleSystem *ps, float dt) {
    ps->count = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &ps->particles[i];
        if (!p->active) continue;
        ps->count++;

        p->life -= dt;
        if (p->life <= 0.0f) { p->active = 0; continue; }

        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;
        float drag = powf(0.985f, dt * 60.0f);
        p->vel.x *= drag;
        p->vel.y *= drag;
        p->rotation += p->rotSpeed * dt;
    }
}

void Particles_Draw(const ParticleSystem *ps) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle *p = &ps->particles[i];
        if (!p->active) continue;

        float t = 1.0f - (p->life / p->lifeMax);   /* 0=new, 1=dying */
        float alpha = 1.0f - t;
        float radius = p->radius + (p->radiusEnd - p->radius) * t;
        Color col = ColorLerp(p->color, p->colorEnd, t);
        col = ColorAlpha(col, alpha);

        if (radius <= 2.0f) {
            DrawPixelV(p->pos, col);
        } else {
            DrawCircleV(p->pos, radius, col);
        }
    }
}

/* ── Emitters ────────────────────────────────────────── */

void Particles_SpawnExplosion(ParticleSystem *ps, Vector2 pos, float radius,
                              Color base, int count) {
    for (int i = 0; i < count; i++) {
        Particle *p = AllocParticle(ps);
        if (!p) return;

        float angle = RandFloat(0.0f, 360.0f);
        float speed = RandFloat(60.0f, 280.0f + radius * 2.0f);
        p->pos = pos;
        p->vel = Vec2Scale(AngleToDir(angle), speed);
        p->life = RandFloat(0.4f, 1.1f);
        p->lifeMax = p->life;
        p->radius = RandFloat(2.0f, radius * 0.4f);
        p->radiusEnd = 0.5f;
        p->color = base;
        p->colorEnd = (Color){ 255, 60, 0, 255 };
        p->rotation = angle;
        p->rotSpeed = RandFloat(-120.0f, 120.0f);
        p->type = PARTICLE_EXPLOSION;
        p->active = 1;
    }
    /* Central bright flash */
    Particle *flash = AllocParticle(ps);
    if (flash) {
        flash->pos = pos;
        flash->vel = (Vector2){0, 0};
        flash->life = 0.2f;
        flash->lifeMax = 0.2f;
        flash->radius = radius * 0.8f;
        flash->radiusEnd = radius * 1.5f;
        flash->color = WHITE;
        flash->colorEnd = (Color){255, 200, 50, 0};
        flash->rotSpeed = 0;
        flash->type = PARTICLE_EXPLOSION;
        flash->active = 1;
    }
}

void Particles_SpawnSparks(ParticleSystem *ps, Vector2 pos, int count, Color color) {
    for (int i = 0; i < count; i++) {
        Particle *p = AllocParticle(ps);
        if (!p) return;

        float angle = RandFloat(0.0f, 360.0f);
        float speed = RandFloat(80.0f, 220.0f);
        p->pos = pos;
        p->vel = Vec2Scale(AngleToDir(angle), speed);
        p->life = RandFloat(0.15f, 0.45f);
        p->lifeMax = p->life;
        p->radius = 1.5f;
        p->radiusEnd = 0.0f;
        p->color = color;
        p->colorEnd = ColorAlpha(color, 0.0f);
        p->rotSpeed = 0;
        p->type = PARTICLE_SPARK;
        p->active = 1;
    }
}

void Particles_SpawnExhaust(ParticleSystem *ps, Vector2 pos, Vector2 dir, Color color) {
    Particle *p = AllocParticle(ps);
    if (!p) return;

    /* Slightly randomize exhaust direction */
    float jitter = RandFloat(-20.0f, 20.0f);
    float angle = RAD2DEG(atan2f(dir.x, -dir.y)) + 180.0f + jitter;

    p->pos = pos;
    p->vel = Vec2Scale(AngleToDir(angle), RandFloat(50.0f, 130.0f));
    p->life = RandFloat(0.12f, 0.28f);
    p->lifeMax = p->life;
    p->radius = RandFloat(2.0f, 5.0f);
    p->radiusEnd = 0.0f;
    p->color = color;
    p->colorEnd = ColorAlpha(color, 0.0f);
    p->rotSpeed = 0;
    p->type = PARTICLE_EXHAUST;
    p->active = 1;
}

void Particles_SpawnTrail(ParticleSystem *ps, Vector2 pos, Color color) {
    Particle *p = AllocParticle(ps);
    if (!p) return;

    p->pos = pos;
    p->vel = (Vector2){ RandFloat(-8.0f, 8.0f), RandFloat(-8.0f, 8.0f) };
    p->life = 0.08f;
    p->lifeMax = 0.08f;
    p->radius = 3.0f;
    p->radiusEnd = 1.0f;
    p->color = color;
    p->colorEnd = ColorAlpha(color, 0.0f);
    p->rotSpeed = 0;
    p->type = PARTICLE_TRAIL;
    p->active = 1;
}

void Particles_SpawnPowerup(ParticleSystem *ps, Vector2 pos, Color color) {
    for (int i = 0; i < 20; i++) {
        Particle *p = AllocParticle(ps);
        if (!p) return;

        float angle = RandFloat(0.0f, 360.0f);
        p->pos = pos;
        p->vel = Vec2Scale(AngleToDir(angle), RandFloat(30.0f, 120.0f));
        p->life = RandFloat(0.5f, 1.2f);
        p->lifeMax = p->life;
        p->radius = 3.0f;
        p->radiusEnd = 0.0f;
        p->color = WHITE;
        p->colorEnd = color;
        p->rotSpeed = 0;
        p->type = PARTICLE_POWERUP;
        p->active = 1;
    }
}

void Particles_SpawnDebris(ParticleSystem *ps, Vector2 pos, int count, Color color) {
    for (int i = 0; i < count; i++) {
        Particle *p = AllocParticle(ps);
        if (!p) return;

        float angle = RandFloat(0.0f, 360.0f);
        p->pos = pos;
        p->vel = Vec2Scale(AngleToDir(angle), RandFloat(20.0f, 160.0f));
        p->life = RandFloat(0.6f, 1.8f);
        p->lifeMax = p->life;
        p->radius = RandFloat(1.5f, 4.0f);
        p->radiusEnd = 0.0f;
        p->color = color;
        p->colorEnd = ColorAlpha(color, 0.0f);
        p->rotation = angle;
        p->rotSpeed = RandFloat(-80.0f, 80.0f);
        p->type = PARTICLE_DEBRIS;
        p->active = 1;
    }
}

