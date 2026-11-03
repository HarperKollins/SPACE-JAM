/**
 * config.h - Global compile-time configuration for SPACE JAM
 *
 * All tunable constants are centralized here. Modify these values to
 * balance gameplay or tune performance without hunting through the codebase.
 */
#ifndef SPACEJAM_CONFIG_H
#define SPACEJAM_CONFIG_H

#include <stdint.h>
#include "font_manager.h"

/* ----------------------------------------------------------
   Window / Display
---------------------------------------------------------- */
#define SCREEN_WIDTH        1280
#define SCREEN_HEIGHT       720
#define GAME_TITLE          "SPACE JAM"
#define TARGET_FPS          60
#define MAX_FRAME_TIME      0.05f   /* clamp delta > 50 ms to prevent spiral */

/* ----------------------------------------------------------
   Starfield
---------------------------------------------------------- */
#define STAR_LAYERS         3
#define STARS_PER_LAYER     150
#define STAR_SPEED_LAYER0   20.0f   /* slowest / farthest */
#define STAR_SPEED_LAYER1   45.0f
#define STAR_SPEED_LAYER2   80.0f   /* fastest / nearest */

/* ----------------------------------------------------------
   Player Physics
---------------------------------------------------------- */
#define PLAYER_ACCEL        550.0f
#define PLAYER_BOOST_ACCEL  950.0f
#define PLAYER_MAX_SPEED    520.0f
#define PLAYER_BOOST_SPEED  780.0f
#define PLAYER_DAMPING      0.975f
#define PLAYER_ROT_SPEED    340.0f  /* degrees per second - fast, agile dogfighter */
#define PLAYER_RADIUS       18.0f
#define PLAYER_MAX_HEALTH   100
#define PLAYER_MAX_LIVES    3
#define PLAYER_IFRAMES      1.8f    /* invulnerability seconds after hit */

/* ----------------------------------------------------------
   Weapons
---------------------------------------------------------- */
#define MAX_PLAYER_BULLETS  128
#define BULLET_SPEED        820.0f
#define BULLET_LIFETIME     2.0f
#define BULLET_DAMAGE       12
#define BULLET_COOLDOWN     0.15f   /* seconds between shots */
#define BULLET_RADIUS       4.5f

/* ----------------------------------------------------------
   Enemy Bullets
---------------------------------------------------------- */
#define MAX_ENEMY_BULLETS   128
#define ENEMY_BULLET_SPEED  250.0f  /* readable, dogfightable combat speed */
#define ENEMY_BULLET_LIFE   3.8f
#define ENEMY_BULLET_DAMAGE 10
#define ENEMY_BULLET_RADIUS 4.5f

/* ----------------------------------------------------------
   Enemies
---------------------------------------------------------- */
#define MAX_ENEMIES         32
#define ENEMY_SPAWN_MARGIN  60.0f   /* spawn outside screen by this many px */

/* Scout */
#define SCOUT_SPEED         200.0f
#define SCOUT_HEALTH        20
#define SCOUT_SCORE         100
#define SCOUT_RADIUS        14.0f

/* Interceptor */
#define INTERCEPTOR_SPEED   140.0f
#define INTERCEPTOR_HEALTH  50
#define INTERCEPTOR_SCORE   200
#define INTERCEPTOR_RADIUS  18.0f

/* Tank */
#define TANK_SPEED          70.0f
#define TANK_HEALTH         150
#define TANK_SCORE          350
#define TANK_RADIUS         26.0f

/* Bomber */
#define BOMBER_SPEED        100.0f
#define BOMBER_HEALTH       80
#define BOMBER_SCORE        300
#define BOMBER_RADIUS       20.0f
#define BOMBER_FIRE_INTERVAL 2.2f
#define BOMBER_PREFERRED_DIST 280.0f

/* ----------------------------------------------------------
   Asteroids
---------------------------------------------------------- */
#define MAX_ASTEROIDS       48
#define ASTEROID_SPAWN_INTERVAL 3.0f  /* base seconds between spawns */
#define ASTEROID_LARGE_RADIUS   32.0f
#define ASTEROID_MEDIUM_RADIUS  18.0f
#define ASTEROID_SMALL_RADIUS   10.0f
#define ASTEROID_LARGE_SPEED    55.0f
#define ASTEROID_MEDIUM_SPEED   90.0f
#define ASTEROID_SMALL_SPEED    140.0f
/* Asteroid health: each hit from a standard laser (BULLET_DAMAGE=10)
   takes 10 HP. Setting large=30, medium=20, small=10 ensures large takes
   3 hits, medium takes 2 hits, and small takes 1 hit to destroy/split. */
#define ASTEROID_LARGE_HP       30
#define ASTEROID_MEDIUM_HP      20
#define ASTEROID_SMALL_HP       10
#define ASTEROID_LARGE_SCORE    50
#define ASTEROID_MEDIUM_SCORE   75
#define ASTEROID_SMALL_SCORE    100

/* ----------------------------------------------------------
   Particles
---------------------------------------------------------- */
#define MAX_PARTICLES       512
#define EXHAUST_PARTICLES_PER_FRAME  2

/* ----------------------------------------------------------
   Power-ups
---------------------------------------------------------- */
#define MAX_POWERUPS        8
#define POWERUP_RADIUS      14.0f
#define POWERUP_LIFETIME    12.0f   /* seconds before it disappears */
#define POWERUP_SPAWN_CHANCE 0.25f  /* probability when enemy dies */

/* Power-up durations (seconds) */
#define POWERUP_SHIELD_DURATION    8.0f
#define POWERUP_RAPIDFIRE_DURATION 6.0f
#define POWERUP_DOUBLESHOT_DURATION 8.0f
#define POWERUP_SPREAD_DURATION    7.0f
#define POWERUP_INVINCIBLE_DURATION 5.0f
#define POWERUP_HEALTH_RESTORE     40

/* ----------------------------------------------------------
   Wave System
---------------------------------------------------------- */
#define WAVE_CLEAR_PAUSE    2.5f    /* seconds between waves */
#define BOSS_WAVE_INTERVAL  5       /* boss appears every N waves */

/* ----------------------------------------------------------
   Boss
---------------------------------------------------------- */
#define BOSS_HEALTH         800
#define BOSS_RADIUS         55.0f
#define BOSS_SPEED          80.0f
#define BOSS_SCORE          5000
#define BOSS_BULLET_INTERVAL 0.6f
#define BOSS_PHASE2_HP_THRESHOLD 0.5f  /* enters phase 2 at 50% HP */

/* ----------------------------------------------------------
   Audio
---------------------------------------------------------- */
#define DEFAULT_MASTER_VOL  1.0f
#define DEFAULT_MUSIC_VOL   0.85f
#define DEFAULT_SFX_VOL     1.0f

/* ----------------------------------------------------------
   High Score
---------------------------------------------------------- */
#define HIGHSCORE_FILE      "spacejam_scores.dat"
#define HIGHSCORE_MAX_ENTRIES 10
#define HIGHSCORE_MAGIC     0x534A4D31u  /* "SJM1" */

/* ----------------------------------------------------------
   Misc
---------------------------------------------------------- */
#define SCREEN_SHAKE_MAX    8.0f
#define SCREEN_SHAKE_DECAY  6.0f

#endif /* SPACEJAM_CONFIG_H */

