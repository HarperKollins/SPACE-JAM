/**
 * relic.h - The Seven Vectors Relic System
 *
 * Implements the ancient Architect relics recovered across the galaxy:
 * - Genesis Vector: Passive weapon overcharge & piercing
 * - Gravity Vector: Active singularity vortex (Key Q)
 * - Temporal Vector: Active chrono-stasis & hull rewind (Key E)
 * - Void Vector: Active matter phasing (Key F)
 */
#ifndef SPACEJAM_RELIC_H
#define SPACEJAM_RELIC_H

#include "raylib.h"
#include "player.h"
#include "enemy.h"
#include "asteroid.h"
#include "particles.h"
#include "audio.h"
#include "progress.h"

typedef struct {
    uint32_t unlockedMask;

    /* 1. Gravity Vector (Key Q): Singularity Implosion */
    float    gravityCooldown;
    float    gravityDuration;
    Vector2  gravityPos;
    int      gravityActive;

    /* 2. Temporal Vector (Key E): Chrono-Stasis & Rewind */
    float    temporalCooldown;
    float    temporalDuration;
    int      temporalActive;

    /* 3. Void Vector (Key F): Phasing */
    float    voidCooldown;
    float    voidDuration;
    int      voidActive;

    float    animTime;
} RelicSystem;

void Relics_Init(RelicSystem *rs, const PlayerProgress *prog);
void Relics_Update(RelicSystem *rs, Player *player, EnemyPool *enemies, AsteroidField *asteroids,
                   ParticleSystem *particles, AudioManager *audio, float dt, int screenW, int screenH);
void Relics_DrawEffects(const RelicSystem *rs);
void Relics_DrawHUD(const RelicSystem *rs, int screenW, int screenH);
void Relics_Unlock(RelicSystem *rs, VectorId vectorId, PlayerProgress *prog);

#endif /* SPACEJAM_RELIC_H */
