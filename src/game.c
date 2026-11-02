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

/* Wave spawn state */
static float g_enemySpawnTimer  = 0.0f;
static float g_enemySpawnInterval = 3.5f;
static int   g_waveTotalEnemies  = 0;
static int   g_waveSpawnedCount  = 0;

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
    ctx->highScore = HighScore_GetTop(&g_scores);

    Audio_Init(&g_audio,
               ctx->settings.masterVolume,
               ctx->settings.musicVolume,
               ctx->settings.sfxVolume);

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
    Audio_Shutdown(&g_audio);
}

void Game_ChangeState(GameContext *ctx, GameState next) {
    ctx->prevState = ctx->state;
    ctx->state     = next;
    g_menu.selectedItem = 0;
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

    /* Controls hint overlay */
    g_hintTimer   = 8.0f;
    g_hintVisible = 1;

    Asteroids_SetSpawnRate(&g_asteroids, 1);
    Game_NextWave(ctx);
}

void Game_NextWave(GameContext *ctx) {
    ctx->wave++;
    ctx->waveEnemiesKilled  = 0;

    /* Scale difficulty */
    int baseCount = 5 + (ctx->wave - 1) * 3;
    g_waveTotalEnemies     = baseCount;
    g_waveSpawnedCount     = 0;
    ctx->waveEnemiesRequired = baseCount;
    g_enemySpawnInterval   = MAX(0.8f, 3.5f - (float)(ctx->wave - 1) * 0.2f);
    g_enemySpawnTimer      = 0.5f;

    ctx->bossActive = (ctx->wave % BOSS_WAVE_INTERVAL == 0);
    if (ctx->bossActive) {
        Enemy_SpawnBoss(&g_enemies, GetScreenWidth(), GetScreenHeight());
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
                Vector2 dir = AngleToDir(g_player.rotation);
                /* Bullet origin slightly in front of ship */
                Vector2 tip = { g_player.pos.x + dir.x * 22.0f,
                                g_player.pos.y + dir.y * 22.0f };

                if (g_player.weapon == WEAPON_DOUBLE) {
                    /* Two bullets offset perpendicular */
                    float perpX = -dir.y, perpY = dir.x;
                    Vector2 l = { tip.x - perpX * 7.0f, tip.y - perpY * 7.0f };
                    Vector2 r = { tip.x + perpX * 7.0f, tip.y + perpY * 7.0f };
                    Projectile_SpawnPlayer(&g_projectiles, l, dir, BULLET_DAMAGE);
                    Projectile_SpawnPlayer(&g_projectiles, r, dir, BULLET_DAMAGE);
                } else if (g_player.weapon == WEAPON_SPREAD) {
                    /* Three-way spread shot */
                    Projectile_SpawnPlayer(&g_projectiles, tip, dir, BULLET_DAMAGE);
                    Vector2 leftDir  = AngleToDir(g_player.rotation - 15.0f);
                    Vector2 rightDir = AngleToDir(g_player.rotation + 15.0f);
                    Projectile_SpawnPlayer(&g_projectiles, tip, leftDir, BULLET_DAMAGE);
                    Projectile_SpawnPlayer(&g_projectiles, tip, rightDir, BULLET_DAMAGE);
                } else {
                    Projectile_SpawnPlayer(&g_projectiles, tip, dir, BULLET_DAMAGE);
                }
                Audio_PlaySFX(&g_audio, SFX_SHOOT);
            }

            /* Exhaust particles while thrusting */
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
                Vector2 fwd = AngleToDir(g_player.rotation);
                Vector2 exhaustPos = { g_player.pos.x - fwd.x * 14.0f,
                                       g_player.pos.y - fwd.y * 14.0f };
                for (int i = 0; i < EXHAUST_PARTICLES_PER_FRAME; i++) {
                    Particles_SpawnExhaust(&g_particles, exhaustPos, fwd,
                                           (Color){60, 140, 255, 180});
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

            /* Asteroid update */
            Asteroids_Update(&g_asteroids, dt, sw, sh, ctx->wave);

            /* Projectile update */
            Projectiles_Update(&g_projectiles, dt, sw, sh);

            /* Particle update */
            Particles_Update(&g_particles, dt);

            /* Powerup update */
            Powerups_Update(&g_powerups, dt);

            /* Collision detection */
            Collision_Update(&g_player, &g_enemies, &g_asteroids,
                             &g_projectiles, &g_powerups, &g_particles, ctx, &g_audio);

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

        if (ctx->state == STATE_PAUSED) {
            Menu_DrawPause(&g_menu, sw, sh);
        }

        /* Sector & Wave announcement banner + Tactical Comms chatter */
        static float waveAnnounceTimer = 0.0f;
        static int   lastAnnouncedWave = 0;
        if (lastAnnouncedWave != ctx->wave) {
            lastAnnouncedWave = ctx->wave;
            waveAnnounceTimer = 3.6f;
        }
        if (waveAnnounceTimer > 0.0f) {
            waveAnnounceTimer -= GetFrameTime();
            float alpha = CLAMP(waveAnnounceTimer * 1.2f, 0.0f, 1.0f);
            unsigned char a = (unsigned char)(alpha * 255);

            int bossWave = (ctx->wave % BOSS_WAVE_INTERVAL == 0);
            const char *sectorName = (ctx->wave <= 4) ? "SECTOR 1 // THE SHROUDED FRONTIER" :
                                     (ctx->wave == 5) ? "FLEET ALERT // DREADNOUGHT INTERCEPTION" :
                                     (ctx->wave <= 9) ? "SECTOR 2 // THE ORBITAL GRAVEYARD" :
                                                        "SECTOR 3 // SYNOD CITADEL REACH";

            /* Tactical Comms Message */
            const char *commsMsg =
                (ctx->wave == 1) ? "[FLEET HQ]: Pilot Kollins, Synod Dagger interceptors detected. Clear the airspace, Ace!" :
                (ctx->wave == 2) ? "[FLEET HQ]: Good kills. High-speed strike echelon detected on radar." :
                (ctx->wave == 3) ? "[FLEET HQ]: Watch your flanks! Synod Jackal heavy fighters closing in." :
                (ctx->wave == 4) ? "[FLEET HQ]: Ordnance alert! Avoid Goliath plasma torpedo barrages." :
                (ctx->wave == 5) ? "[EMERGENCY]: Synod Dreadnought 'Vindicator' dropping out of warp! Target the bridge!" :
                (ctx->wave == 6) ? "[FLEET HQ]: Blockade shattered! You've entered the graveyard of the 4th Fleet." :
                (ctx->wave == 7) ? "[FLEET HQ]: Heavy Bulwark ironclads deploying composite shields. Keep moving!" :
                (ctx->wave == 8) ? "[FLEET HQ]: Refugee convoy nearing hyperspace buoy! Hold the line, Ace!" :
                (ctx->wave == 9) ? "[FLEET HQ]: Heavy siege wave incoming. Collect weapon power-ups and fire at will!" :
                                   "[FLEET HQ]: Imperial fortress flagship detected. Bring her down for the Free Colonies!";

            /* 1. Sector & Wave Center Banner */
            int stw = MeasureText(sectorName, 18);
            Color secCol = bossWave ? (Color){ 255, 60, 60, a } : (Color){ 0, 220, 255, a };
            DrawText(sectorName, sw / 2 - stw / 2, sh / 2 - 76, 18, secCol);

            char wbuf[32];
            snprintf(wbuf, sizeof(wbuf), bossWave ? "!! BOSS ENCOUNTER !!" : "WAVE %d", ctx->wave);
            int tw = MeasureText(wbuf, 44);
            Color wc = bossWave ? (Color){ 255, 80, 80, a } : (Color){ 255, 215, 0, a };
            DrawText(wbuf, sw / 2 - tw / 2, sh / 2 - 46, 44, wc);

            /* 2. Tactical Comms Box at Bottom */
            int cboxW = MIN(740, sw - 80);
            int cboxH = 50;
            int cboxX = sw / 2 - cboxW / 2;
            int cboxY = sh - 100;
            DrawRectangleRounded((Rectangle){ (float)cboxX, (float)cboxY, (float)cboxW, (float)cboxH },
                                 0.15f, 6, (Color){ 4, 8, 20, (unsigned char)(alpha * 210) });
            DrawRectangleRoundedLines((Rectangle){ (float)cboxX, (float)cboxY, (float)cboxW, (float)cboxH },
                                      0.15f, 6, (Color){ 0, 180, 255, (unsigned char)(alpha * 160) });
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
            DrawRectangleRounded((Rectangle){(float)hx - 10, (float)hy - 8, 220, 160},
                                 0.1f, 6, (Color){6, 6, 24, a});
            DrawRectangleRoundedLines((Rectangle){(float)hx - 10, (float)hy - 8, 220, 160},
                                      0.1f, 6, (Color){0, 150, 255, (unsigned char)(a / 2)});
            DrawText("MISSION CONTROLS", hx, hy,      11, (Color){255, 200, 0, ta});
            DrawText("W / UP     Thrust",  hx, hy + 18, 13, (Color){0, 220, 255, ta});
            DrawText("S / DOWN   Brake",   hx, hy + 36, 13, (Color){0, 220, 255, ta});
            DrawText("A / D      Rotate",  hx, hy + 54, 13, (Color){0, 220, 255, ta});
            DrawText("SPACE      Fire",    hx, hy + 72, 13, (Color){0, 220, 255, ta});
            DrawText("ESC        Pause",   hx, hy + 90, 13, (Color){0, 220, 255, ta});
            DrawText("F1         Toggle",  hx, hy + 108, 13, (Color){160, 160, 200, ta});
            DrawText("Press F1 to hide",   hx, hy + 132, 11, (Color){120, 120, 160, (unsigned char)(ta / 2)});
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

