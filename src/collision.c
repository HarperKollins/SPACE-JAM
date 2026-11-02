/**
 * collision.c - Collision detection across all entity types
 */
#include "collision.h"
#include "utils.h"
#include "config.h"
#include "powerup.h"
#include <math.h>

int CheckCircleCircle(Vector2 aPos, float aRad, Vector2 bPos, float bRad) {
    float dx = aPos.x - bPos.x;
    float dy = aPos.y - bPos.y;
    float dist2 = dx * dx + dy * dy;
    float radSum = aRad + bRad;
    return dist2 <= radSum * radSum;
}

int Collision_Update(
    Player        *player,
    EnemyPool     *enemies,
    AsteroidField *asteroids,
    ProjectilePool *projectiles,
    PowerupPool    *powerups,
    ParticleSystem *particles,
    GameContext    *ctx,
    AudioManager   *audio)
{
    int scoreEarned = 0;

    /* ─── Player bullets vs enemies ─────────────────── */
    for (int bi = 0; bi < MAX_PLAYER_BULLETS; bi++) {
        Projectile *b = &projectiles->player[bi];
        if (!b->active) continue;

        for (int ei = 0; ei < MAX_ENEMIES; ei++) {
            Enemy *e = &enemies->enemies[ei];
            if (!e->active || e->state == ESTATE_DYING || e->type == CIVILIAN_SHUTTLE) continue;

            if (CheckCircleCircle(b->pos, b->radius, e->pos, e->radius)) {
                Projectiles_KillPlayer(projectiles, bi);

                /* Hit effect */
                Particles_SpawnSparks(particles, b->pos, 6, b->color);

                Enemy_TakeDamage(e, b->damage);

                if (e->state == ESTATE_DYING) {
                    /* Kill confirmed */
                    int earned = e->score;
                    /* Boss score */
                    if (e->type == ENEMY_BOSS) earned = BOSS_SCORE;
                    Game_AddScore(ctx, earned);
                    scoreEarned += earned;

                    /* Explosion sound */
                    if (e->type == ENEMY_BOSS) {
                        Audio_PlaySFX(audio, SFX_BOSS_EXPLODE);
                    } else if (e->type == ENEMY_TANK) {
                        Audio_PlaySFX(audio, SFX_EXPLOSION_LARGE);
                    } else {
                        Audio_PlaySFX(audio, SFX_EXPLOSION_SMALL);
                    }

                    /* Explosion */
                    float exRad = e->radius;
                    Color exCol = (e->type == ENEMY_BOSS) ?
                                  (Color){180, 0, 255, 255} : (Color){255, 120, 0, 255};
                    Particles_SpawnExplosion(particles, e->pos, exRad,  exCol, 20 + (int)(exRad));
                    Particles_SpawnDebris(particles, e->pos, 8, exCol);

                    /* Screen shake on boss death */
                    if (e->type == ENEMY_BOSS) ctx->screenShake = SCREEN_SHAKE_MAX * 2.0f;
                    else ctx->screenShake = CLAMP(ctx->screenShake + 3.0f, 0.0f, SCREEN_SHAKE_MAX);

                    ctx->waveEnemiesKilled++;

                    /* Power-up drop */
                    if (RandChance(POWERUP_SPAWN_CHANCE)) {
                        Powerup_SpawnRandom(powerups, e->pos);
                    }
                    if (e->type == ENEMY_BOSS) {
                        /* Boss always drops max powerup */
                        Powerup_Spawn(powerups, e->pos, POWERUP_INVINCIBLE);
                    }
                }
                break; /* bullet consumed */
            }
        }
    }

    /* ─── Player bullets vs asteroids ──────────────── */
    for (int bi = 0; bi < MAX_PLAYER_BULLETS; bi++) {
        Projectile *b = &projectiles->player[bi];
        if (!b->active) continue;

        for (int ai = 0; ai < MAX_ASTEROIDS; ai++) {
            Asteroid *a = &asteroids->asteroids[ai];
            if (!a->active) continue;

            if (CheckCircleCircle(b->pos, b->radius, a->pos, a->radius)) {
                Projectiles_KillPlayer(projectiles, bi);
                Particles_SpawnSparks(particles, b->pos, 4, (Color){180, 150, 100, 255});

                /* Cache pos/color before Hit() deactivates the asteroid */
                Vector2 aPos  = a->pos;
                float   aRad  = a->radius;
                Color   aCol  = a->color;
                int earned = Asteroid_Hit(asteroids, ai, b->damage,
                                          GetScreenWidth(), GetScreenHeight());
                if (earned > 0) {
                    Game_AddScore(ctx, earned);
                    scoreEarned += earned;
                    Audio_PlaySFX(audio, SFX_EXPLOSION_SMALL);
                    Particles_SpawnExplosion(particles, aPos, aRad,
                                            (Color){160, 140, 100, 255}, 12);
                    Particles_SpawnDebris(particles, aPos, 5, aCol);
                    ctx->screenShake = CLAMP(ctx->screenShake + 1.5f, 0.0f, SCREEN_SHAKE_MAX);

                    if (RandChance(0.08f)) {
                        Powerup_SpawnRandom(powerups, a->pos);
                    }
                }
                break;
            }
        }
    }

    /* ─── Enemy bullets vs player ───────────────────── */
    if (Player_IsAlive(player) && player->iFrameTimer <= 0.0f) {
        for (int bi = 0; bi < MAX_ENEMY_BULLETS; bi++) {
            Projectile *b = &projectiles->enemy[bi];
            if (!b->active) continue;

            if (CheckCircleCircle(b->pos, b->radius, player->pos, PLAYER_RADIUS)) {
                Projectiles_KillEnemy(projectiles, bi);
                Particles_SpawnSparks(particles, b->pos, 8, (Color){255, 100, 0, 255});
                Player_TakeDamage(player, b->damage);
                Audio_PlaySFX(audio, SFX_PLAYER_HIT);
                ctx->screenShake = CLAMP(ctx->screenShake + 4.0f, 0.0f, SCREEN_SHAKE_MAX * 1.5f);
            }
        }
    }

    /* ─── Enemies vs player (deflector repulsorfields) ───────────────── */
    /* Enemy ships do NOT deal hull ramming damage. If craft get close,
       their repulsorfields gently deflect each other away */
    if (Player_IsAlive(player)) {
        for (int ei = 0; ei < MAX_ENEMIES; ei++) {
            Enemy *e = &enemies->enemies[ei];
            if (!e->active || e->state == ESTATE_DYING || e->type == CIVILIAN_SHUTTLE) continue;

            if (CheckCircleCircle(player->pos, PLAYER_RADIUS + 4.0f, e->pos, e->radius + 4.0f)) {
                Vector2 diff = Vec2Sub(e->pos, player->pos);
                float pLen = Vec2Len(diff);
                if (pLen > 0.001f) {
                    Vector2 norm = Vec2Scale(diff, 1.0f / pLen);
                    e->vel = Vec2Add(e->vel, Vec2Scale(norm, 180.0f));
                    player->vel = Vec2Sub(player->vel, Vec2Scale(norm, 90.0f));
                }
                Vector2 mid = { (player->pos.x + e->pos.x) * 0.5f, (player->pos.y + e->pos.y) * 0.5f };
                Particles_SpawnSparks(particles, mid, 4, (Color){ 0, 220, 255, 180 });
            }
        }
    }

    /* ─── Asteroids vs player ───────────────────────── */
    if (Player_IsAlive(player) && player->iFrameTimer <= 0.0f) {
        for (int ai = 0; ai < MAX_ASTEROIDS; ai++) {
            Asteroid *a = &asteroids->asteroids[ai];
            if (!a->active) continue;

            if (CheckCircleCircle(player->pos, PLAYER_RADIUS, a->pos, a->radius)) {
                Player_TakeDamage(player, 15);
                Audio_PlaySFX(audio, SFX_PLAYER_HIT);
                /* Deflect asteroid */
                Vector2 push = Vec2Norm(Vec2Sub(a->pos, player->pos));
                a->vel.x += push.x * 120.0f;
                a->vel.y += push.y * 120.0f;
                Particles_SpawnSparks(particles, player->pos, 8, (Color){200, 180, 120, 255});
                ctx->screenShake = CLAMP(ctx->screenShake + 3.5f, 0.0f, SCREEN_SHAKE_MAX * 1.5f);
            }
        }
    }

    /* ─── Powerups vs player ────────────────────────── */
    if (Player_IsAlive(player)) {
        for (int pi = 0; pi < MAX_POWERUPS; pi++) {
            Powerup *pu = &powerups->powerups[pi];
            if (!pu->active) continue;

            if (CheckCircleCircle(player->pos, PLAYER_RADIUS, pu->pos, POWERUP_RADIUS)) {
                Audio_PlaySFX(audio, SFX_POWERUP);
                Particles_SpawnPowerup(particles, pu->pos, Powerup_GetColor(pu->type));
                Player_ApplyPowerup(player, pu->type);
                Powerup_Kill(powerups, pi);
            }
        }
    }

    /* ─── Enemy weapon fire (all craft engage with ranged weapons) ─── */
    for (int ei = 0; ei < MAX_ENEMIES; ei++) {
        Enemy *e = &enemies->enemies[ei];
        if (!e->active || e->state == ESTATE_DYING) continue;

        int shouldFire = 0;
        float nextInterval = 2.0f;

        if (e->type == ENEMY_SCOUT && e->fireTimer <= 0.0f) {
            nextInterval = 3.0f + RandFloat(-0.3f, 0.4f);
            shouldFire = 1;
        } else if (e->type == ENEMY_INTERCEPTOR && e->fireTimer <= 0.0f) {
            nextInterval = 2.8f + RandFloat(-0.2f, 0.4f);
            shouldFire = 1;
        } else if (e->type == ENEMY_TANK && e->fireTimer <= 0.0f) {
            nextInterval = 3.6f + RandFloat(-0.3f, 0.5f);
            shouldFire = 1;
        } else if (e->type == ENEMY_BOMBER && e->fireTimer <= 0.0f) {
            nextInterval = 3.8f + RandFloat(-0.2f, 0.4f);
            shouldFire = 1;
        } else if (e->type == ENEMY_BOSS && e->fireTimer <= 0.0f) {
            nextInterval = 2.5f * (e->bossPhase == 2 ? 0.75f : 1.0f);
            shouldFire = 1;
        }

        if (shouldFire && Player_IsAlive(player)) {
            e->fireTimer = nextInterval;
            Vector2 dir = Vec2Norm(Vec2Sub(player->pos, e->pos));

            Audio_PlaySFX(audio, SFX_ENEMY_SHOOT);

            if (e->type == ENEMY_SCOUT) {
                /* Cyan needle pulse blaster — swift and precise */
                Projectile_SpawnEnemyEx(projectiles, e->pos, dir, 230.0f, 4.0f,
                                        8, (Color){ 0, 230, 220, 255 });
            } else if (e->type == ENEMY_INTERCEPTOR) {
                /* Twin crimson plasma cannons — staggered aim angle */
                float baseAngle = RAD2DEG(atan2f(dir.x, -dir.y));
                Vector2 d1 = AngleToDir(baseAngle - 7.0f);
                Vector2 d2 = AngleToDir(baseAngle + 7.0f);
                Projectile_SpawnEnemyEx(projectiles, e->pos, d1, 220.0f, 4.5f,
                                        9, (Color){ 255, 45, 65, 255 });
                Projectile_SpawnEnemyEx(projectiles, e->pos, d2, 220.0f, 4.5f,
                                        9, (Color){ 255, 45, 65, 255 });
            } else if (e->type == ENEMY_TANK) {
                /* Heavy hazard amber flak shell: high-caliber explosive round */
                Projectile_SpawnEnemyEx(projectiles, e->pos, dir, 190.0f, 7.0f,
                                        16, (Color){ 255, 175, 20, 255 });
            } else if (e->type == ENEMY_BOMBER) {
                /* Long-range acid-violet plasma torpedo */
                Projectile_SpawnEnemyEx(projectiles, e->pos, dir, 170.0f, 6.5f,
                                        14, (Color){ 180, 40, 255, 255 });
            } else if (e->type == ENEMY_BOSS) {
                if (e->bossPhase == 2) {
                    /* Phase 2: spread barrage (5 bullets) */
                    for (int k = -2; k <= 2; k++) {
                        float spreadAngle = RAD2DEG(atan2f(dir.x, -dir.y)) + k * 14.0f;
                        Vector2 spreadDir = AngleToDir(spreadAngle);
                        Projectile_SpawnEnemyEx(projectiles, e->pos, spreadDir, 250.0f, 5.5f,
                                                14, (Color){ 255, 30, 200, 255 });
                    }
                } else {
                    /* Phase 1: aimed triple turbolasers */
                    for (int k = -1; k <= 1; k++) {
                        float spreadAngle = RAD2DEG(atan2f(dir.x, -dir.y)) + k * 16.0f;
                        Vector2 spreadDir = AngleToDir(spreadAngle);
                        Projectile_SpawnEnemyEx(projectiles, e->pos, spreadDir, 230.0f, 5.0f,
                                                12, (Color){ 0, 230, 160, 255 });
                    }
                }
            }
        }
    }

    return scoreEarned;
}

