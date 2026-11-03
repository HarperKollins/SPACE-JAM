/**
 * collision.h - Collision detection queries
 */
#ifndef SPACEJAM_COLLISION_H
#define SPACEJAM_COLLISION_H

#include "raylib.h"
#include "player.h"
#include "enemy.h"
#include "asteroid.h"
#include "projectile.h"
#include "powerup.h"
#include "particles.h"
#include "game.h"
#include "progress.h"
#include "audio.h"

/**
 * Run all collision checks for one frame.
 * Modifies entities in-place (marks dead, subtracts health, etc.).
 * Appends particles for hit/destroy effects.
 * Returns score earned this frame from kills.
 */
int Collision_Update(
    Player        *player,
    EnemyPool     *enemies,
    AsteroidField *asteroids,
    ProjectilePool *projectiles,
    PowerupPool    *powerups,
    ParticleSystem *particles,
    GameContext    *ctx,
    PlayerProgress *prog,
    AudioManager   *audio
);

/* Circle vs circle */
int CheckCircleCircle(Vector2 aPos, float aRad, Vector2 bPos, float bRad);

#endif /* SPACEJAM_COLLISION_H */

