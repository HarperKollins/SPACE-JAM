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

typedef enum {
  SHIP_TIER_MK1 = 0, /* Sector 1: Standard Vanguard Interceptor */
  SHIP_TIER_MK2,     /* Sector 2: Armored Striker (reinforced titanium prow & vents) */
  SHIP_TIER_MK3,     /* Sector 3: Plasma Ace (cyan plasma circuit etchings & quad hardpoints) */
  SHIP_TIER_MK4,     /* Sector 4: Ascendant Legend (royal gold trims, radiant particle aura) */
  SHIP_TIER_COUNT
} ShipTier;

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
  ShipTier   shipTier;
  float fireCooldown;
  float rapidFireTimer;
  float doubleShotTimer;
  float spreadShotTimer;
  float shieldTimer;
  float invincibleTimer;

  /* Visual */
  float thrustAnim; /* 0-1, drives base engine glow (always active/flexing) */
  float boostAnim;  /* 0-1, hyper-nitro afterburner overdrive */
  float bankAngle;  /* -1.0 to +1.0, 3D visual roll when turning */
  float flashTimer; /* hit flash */
  int shieldActive;
  int isBoosting;
  Texture2D texSprite;
  int spriteLoaded;
} Player;

void Player_Init(Player *p, int screenW, int screenH);
void Player_Shutdown(Player *p);
void Player_SetShipTier(Player *p, ShipTier tier);
void Player_Update(Player *p, float dt, int screenW, int screenH);
void Player_Draw(const Player *p);
void Player_TakeDamage(Player *p, int dmg);
int Player_TryShoot(Player *p, float dt); /* returns 1 if should spawn bullet */
void Player_ApplyPowerup(Player *p, int powerupType);
int Player_IsAlive(const Player *p);

#endif /* SPACEJAM_PLAYER_H */
