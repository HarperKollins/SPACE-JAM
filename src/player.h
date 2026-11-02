/**
 * player.h - Player spacecraft state and logic
 */
#ifndef SPACEJAM_PLAYER_H
#define SPACEJAM_PLAYER_H

#include "config.h"
#include "raylib.h"

typedef enum {
  WEAPON_SINGLE = 0,
  WEAPON_DOUBLE,
  WEAPON_SPREAD,
  WEAPON_RAPID,
  WEAPON_COUNT
} WeaponType;

typedef struct {
  /* Transform */
  Vector2 pos;
  Vector2 vel;
  float rotation; /* degrees, 0 = up */

  /* Combat */
  int health;
  int lives;
  float iFrameTimer; /* invulnerability frames remaining */
  int dead;
  float deathTimer;

  /* Weapon state */
  WeaponType weapon;
  float fireCooldown;
  float rapidFireTimer;
  float doubleShotTimer;
  float spreadShotTimer;
  float shieldTimer;
  float invincibleTimer;

  /* Visual */
  float thrustAnim; /* 0–1, drives engine glow */
  float flashTimer; /* hit flash */
  int shieldActive;
} Player;

void Player_Init(Player *p, int screenW, int screenH);
void Player_Update(Player *p, float dt, int screenW, int screenH);
void Player_Draw(const Player *p);
void Player_TakeDamage(Player *p, int dmg);
int Player_TryShoot(Player *p, float dt); /* returns 1 if should spawn bullet */
void Player_ApplyPowerup(Player *p, int powerupType);
int Player_IsAlive(const Player *p);

#endif /* SPACEJAM_PLAYER_H */
