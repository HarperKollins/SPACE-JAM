/**
 * relic.c - Vector Relics implementation (Gravity, Temporal, Void, Genesis)
 */
#include "relic.h"
#include "utils.h"
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void Relics_Init(RelicSystem *rs, const PlayerProgress *prog) {
    rs->unlockedMask     = prog->relicsBitmask;
    /* Always unlock Genesis Vector by default (recovered in prologue from Earth) */
    rs->unlockedMask    |= (1u << VECTOR_GENESIS);

    /* Unlock Gravity Vector if player reached Sector 2 or higher */
    if (prog->maxSectorReached >= 2) {
        rs->unlockedMask |= (1u << VECTOR_GRAVITY);
    }
    /* Unlock Temporal Vector if player reached Sector 3 or higher */
    if (prog->maxSectorReached >= 3) {
        rs->unlockedMask |= (1u << VECTOR_TEMPORAL);
    }
    /* Unlock Void Vector if player reached Sector 4 or higher */
    if (prog->maxSectorReached >= 4) {
        rs->unlockedMask |= (1u << VECTOR_VOID);
    }

    rs->gravityCooldown  = 0.0f;
    rs->gravityDuration  = 0.0f;
    rs->gravityPos       = (Vector2){0, 0};
    rs->gravityActive    = 0;

    rs->temporalCooldown = 0.0f;
    rs->temporalDuration = 0.0f;
    rs->temporalActive   = 0;

    rs->voidCooldown     = 0.0f;
    rs->voidDuration     = 0.0f;
    rs->voidActive       = 0;

    rs->animTime         = 0.0f;
}

void Relics_Unlock(RelicSystem *rs, VectorId vectorId, PlayerProgress *prog) {
    if ((int)vectorId < 0 || (int)vectorId >= VECTOR_COUNT) return;
    rs->unlockedMask |= (1u << (uint32_t)vectorId);
    Progress_UnlockRelic(prog, vectorId);
}

void Relics_Update(RelicSystem *rs, Player *player, EnemyPool *enemies, AsteroidField *asteroids,
                   ParticleSystem *particles, AudioManager *audio, float dt, int screenW, int screenH) {
    (void)screenW;
    (void)screenH;
    rs->animTime += dt;

    /* Cooldown decrements */
    if (rs->gravityCooldown > 0.0f)  rs->gravityCooldown  -= dt;
    if (rs->temporalCooldown > 0.0f) rs->temporalCooldown -= dt;
    if (rs->voidCooldown > 0.0f)     rs->voidCooldown     -= dt;

    /* -- 1. Gravity Vector (Key Q) Activation -- */
    if ((rs->unlockedMask & (1u << VECTOR_GRAVITY)) && rs->gravityCooldown <= 0.0f) {
        if (IsKeyPressed(KEY_Q)) {
            rs->gravityActive   = 1;
            rs->gravityDuration = 3.2f;
            rs->gravityCooldown = 12.0f;

            /* Target mouse position or 240px ahead of ship */
            Vector2 m = GetMousePosition();
            if (m.x > 0 && m.x < screenW && m.y > 0 && m.y < screenH) {
                rs->gravityPos = m;
            } else {
                Vector2 fwd = AngleToDir(player->rotation);
                rs->gravityPos = (Vector2){ player->pos.x + fwd.x * 240.0f,
                                            player->pos.y + fwd.y * 240.0f };
            }

            Particles_SpawnExplosion(particles, rs->gravityPos, 40.0f, (Color){ 180, 40, 255, 255 }, 24);
            Audio_PlaySFX(audio, SFX_EXPLOSION_LARGE);
        }
    }

    /* Process Active Gravity Singularity */
    if (rs->gravityActive) {
        rs->gravityDuration -= dt;
        if (rs->gravityDuration <= 0.0f) {
            rs->gravityActive = 0;
            Particles_SpawnExplosion(particles, rs->gravityPos, 50.0f, (Color){ 255, 100, 255, 255 }, 30);
            Audio_PlaySFX(audio, SFX_EXPLOSION_LARGE);
        } else {
            /* Pull particles into the event horizon */
            if (RandChance(0.6f)) {
                Vector2 spawnP = { rs->gravityPos.x + RandFloat(-180.0f, 180.0f),
                                   rs->gravityPos.y + RandFloat(-180.0f, 180.0f) };
                Particles_SpawnSparks(particles, spawnP, 2, (Color){ 200, 50, 255, 200 });
            }

            /* Pull and crush all enemies */
            for (int i = 0; i < MAX_ENEMIES; i++) {
                Enemy *e = &enemies->enemies[i];
                if (!e->active || e->state == ESTATE_DYING) continue;
                float dx = rs->gravityPos.x - e->pos.x;
                float dy = rs->gravityPos.y - e->pos.y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < 360.0f && dist > 8.0f) {
                    float pull = (1.0f - dist / 360.0f) * 750.0f;
                    e->pos.x += (dx / dist) * pull * dt;
                    e->pos.y += (dy / dist) * pull * dt;
                    /* Crushing singularity damage */
                    if (dist < 60.0f) {
                        e->health -= (int)(60.0f * dt);
                        if (e->health <= 0) {
                            e->health = 0;
                            e->state = ESTATE_DYING;
                            e->stateTimer = 0.4f;
                            Particles_SpawnExplosion(particles, e->pos, e->radius, (Color){ 180, 0, 255, 255 }, 16);
                        }
                    }
                }
            }

            /* Pull and fragment asteroids */
            for (int i = 0; i < MAX_ASTEROIDS; i++) {
                Asteroid *a = &asteroids->asteroids[i];
                if (!a->active) continue;
                float dx = rs->gravityPos.x - a->pos.x;
                float dy = rs->gravityPos.y - a->pos.y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < 320.0f && dist > 10.0f) {
                    float pull = (1.0f - dist / 320.0f) * 500.0f;
                    a->pos.x += (dx / dist) * pull * dt;
                    a->pos.y += (dy / dist) * pull * dt;
                }
            }
        }
    }

    /* -- 2. Temporal Vector (Key E) Activation -- */
    if ((rs->unlockedMask & (1u << VECTOR_TEMPORAL)) && rs->temporalCooldown <= 0.0f) {
        if (IsKeyPressed(KEY_E)) {
            rs->temporalActive   = 1;
            rs->temporalDuration = 3.8f;
            rs->temporalCooldown = 18.0f;

            /* Chrono Rewind: Restore player hull & shields */
            player->health = MIN(PLAYER_MAX_HEALTH, player->health + 35);
            player->shieldTimer = 3.5f;
            player->shieldActive = 1;

            Particles_SpawnSparks(particles, player->pos, 25, (Color){ 0, 255, 180, 255 });
            Audio_PlaySFX(audio, SFX_WARP_JUMP);
        }
    }

    /* Process Active Temporal Chrono-Stasis */
    if (rs->temporalActive) {
        rs->temporalDuration -= dt;
        if (rs->temporalDuration <= 0.0f) {
            rs->temporalActive = 0;
        } else {
            /* Slow down all enemy movement by 75% */
            for (int i = 0; i < MAX_ENEMIES; i++) {
                Enemy *e = &enemies->enemies[i];
                if (!e->active) continue;
                e->vel.x *= (1.0f - dt * 2.5f);
                e->vel.y *= (1.0f - dt * 2.5f);
            }
        }
    }

    /* -- 3. Void Vector (Key F) Activation -- */
    if ((rs->unlockedMask & (1u << VECTOR_VOID)) && rs->voidCooldown <= 0.0f) {
        if (IsKeyPressed(KEY_F)) {
            rs->voidActive   = 1;
            rs->voidDuration = 3.5f;
            rs->voidCooldown = 15.0f;

            player->invincibleTimer = 3.5f;
            Particles_SpawnExplosion(particles, player->pos, 30.0f, (Color){ 100, 200, 255, 200 }, 18);
            Audio_PlaySFX(audio, SFX_POWERUP);
        }
    }

    if (rs->voidActive) {
        rs->voidDuration -= dt;
        if (rs->voidDuration <= 0.0f) {
            rs->voidActive = 0;
        }
    }
}

void Relics_DrawEffects(const RelicSystem *rs) {
    /* 1. Draw Gravity Singularity Accretion Disk */
    if (rs->gravityActive) {
        Vector2 c = rs->gravityPos;
        float pulse = 0.5f + 0.5f * sinf(rs->animTime * 12.0f);

        /* Outer gravitational distortion glow */
        DrawCircleV(c, 160.0f + pulse * 20.0f, (Color){ 80, 0, 160, 45 });
        DrawCircleV(c, 90.0f + pulse * 14.0f,  (Color){ 140, 20, 220, 80 });

        /* Swirling accretion rings */
        for (int r = 0; r < 4; r++) {
            float rad = 45.0f + (float)r * 22.0f + sinf(rs->animTime * 6.0f + (float)r) * 8.0f;
            DrawCircleLinesV(c, rad, (Color){ 200, 80, 255, (unsigned char)(140 - r * 25) });
        }

        /* Black hole event horizon */
        DrawCircleV(c, 24.0f, (Color){ 5, 2, 12, 250 });
        DrawCircleLinesV(c, 25.0f, (Color){ 255, 120, 255, 255 });
    }

    /* 2. Draw Temporal Chrono-Stasis Screen Vignette */
    if (rs->temporalActive) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        float tPulse = 0.5f + 0.5f * sinf(rs->animTime * 8.0f);
        Color emeraldVig = (Color){ 0, 240, 160, (unsigned char)(30 + tPulse * 30) };

        DrawRectangleLinesEx((Rectangle){ 0, 0, (float)sw, (float)sh }, 8.0f, emeraldVig);
        DrawRectangleLinesEx((Rectangle){ 8, 8, (float)(sw - 16), (float)(sh - 16) }, 4.0f,
                             ColorAlpha(emeraldVig, 0.4f));

        /* Subtle Chrono-Stasis banner */
        const char *tMsg = "[ TEMPORAL STASIS ACTIVE // TIME SLOWED 75% ]";
        int tw = MeasureText(tMsg, 14);
        DrawText(tMsg, sw / 2 - tw / 2, 75, 14, (Color){ 0, 255, 180, (unsigned char)(180 + tPulse * 75) });
    }
}

void Relics_DrawHUD(const RelicSystem *rs, int screenW, int screenH) {
    int panW = 340;
    int panH = 48;
    int panX = screenW - panW - 16;
    int panY = screenH - 120;

    /* Background bar */
    DrawRectangleRounded((Rectangle){ (float)panX, (float)panY, (float)panW, (float)panH }, 0.12f, 6,
                         (Color){ 6, 12, 26, 210 });
    DrawRectangleRoundedLines((Rectangle){ (float)panX, (float)panY, (float)panW, (float)panH }, 0.12f, 6,
                              (Color){ 0, 150, 220, 160 });

    /* Section Label */
    DrawText("VECTOR RELICS", panX + 12, panY + 6, 10, (Color){ 100, 180, 240, 190 });

    /* Badge 1: [Q] Gravity Vector */
    int hasGrav = (rs->unlockedMask & (1u << VECTOR_GRAVITY)) != 0;
    int gravReady = hasGrav && (rs->gravityCooldown <= 0.0f);
    int bx1 = panX + 12;
    int by  = panY + 20;

    char qBuf[32];
    if (!hasGrav) {
        snprintf(qBuf, sizeof(qBuf), "[Q] LOCKED");
    } else if (rs->gravityActive) {
        snprintf(qBuf, sizeof(qBuf), "[Q] ACTIVE");
    } else if (rs->gravityCooldown > 0.0f) {
        snprintf(qBuf, sizeof(qBuf), "[Q] %.1fs", rs->gravityCooldown);
    } else {
        snprintf(qBuf, sizeof(qBuf), "[Q] GRAVITY");
    }

    Color qCol = !hasGrav    ? (Color){ 70, 70, 90, 160 } :
                 rs->gravityActive ? (Color){ 255, 80, 255, 255 } :
                 gravReady   ? (Color){ 200, 80, 255, 255 } : (Color){ 140, 100, 160, 180 };
    DrawText(qBuf, bx1, by, 12, qCol);

    /* Badge 2: [E] Temporal Vector */
    int hasTemp = (rs->unlockedMask & (1u << VECTOR_TEMPORAL)) != 0;
    int tempReady = hasTemp && (rs->temporalCooldown <= 0.0f);
    int bx2 = panX + 115;

    char eBuf[32];
    if (!hasTemp) {
        snprintf(eBuf, sizeof(eBuf), "[E] LOCKED");
    } else if (rs->temporalActive) {
        snprintf(eBuf, sizeof(eBuf), "[E] ACTIVE");
    } else if (rs->temporalCooldown > 0.0f) {
        snprintf(eBuf, sizeof(eBuf), "[E] %.1fs", rs->temporalCooldown);
    } else {
        snprintf(eBuf, sizeof(eBuf), "[E] TIME");
    }

    Color eCol = !hasTemp    ? (Color){ 70, 70, 90, 160 } :
                 rs->temporalActive ? (Color){ 0, 255, 200, 255 } :
                 tempReady   ? (Color){ 0, 240, 180, 255 } : (Color){ 80, 150, 130, 180 };
    DrawText(eBuf, bx2, by, 12, eCol);

    /* Badge 3: [F] Void Vector */
    int hasVoid = (rs->unlockedMask & (1u << VECTOR_VOID)) != 0;
    int voidReady = hasVoid && (rs->voidCooldown <= 0.0f);
    int bx3 = panX + 215;

    char fBuf[32];
    if (!hasVoid) {
        snprintf(fBuf, sizeof(fBuf), "[F] LOCKED");
    } else if (rs->voidActive) {
        snprintf(fBuf, sizeof(fBuf), "[F] ACTIVE");
    } else if (rs->voidCooldown > 0.0f) {
        snprintf(fBuf, sizeof(fBuf), "[F] %.1fs", rs->voidCooldown);
    } else {
        snprintf(fBuf, sizeof(fBuf), "[F] PHASE");
    }

    Color fCol = !hasVoid    ? (Color){ 70, 70, 90, 160 } :
                 rs->voidActive ? (Color){ 100, 200, 255, 255 } :
                 voidReady   ? (Color){ 80, 180, 255, 255 } : (Color){ 70, 120, 160, 180 };
    DrawText(fBuf, bx3, by, 12, fCol);
}
