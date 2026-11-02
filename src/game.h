/**
 * game.h - Top-level game state and coordinator
 */
#ifndef SPACEJAM_GAME_H
#define SPACEJAM_GAME_H

#include <stdint.h>
#include "config.h"

/* ── Game States ─────────────────────────────────────── */
typedef enum {
    STATE_MAIN_MENU = 0,
    STATE_PROLOGUE,
    STATE_COMMS,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER,
    STATE_SETTINGS,
    STATE_CONTROLS,
    STATE_MEDALS,
    STATE_VICTORY,
    STATE_COUNT
} GameState;

/* ── Settings ────────────────────────────────────────── */
typedef struct {
    float masterVolume;
    float musicVolume;
    float sfxVolume;
    int   fullscreen;
    int   screenShake;
    int   particleQuality;  /* 0=low, 1=medium, 2=high */
    int   screenWidth;
    int   screenHeight;
} GameSettings;

/* ── Game Context ────────────────────────────────────── */
typedef struct {
    GameState  state;
    GameState  prevState;
    int        running;

    int        score;
    int        highScore;
    int        wave;
    int        waveEnemiesKilled;
    int        waveEnemiesRequired;
    float      waveClearTimer;
    int        bossActive;

    float      screenShake;
    float      totalTime;

    GameSettings settings;
} GameContext;

/* ── Lifecycle ───────────────────────────────────────── */
void Game_Init(GameContext *ctx);
void Game_Update(GameContext *ctx, float dt);
void Game_Draw(GameContext *ctx);
void Game_Shutdown(GameContext *ctx);
void Game_ChangeState(GameContext *ctx, GameState next);

/* Score / wave helpers */
void Game_AddScore(GameContext *ctx, int points);
void Game_NextWave(GameContext *ctx);
void Game_StartNewGame(GameContext *ctx);
void Game_PlaySFX(int sfxId);

#endif /* SPACEJAM_GAME_H */

