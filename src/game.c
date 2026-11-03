/**
 * game.c - Top-level game coordinator: state machine, game loop integration
 */
#include "game.h"
#include "world.h"
#include "player.h"
#include "enemy.h"
#include "asteroid.h"
#include "projectile.h"
#include "particles.h"
#include "powerup.h"
#include "collision.h"
#include "hud.h"
#include "menu.h"
#include "audio.h"
#include "highscore.h"
#include "settings.h"
#include "progress.h"
#include "utils.h"
#include "config.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ── Module-level game world state ──────────────────── */
static Starfield      g_stars;
static Player         g_player;
static EnemyPool      g_enemies;
static AsteroidField  g_asteroids;
static ProjectilePool g_projectiles;
static ParticleSystem g_particles;
static PowerupPool    g_powerups;
static MenuState      g_menu;
static AudioManager   g_audio;
static HighScoreTable g_scores;
static PlayerProgress g_prog;

/* Wave spawn state */
static float g_enemySpawnTimer  = 0.0f;
static float g_enemySpawnInterval = 3.5f;
static int   g_waveTotalEnemies  = 0;
static int   g_waveSpawnedCount  = 0;
static float g_shuttleSpawnTimer = 6.0f;
static float g_boostHoldTimer    = 0.0f;

/* Screen shake */
static float g_shakeX = 0.0f;
static float g_shakeY = 0.0f;

/* In-game controls hint timer (shows on first 8s of a new game, F1 toggles) */
static float g_hintTimer = 0.0f;
static int   g_hintVisible = 0;

/* ── Lifecycle ───────────────────────────────────────── */

void Game_Init(GameContext *ctx) {
    Settings_Load(&ctx->settings);

    int sw = SCREEN_WIDTH;
    int sh = SCREEN_HEIGHT;

    /* Window already open from main.c */
    SetTargetFPS(TARGET_FPS);

    Starfield_Init(&g_stars, sw, sh);
    Enemies_Init(&g_enemies);
    Asteroids_Init(&g_asteroids);
    Projectiles_Init(&g_projectiles);
    Particles_Init(&g_particles);
    Powerups_Init(&g_powerups);
    Menu_Init(&g_menu);

    HighScore_Load(&g_scores);
    Progress_Init(&g_prog);
    ctx->highScore = HighScore_GetTop(&g_scores);
    if (g_prog.highestScore > ctx->highScore) ctx->highScore = g_prog.highestScore;

    Audio_Init(&g_audio,
               ctx->settings.masterVolume,
               ctx->settings.musicVolume,
               ctx->settings.sfxVolume);
    Audio_PlayMusicTrack(&g_audio, MUSIC_TRACK_INTRO);

    ctx->state     = STATE_MAIN_MENU;
    ctx->prevState = STATE_MAIN_MENU;
    ctx->running   = 1;
    ctx->score     = 0;
    ctx->wave      = 0;
}

void Game_Shutdown(GameContext *ctx) {
    HighScore_Insert(&g_scores, ctx->score, ctx->wave, "ACE");
    HighScore_Save(&g_scores);
    Settings_Save(&ctx->settings);
    Menu_Shutdown(&g_menu);
    Audio_Shutdown(&g_audio);
}

void Game_ChangeState(GameContext *ctx, GameState next) {
    ctx->prevState = ctx->state;
    ctx->state     = next;
    g_menu.selectedItem = 0;

    if (next == STATE_MAIN_MENU || next == STATE_PROLOGUE ||
        next == STATE_COMMS     || next == STATE_SETTINGS  ||
        next == STATE_CONTROLS  || next == STATE_MEDALS) {
        Audio_PlayMusicTrack(&g_audio, MUSIC_TRACK_INTRO);
    } else if (next == STATE_PLAYING || next == STATE_PAUSED) {
        Audio_PlayMusicTrack(&g_audio, MUSIC_TRACK_BATTLE);
    } else if (next == STATE_GAME_OVER) {
        Audio_StopMusic(&g_audio);
        Audio_PlaySFX(&g_audio, SFX_GAME_OVER);
    }
}

void Game_PlaySFX(int sfxId) {
    Audio_PlaySFX(&g_audio, (SfxId)sfxId);
}

void Game_AddScore(GameContext *ctx, int points) {
    ctx->score += points;
    if (ctx->score > ctx->highScore) ctx->highScore = ctx->score;
}

void Game_StartNewGame(GameContext *ctx) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    Player_Init(&g_player, sw, sh);
    Enemies_Clear(&g_enemies);
    Asteroids_Clear(&g_asteroids);
    Particles_Clear(&g_particles);
    Powerups_Clear(&g_powerups);
    Projectiles_Init(&g_projectiles);

    ctx->score              = 0;
    ctx->wave               = 0;
    ctx->waveEnemiesKilled  = 0;
    ctx->waveEnemiesRequired = 0;
    ctx->waveClearTimer     = 0.0f;
    ctx->bossActive         = 0;
    ctx->screenShake        = 0.0f;

    g_enemySpawnTimer    = 0.0f;
    g_enemySpawnInterval = 3.5f;
    g_waveTotalEnemies   = 0;
    g_waveSpawnedCount   = 0;
    g_shuttleSpawnTimer  = 6.0f;
    g_boostHoldTimer     = 0.0f;

    /* Controls hint overlay */
    g_hintTimer   = 8.0f;
    g_hintVisible = 1;

    Asteroids_SetSpawnRate(&g_asteroids, 1);
    Game_NextWave(ctx);
}

void Game_NextWave(GameContext *ctx) {
    ctx->wave++;
    ctx->waveEnemiesKilled  = 0;

    int sectorIdx = (ctx->wave <= 3) ? 0 : (ctx->wave <= 6) ? 1 : (ctx->wave <= 9) ? 2 : 3;
    WorldSector newSector = (WorldSector)sectorIdx;

    /* Hyperspace warp jump transition upon entering a new sector */
    if (g_stars.sector != newSector || ctx->wave == 1) {
        Starfield_SetSector(&g_stars, newSector, 1);
        Audio_PlaySFX(&g_audio, SFX_WARP_JUMP);
    }

    /* Ship Evolution tier based on sector reached */
    Player_SetShipTier(&g_player, (ShipTier)sectorIdx);

    /* Update career progress record */
    if (ctx->wave > g_prog.maxWaveReached) g_prog.maxWaveReached = ctx->wave;
    if (sectorIdx + 1 > g_prog.maxSectorReached) g_prog.maxSectorReached = sectorIdx + 1;
    Progress_Save(&g_prog);

    /* Scale difficulty */
    int baseCount = 5 + (ctx->wave - 1) * 3;
    g_waveTotalEnemies     = baseCount;
    g_waveSpawnedCount     = 0;
    ctx->waveEnemiesRequired = baseCount;
    g_enemySpawnInterval   = MAX(0.8f, 3.2f - (float)(ctx->wave - 1) * 0.18f);
    g_enemySpawnTimer      = 0.5f;

    /* Climax Boss encounter every 3 waves (Sector Bosses!) */
    ctx->bossActive = (ctx->wave % 3 == 0);
    if (ctx->bossActive) {
        Enemy_SpawnSectorBoss(&g_enemies, sectorIdx, GetScreenWidth(), GetScreenHeight());
        Audio_PlaySFX(&g_audio, SFX_BOSS_APPEAR);
        g_waveTotalEnemies   += 1;
        ctx->waveEnemiesRequired += 1;
    }

    Asteroids_SetSpawnRate(&g_asteroids, ctx->wave);
}

/* ── Wave enemy type picker ─────────────────────────── */

static EnemyType PickEnemyType(int wave) {
    int r = GetRandomValue(0, 99);
    if (wave <= 2) {
        /* First waves: scouts only */
        return ENEMY_SCOUT;
    } else if (wave <= 4) {
        if (r < 60) return ENEMY_SCOUT;
        return ENEMY_INTERCEPTOR;
    } else if (wave <= 7) {
        if (r < 40) return ENEMY_SCOUT;
        if (r < 70) return ENEMY_INTERCEPTOR;
        if (r < 85) return ENEMY_BOMBER;
        return ENEMY_TANK;
    } else {
        if (r < 25) return ENEMY_SCOUT;
        if (r < 50) return ENEMY_INTERCEPTOR;
        if (r < 68) return ENEMY_BOMBER;
        return ENEMY_TANK;
    }
}

/* ── Screen shake ────────────────────────────────────── */

static void UpdateScreenShake(GameContext *ctx, float dt) {
    if (!ctx->settings.screenShake) { ctx->screenShake = 0.0f; return; }
    ctx->screenShake = MAX(0.0f, ctx->screenShake - SCREEN_SHAKE_DECAY * dt);
    float mag = ctx->screenShake;
    g_shakeX = RandFloat(-mag, mag);
    g_shakeY = RandFloat(-mag, mag);
}

/* ── Update ──────────────────────────────────────────── */

void Game_Update(GameContext *ctx, float dt) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    Starfield_Update(&g_stars, dt, sw, sh);

    /* Ensure correct music track is active */
    if (ctx->state == STATE_MAIN_MENU || ctx->state == STATE_PROLOGUE ||
        ctx->state == STATE_SETTINGS  || ctx->state == STATE_CONTROLS) {
        Audio_PlayMusicTrack(&g_audio, MUSIC_TRACK_INTRO);
    } else if (ctx->state == STATE_PLAYING || ctx->state == STATE_PAUSED) {
        Audio_PlayMusicTrack(&g_audio, MUSIC_TRACK_BATTLE);
    }

    Audio_UpdateMusic(&g_audio, dt);

    switch (ctx->state) {
        /* ─ Main Menu ─ */
        case STATE_MAIN_MENU:
            Menu_UpdateMain(&g_menu, ctx, dt);
            break;

        /* ─ Story Prologue Crawl ─ */
        case STATE_PROLOGUE:
            Menu_UpdatePrologue(&g_menu, ctx, dt);
            break;

        /* ─ Tactical Comms Cutscene ─ */
        case STATE_COMMS:
            Menu_UpdateComms(&g_menu, ctx, dt);
            break;

        /* ─ Service Record / Medals ─ */
        case STATE_MEDALS:
            Menu_UpdateMedals(&g_menu, ctx, dt);
            break;

        /* ─ Playing ─ */
        case STATE_PLAYING: {
            /* Pause (ESC or click HUD button) */
            Rectangle pauseBtn = { (float)(sw - 152), 32.0f, 144.0f, 24.0f };
            Rectangle hintBtn  = { (float)(sw - 252), 32.0f,  96.0f, 24.0f };
            if (IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), pauseBtn))) {
                Game_ChangeState(ctx, STATE_PAUSED);
                break;
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), hintBtn)) {
                g_hintVisible = !g_hintVisible;
                if (g_hintVisible) g_hintTimer = 8.0f;
            }

            /* F1: toggle controls hint overlay */
            if (IsKeyPressed(KEY_F1)) {
                g_hintVisible = !g_hintVisible;
                if (g_hintVisible) g_hintTimer = 8.0f;
            }
            if (g_hintVisible && g_hintTimer > 0.0f) {
                g_hintTimer -= dt;
                if (g_hintTimer <= 0.0f) g_hintVisible = 0;
            }

            UpdateScreenShake(ctx, dt);

            /* Player update */
            Player_Update(&g_player, dt, sw, sh);

            /* Shoot */
            if (Player_TryShoot(&g_player, dt)) {
                float rotRad = DEG2RAD(g_player.rotation);
                float cosR = cosf(rotRad);
                float sinR = sinf(rotRad);
                Vector2 dir = AngleToDir(g_player.rotation);

                /* Wingtip heavy autocannon hardpoints matching HK-01 concept art */
                Vector2 gunL = { g_player.pos.x + (-19.0f)*cosR - (0.0f)*sinR,
                                 g_player.pos.y + (-19.0f)*sinR + (0.0f)*cosR };
                Vector2 gunR = { g_player.pos.x + ( 19.0f)*cosR - (0.0f)*sinR,
                                 g_player.pos.y + ( 19.0f)*sinR + (0.0f)*cosR };
                Vector2 tip  = { g_player.pos.x + dir.x * 22.0f,
                                 g_player.pos.y + dir.y * 22.0f };

                if (g_player.weapon == WEAPON_DOUBLE) {
                    /* Quad Cannons: Twin Wingtips + Twin Fuselage Sponsons */
                    float perpX = -dir.y, perpY = dir.x;
                    Vector2 midL = { tip.x - perpX * 6.0f, tip.y - perpY * 6.0f };
                    Vector2 midR = { tip.x + perpX * 6.0f, tip.y + perpY * 6.0f };
                    Projectile_SpawnPlayer(&g_projectiles, gunL, dir, BULLET_DAMAGE);
                    Projectile_SpawnPlayer(&g_projectiles, gunR, dir, BULLET_DAMAGE);
                    Projectile_SpawnPlayer(&g_projectiles, midL, dir, BULLET_DAMAGE);
                    Projectile_SpawnPlayer(&g_projectiles, midR, dir, BULLET_DAMAGE);
                    Particles_SpawnSparks(&g_particles, gunL, 3, (Color){ 0, 240, 255, 255 });
                    Particles_SpawnSparks(&g_particles, gunR, 3, (Color){ 0, 240, 255, 255 });
                } else if (g_player.weapon == WEAPON_SPREAD) {
                    /* 5-Way Starburst Array */
                    Vector2 leftDir  = AngleToDir(g_player.rotation - 18.0f);
                    Vector2 rightDir = AngleToDir(g_player.rotation + 18.0f);
                    Vector2 wideLDir = AngleToDir(g_player.rotation - 32.0f);
                    Vector2 wideRDir = AngleToDir(g_player.rotation + 32.0f);
                    Projectile_SpawnPlayer(&g_projectiles, tip,  dir,      BULLET_DAMAGE);
                    Projectile_SpawnPlayer(&g_projectiles, gunL, leftDir,  BULLET_DAMAGE);
                    Projectile_SpawnPlayer(&g_projectiles, gunR, rightDir, BULLET_DAMAGE);
                    Projectile_SpawnPlayer(&g_projectiles, gunL, wideLDir, BULLET_DAMAGE);
                    Projectile_SpawnPlayer(&g_projectiles, gunR, wideRDir, BULLET_DAMAGE);
                    Particles_SpawnSparks(&g_particles, tip,  4, (Color){ 255, 220,  0, 255 });
                    Particles_SpawnSparks(&g_particles, gunL, 3, (Color){ 255, 220,  0, 255 });
                    Particles_SpawnSparks(&g_particles, gunR, 3, (Color){ 255, 220,  0, 255 });
                } else {
                    /* WEAPON_SINGLE (Default): Twin Synchronized Heavy Wingtip Autocannons */
                    Projectile_SpawnPlayer(&g_projectiles, gunL, dir, BULLET_DAMAGE);
                    Projectile_SpawnPlayer(&g_projectiles, gunR, dir, BULLET_DAMAGE);
                    Particles_SpawnSparks(&g_particles, gunL, 2, (Color){ 0, 220, 255, 255 });
                    Particles_SpawnSparks(&g_particles, gunR, 2, (Color){ 0, 220, 255, 255 });
                }
                Audio_PlaySFX(&g_audio, SFX_SHOOT);
            }

            /* Exhaust & Nitro Particles (Flexing live at all times) */
            {
                Vector2 fwd = AngleToDir(g_player.rotation);
                Vector2 exhaustPos = { g_player.pos.x - fwd.x * 14.0f,
                                       g_player.pos.y - fwd.y * 14.0f };

                if (g_player.isBoosting) {
                    g_boostHoldTimer += dt;
                    if (g_boostHoldTimer >= 10.0f) {
                        Progress_UnlockMedal(&g_prog, MEDAL_HYPER_NITRO);
                    }
                    /* Hyper-Nitro Boost: Intense double exhaust plume & nitro sparks */
                    for (int i = 0; i < EXHAUST_PARTICLES_PER_FRAME * 2; i++) {
                        Particles_SpawnExhaust(&g_particles, exhaustPos, fwd,
                                               (Color){ 0, 230, 255, 230 });
                    }
                    if (RandChance(0.40f)) {
                        Particles_SpawnSparks(&g_particles, exhaustPos, 2, (Color){ 255, 170, 30, 255 });
                    }
                    /* Subtle cockpit throttle rumble */
                    ctx->screenShake = CLAMP(ctx->screenShake + 0.18f, 0.0f, 1.8f);
                } else {
                    g_boostHoldTimer = 0.0f;
                    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
                        /* Standard forward thrust */
                        for (int i = 0; i < EXHAUST_PARTICLES_PER_FRAME; i++) {
                            Particles_SpawnExhaust(&g_particles, exhaustPos, fwd,
                                                   (Color){ 40, 160, 255, 200 });
                        }
                    } else {
                        /* Idle engine flex: occasional trailing ion shimmer */
                        if (RandChance(0.20f)) {
                            Particles_SpawnExhaust(&g_particles, exhaustPos, fwd,
                                                   (Color){ 0, 180, 255, 110 });
                        }
                    }
                }
            }

            /* Sector 1: Civilian Refugee Shuttle Evacuation Spawning */
            if (g_stars.sector == SECTOR_TERRA_NOVUS && !ctx->bossActive && ctx->wave <= 3) {
                g_shuttleSpawnTimer -= dt;
                if (g_shuttleSpawnTimer <= 0.0f) {
                    g_shuttleSpawnTimer = 12.0f;
                    Enemy_SpawnShuttle(&g_enemies, sw, sh);
                }
            }

            /* Wave enemy spawning */
            if (g_waveSpawnedCount < g_waveTotalEnemies && !ctx->bossActive) {
                g_enemySpawnTimer -= dt;
                if (g_enemySpawnTimer <= 0.0f) {
                    g_enemySpawnTimer = g_enemySpawnInterval;
                    EnemyType type = PickEnemyType(ctx->wave);
                    if (Enemy_Spawn(&g_enemies, type, sw, sh)) {
                        g_waveSpawnedCount++;
                    }
                }
            }

            /* Enemy update */
            Enemies_Update(&g_enemies, g_player.pos, dt, sw, sh);

            /* Check civilian shuttle rescue escape */
            for (int ei = 0; ei < MAX_ENEMIES; ei++) {
                Enemy *e = &g_enemies.enemies[ei];
                if (e->active && e->state != ESTATE_DYING && e->type == CIVILIAN_SHUTTLE) {
                    if (e->pos.x > (float)sw + 30.0f) {
                        e->active = 0;
                        g_enemies.count--;
                        g_prog.totalShuttlesSaved++;
                        Game_AddScore(ctx, 500);
                        Particles_SpawnSparks(&g_particles, (Vector2){ (float)(sw - 20), e->pos.y }, 16, (Color){ 0, 255, 200, 255 });
                        Audio_PlaySFX(&g_audio, SFX_MEDAL_AWARD);
                        if (g_prog.totalShuttlesSaved >= 5) {
                            Progress_UnlockMedal(&g_prog, MEDAL_SAVIOR_OF_TERRA);
                        }
                        Progress_Save(&g_prog);
                    }
                }
            }

            /* Asteroid update */
            Asteroids_Update(&g_asteroids, dt, sw, sh, ctx->wave);

            /* Projectile update */
            Projectiles_Update(&g_projectiles, dt, sw, sh);

            /* Particle update */
            Particles_Update(&g_particles, dt);

            /* Powerup update */
            Powerups_Update(&g_powerups, dt);

            /* Track kill count for medals */
            int killsBefore = ctx->waveEnemiesKilled;

            /* Collision detection */
            Collision_Update(&g_player, &g_enemies, &g_asteroids,
                             &g_projectiles, &g_powerups, &g_particles, ctx, &g_audio);

            if (ctx->waveEnemiesKilled > killsBefore) {
                int deltaKills = ctx->waveEnemiesKilled - killsBefore;
                g_prog.totalKills += deltaKills;
            }

            /* Score & Boost medals */
            if (ctx->score >= 10000) {
                Progress_UnlockMedal(&g_prog, MEDAL_DEADEYE);
            }
            if (g_prog.awardPopupTimer > 0.0f) {
                g_prog.awardPopupTimer -= dt;
            }

            /* Check wave completion */
            if (ctx->waveEnemiesKilled >= ctx->waveEnemiesRequired
                && g_enemies.count == 0 && !ctx->bossActive) {
                ctx->waveClearTimer += dt;
                if (ctx->waveClearTimer >= WAVE_CLEAR_PAUSE) {
                    ctx->waveClearTimer = 0.0f;
                    Game_NextWave(ctx);
                }
            }
            /* Boss check */
            if (ctx->bossActive && Enemies_FindBoss(&g_enemies) < 0) {
                ctx->bossActive = 0;
                /* Trigger wave clear timer so game advances past boss */
                ctx->waveEnemiesKilled = ctx->waveEnemiesRequired;
                ctx->waveClearTimer = 0.0f;

                g_prog.totalBossesDefeated++;
                if (ctx->wave == 3) {
                    Progress_UnlockMedal(&g_prog, MEDAL_SAVIOR_OF_TERRA);
                } else if (ctx->wave == 6) {
                    Progress_UnlockMedal(&g_prog, MEDAL_SOLAR_ACE);
                } else if (ctx->wave == 9) {
                    Progress_UnlockMedal(&g_prog, MEDAL_ICE_MARKSMAN);
                } else if (ctx->wave >= 12) {
                    Progress_UnlockMedal(&g_prog, MEDAL_CITADEL_SLAYER);
                }
                if (g_player.health < 25) {
                    Progress_UnlockMedal(&g_prog, MEDAL_IRON_WILL);
                }
                Audio_PlaySFX(&g_audio, SFX_MEDAL_AWARD);
                Progress_Save(&g_prog);
            }

            /* Player death */
            if (!Player_IsAlive(&g_player)) {
                /* Spawn death explosion once upon entering dead state */
                if (g_player.deathTimer == 0.0f) {
                    Particles_SpawnExplosion(&g_particles, g_player.pos, 32.0f,
                                             (Color){0, 150, 255, 255}, 30);
                    Audio_PlaySFX(&g_audio, SFX_EXPLOSION_LARGE);
                }
                /* Wait for death animation then go to game over */
                if (g_player.deathTimer > 1.5f) {
                    HighScore_Insert(&g_scores, ctx->score, ctx->wave, "ACE");
                    HighScore_Save(&g_scores);
                    ctx->highScore = HighScore_GetTop(&g_scores);
                    if (ctx->score > g_prog.highestScore) g_prog.highestScore = ctx->score;
                    if (ctx->wave > g_prog.maxWaveReached) g_prog.maxWaveReached = ctx->wave;
                    if ((int)g_stars.sector + 1 > g_prog.maxSectorReached) g_prog.maxSectorReached = (int)g_stars.sector + 1;
                    Progress_Save(&g_prog);
                    Audio_PlaySFX(&g_audio, SFX_GAME_OVER);
                    Game_ChangeState(ctx, STATE_GAME_OVER);
                }
            }
            break;
        }

        /* ─ Paused ─ */
        case STATE_PAUSED:
            Menu_UpdatePause(&g_menu, ctx, dt);
            break;

        /* ─ Game Over ─ */
        case STATE_GAME_OVER:
            Menu_UpdateGameOver(&g_menu, ctx, dt);
            break;

        /* ─ Settings ─ */
        case STATE_SETTINGS:
            Menu_UpdateSettings(&g_menu, ctx, dt);
            Audio_SetVolumes(&g_audio,
                             ctx->settings.masterVolume,
                             ctx->settings.musicVolume,
                             ctx->settings.sfxVolume);
            break;

        case STATE_CONTROLS:
            Menu_UpdateControls(&g_menu, ctx, dt);
            break;

        case STATE_VICTORY:
            /* Placeholder for future victory state */
            if (IsKeyPressed(KEY_ENTER)) Game_ChangeState(ctx, STATE_MAIN_MENU);
            break;

        default: break;
    }
}

/* ── Draw ────────────────────────────────────────────── */

void Game_Draw(GameContext *ctx) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    /* Apply screen shake offset */
    float ox = (ctx->settings.screenShake) ? g_shakeX : 0.0f;
    float oy = (ctx->settings.screenShake) ? g_shakeY : 0.0f;

    BeginDrawing();

    /* Deep space background */
    ClearBackground((Color){ 2, 2, 12, 255 });

    /* Slight vignette effect */
    for (int i = 0; i < 4; i++) {
        int edge = i * 40;
        DrawRectangleLinesEx(
            (Rectangle){ (float)edge, (float)edge,
                         (float)(sw - 2 * edge), (float)(sh - 2 * edge) },
            (float)(4 - i),
            (Color){ 0, 0, 20, (unsigned char)(60 - i * 12) }
        );
    }

    if (ctx->state == STATE_PLAYING || ctx->state == STATE_PAUSED) {
        /* Push shake offset */
        BeginMode2D((Camera2D){ .offset = {ox, oy}, .target = {0,0},
                                .rotation = 0, .zoom = 1.0f });
        Starfield_Draw(&g_stars);
        Asteroids_Draw(&g_asteroids);
        Powerups_Draw(&g_powerups);
        Enemies_Draw(&g_enemies);
        Projectiles_Draw(&g_projectiles);
        Particles_Draw(&g_particles);
        if (Player_IsAlive(&g_player)) Player_Draw(&g_player);
        EndMode2D();

        HUD_Draw(ctx, &g_player, &g_enemies, sw, sh);
        HUD_DrawMedalPopup(g_prog.newlyUnlocked, g_prog.awardPopupTimer, sw);

        if (ctx->state == STATE_PAUSED) {
            Menu_DrawPause(&g_menu, sw, sh);
        }

        /* Sector & Wave announcement banner + Tactical Comms chatter */
        static float waveAnnounceTimer = 0.0f;
        static int   lastAnnouncedWave = 0;
        if (lastAnnouncedWave != ctx->wave) {
            lastAnnouncedWave = ctx->wave;
            waveAnnounceTimer = 3.8f;
        }
        if (waveAnnounceTimer > 0.0f) {
            waveAnnounceTimer -= GetFrameTime();
            float alpha = CLAMP(waveAnnounceTimer * 1.2f, 0.0f, 1.0f);
            unsigned char a = (unsigned char)(alpha * 255);

            int bossWave = (ctx->wave % 3 == 0);
            const char *sectorName = Starfield_GetSectorName(g_stars.sector);
            const char *sectorSub  = Starfield_GetSectorSubtitle(g_stars.sector);

            /* Tactical Comms Message */
            const char *commsMsg =
                (ctx->wave == 1) ? "[FLEET HQ]: Pilot Harper! Iron Synod fleet has breached Terra Novus! Escort refugee shuttles!" :
                (ctx->wave == 2) ? "[FLEET HQ]: Good shooting, Ace! More civilian transport ships launching from burning cities!" :
                (ctx->wave == 3) ? "[EMERGENCY]: Synod Cruiser 'Terror of Terra' detected! Destroy Commander Varek's ship!" :
                (ctx->wave == 4) ? "[FLEET HQ]: Sector 2: The Solar Corona. Blazing sun flares masking Synod strike wings!" :
                (ctx->wave == 5) ? "[FLEET HQ]: Solar radiation critical! Collect weapon power-ups and punch through!" :
                (ctx->wave == 6) ? "[ALERT]: Inquisitor Malakor's Solar Siege Platform engaging! Focus fire on reactor!" :
                (ctx->wave == 7) ? "[FLEET HQ]: Sector 3: Ice Rings of Xylos. Emerald gas giant ahead. Watch the crystal fields!" :
                (ctx->wave == 8) ? "[FLEET HQ]: Enemy stealth corvettes moving through the rings. Keep active scans up!" :
                (ctx->wave == 9) ? "[ALERT]: High Admiral Zephyr's flagship 'Frostbite' engaging! Break through the blockade!" :
                (ctx->wave == 10) ? "[FLEET HQ]: Sector 4: Synod Citadel! We are at the heart of the Imperial stronghold!" :
                (ctx->wave == 11) ? "[FLEET HQ]: Heavy fortress cannons online. All Colony wings support Vanguard-1!" :
                (ctx->wave == 12) ? "[FINAL MISSION]: The Iron Archon has engaged! Destroy the Imperial Citadel Flagship!" :
                                   "[FLEET HQ]: Citadel defenses neutralized! Victory for the Free Colonies!";

            /* 1. Sector & Wave Center Banner */
            int stw = MeasureText(sectorName, 18);
            Color secCol = bossWave ? (Color){ 255, 60, 60, a } : (Color){ 0, 220, 255, a };
            DrawText(sectorName, sw / 2 - stw / 2, sh / 2 - 82, 18, secCol);

            int subw = MeasureText(sectorSub, 13);
            DrawText(sectorSub, sw / 2 - subw / 2, sh / 2 - 60, 13, (Color){ 180, 210, 240, a });

            char wbuf[64];
            if (bossWave) {
                int sector = (ctx->wave <= 3) ? 0 : (ctx->wave <= 6) ? 1 : (ctx->wave <= 9) ? 2 : 3;
                snprintf(wbuf, sizeof(wbuf), "!! BOSS: %s !!", Enemy_GetBossName(sector));
            } else {
                snprintf(wbuf, sizeof(wbuf), "WAVE %d", ctx->wave);
            }
            int tw = MeasureText(wbuf, bossWave ? 24 : 44);
            Color wc = bossWave ? (Color){ 255, 80, 80, a } : (Color){ 255, 215, 0, a };
            DrawText(wbuf, sw / 2 - tw / 2, sh / 2 - 38, bossWave ? 24 : 44, wc);

            /* 2. Tactical Comms Box at Bottom */
            int cboxW = MIN(760, sw - 80);
            int cboxH = 52;
            int cboxX = sw / 2 - cboxW / 2;
            int cboxY = sh - 100;
            DrawRectangleRounded((Rectangle){ (float)cboxX, (float)cboxY, (float)cboxW, (float)cboxH },
                                 0.15f, 6, (Color){ 4, 8, 20, (unsigned char)(alpha * 215) });
            DrawRectangleRoundedLines((Rectangle){ (float)cboxX, (float)cboxY, (float)cboxW, (float)cboxH },
                                      0.15f, 6, (Color){ 0, 180, 255, (unsigned char)(alpha * 170) });
            DrawText("TACTICAL COMMS // FREQ 104.8 MHZ", cboxX + 16, cboxY + 8, 11,
                     (Color){ 0, 255, 180, a });
            DrawText(commsMsg, cboxX + 16, cboxY + 24, 14,
                     (Color){ 230, 240, 255, a });
        }

        /* In-game controls hint overlay (fades out after 8s, F1 to toggle) */
        if (g_hintVisible && g_hintTimer > 0.0f) {
            float alpha = CLAMP(g_hintTimer * 0.8f, 0.0f, 1.0f);
            unsigned char a = (unsigned char)(alpha * 200);
            unsigned char ta = (unsigned char)(alpha * 255);
            int hx = sw - 230, hy = 75;
            DrawRectangleRounded((Rectangle){(float)hx - 10, (float)hy - 8, 220, 180},
                                 0.1f, 6, (Color){6, 6, 24, a});
            DrawRectangleRoundedLines((Rectangle){(float)hx - 10, (float)hy - 8, 220, 180},
                                      0.1f, 6, (Color){0, 150, 255, (unsigned char)(a / 2)});
            DrawText("MISSION CONTROLS", hx, hy,      11, (Color){255, 200, 0, ta});
            DrawText("W / UP     Thrust / Nitro",  hx, hy + 18, 12, (Color){0, 220, 255, ta});
            DrawText("SHIFT/RMB  Hyper-Boost",      hx, hy + 36, 12, (Color){0, 255, 180, ta});
            DrawText("S / DOWN   Brake",           hx, hy + 54, 12, (Color){0, 220, 255, ta});
            DrawText("A / D      Rotate / Bank",   hx, hy + 72, 12, (Color){0, 220, 255, ta});
            DrawText("SPACE/LMB  Twin Cannons",    hx, hy + 90, 12, (Color){0, 220, 255, ta});
            DrawText("ESC        Pause",           hx, hy + 108, 12, (Color){0, 220, 255, ta});
            DrawText("F1         Toggle Hint",     hx, hy + 126, 12, (Color){160, 160, 200, ta});
            DrawText("Press F1 to hide",           hx, hy + 150, 11, (Color){120, 120, 160, (unsigned char)(ta / 2)});
        }

    } else {
        /* Non-gameplay states draw starfield as decoration */
        Starfield_Draw(&g_stars);
    }

    switch (ctx->state) {
        case STATE_MAIN_MENU:
            Menu_DrawMain(&g_menu, ctx, sw, sh); break;
        case STATE_PROLOGUE:
            Menu_DrawPrologue(&g_menu, sw, sh); break;
        case STATE_COMMS:
            Menu_DrawComms(&g_menu, sw, sh); break;
        case STATE_MEDALS:
            Menu_DrawMedals(&g_menu, &g_prog, sw, sh); break;
        case STATE_GAME_OVER:
            Menu_DrawGameOver(&g_menu, ctx, sw, sh); break;
        case STATE_SETTINGS:
            Menu_DrawSettings(&g_menu, ctx, sw, sh); break;
        case STATE_CONTROLS:
            Menu_DrawControls(&g_menu, sw, sh); break;
        case STATE_VICTORY: {
            const char *vt = "YOU WIN!";
            int tw = MeasureText(vt, 60);
            DrawText(vt, sw / 2 - tw / 2, sh / 2 - 60, 60, GOLD);
            DrawText("Press ENTER to continue", sw / 2 - 130, sh / 2 + 20, 22, LIGHTGRAY);
            break;
        }
        default: break;
    }

    /* FPS counter (top right corner, small) */
    DrawFPS(sw - 80, 6);

    EndDrawing();
}

