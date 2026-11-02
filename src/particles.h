/**
 * particles.h - Particle system
 */
#ifndef SPACEJAM_PARTICLES_H
#define SPACEJAM_PARTICLES_H

#include "raylib.h"
#include "config.h"

typedef enum {
    PARTICLE_NONE = 0,
    PARTICLE_EXPLOSION,
    PARTICLE_SPARK,
    PARTICLE_EXHAUST,
    PARTICLE_TRAIL,
    PARTICLE_POWERUP,
    PARTICLE_DEBRIS
} ParticleType;

typedef struct {
    Vector2      pos;
    Vector2      vel;
    Color        color;
    Color        colorEnd;
    float        life;
    float        lifeMax;
    float        radius;
    float        radiusEnd;
    float        rotation;
    float        rotSpeed;
    ParticleType type;
    int          active;
} Particle;

typedef struct {
    Particle particles[MAX_PARTICLES];
    int      count;   /* number of active slots (not used for pool-search opt) */
} ParticleSystem;

void Particles_Init(ParticleSystem *ps);
void Particles_Update(ParticleSystem *ps, float dt);
void Particles_Draw(const ParticleSystem *ps);
void Particles_Clear(ParticleSystem *ps);

/* Emitter helpers */
void Particles_SpawnExplosion(ParticleSystem *ps, Vector2 pos, float radius, Color base, int count);
void Particles_SpawnSparks(ParticleSystem *ps, Vector2 pos, int count, Color color);
void Particles_SpawnExhaust(ParticleSystem *ps, Vector2 pos, Vector2 dir, Color color);
void Particles_SpawnTrail(ParticleSystem *ps, Vector2 pos, Color color);
void Particles_SpawnPowerup(ParticleSystem *ps, Vector2 pos, Color color);
void Particles_SpawnDebris(ParticleSystem *ps, Vector2 pos, int count, Color color);

#endif /* SPACEJAM_PARTICLES_H */

