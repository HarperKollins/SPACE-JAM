/**
 * player.c - Player spacecraft logic and rendering
 */
#include "player.h"
#include "utils.h"
#include "config.h"
#include "raylib.h"
#include <math.h>

/* ── Init ────────────────────────────────────────────── */

void Player_Init(Player *p, int screenW, int screenH) {
    p->pos.x     = (float)screenW  * 0.5f;
    p->pos.y     = (float)screenH  * 0.7f;
    p->vel.x     = 0.0f;
    p->vel.y     = 0.0f;
    p->rotation  = 0.0f;

    p->health    = PLAYER_MAX_HEALTH;
    p->lives     = PLAYER_MAX_LIVES;
    p->iFrameTimer = 0.0f;
    p->dead      = 0;
    p->deathTimer = 0.0f;

    p->weapon       = WEAPON_SINGLE;
    p->fireCooldown = 0.0f;
    p->rapidFireTimer   = 0.0f;
    p->doubleShotTimer  = 0.0f;
    p->spreadShotTimer  = 0.0f;
    p->shieldTimer      = 0.0f;
    p->invincibleTimer  = 0.0f;
    p->thrustAnim   = 0.0f;
    p->flashTimer   = 0.0f;
    p->shieldActive = 0;
}

/* ── Input & Physics ─────────────────────────────────── */

void Player_Update(Player *p, float dt, int screenW, int screenH) {
    if (p->dead) {
        p->deathTimer += dt;
        return;
    }

    /* ── Rotation ── */
    float rot = 0.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  rot -= PLAYER_ROT_SPEED;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) rot += PLAYER_ROT_SPEED;
    p->rotation += rot * dt;

    /* ── Thrust ── */
    Vector2 forward = AngleToDir(p->rotation);
    int thrusting = 0;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
        p->vel.x += forward.x * PLAYER_ACCEL * dt;
        p->vel.y += forward.y * PLAYER_ACCEL * dt;
        thrusting = 1;
    }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
        p->vel.x -= forward.x * PLAYER_ACCEL * 0.55f * dt;
        p->vel.y -= forward.y * PLAYER_ACCEL * 0.55f * dt;
    }

    /* ── Speed cap ── */
    float speed = Vec2Len(p->vel);
    if (speed > PLAYER_MAX_SPEED) {
        p->vel = Vec2Scale(Vec2Norm(p->vel), PLAYER_MAX_SPEED);
    }

    /* ── Damping (frame-rate independent) ── */
    float damping = powf(PLAYER_DAMPING, dt * 60.0f);
    p->vel.x *= damping;
    p->vel.y *= damping;

    /* ── Move ── */
    p->pos.x += p->vel.x * dt;
    p->pos.y += p->vel.y * dt;

    /* ── Screen boundaries (wrap) ── */
    float margin = PLAYER_RADIUS;
    if (p->pos.x < -margin)       p->pos.x = (float)screenW + margin;
    if (p->pos.x > screenW + margin) p->pos.x = -margin;
    if (p->pos.y < -margin)       p->pos.y = (float)screenH + margin;
    if (p->pos.y > screenH + margin) p->pos.y = -margin;

    /* ── Timers ── */
    if (p->iFrameTimer > 0.0f)    p->iFrameTimer    -= dt;
    if (p->flashTimer  > 0.0f)    p->flashTimer     -= dt;
    if (p->rapidFireTimer > 0.0f) p->rapidFireTimer -= dt;
    if (p->doubleShotTimer > 0.0f) p->doubleShotTimer -= dt;
    if (p->spreadShotTimer > 0.0f) p->spreadShotTimer -= dt;
    if (p->shieldTimer > 0.0f) {
        p->shieldTimer -= dt;
        p->shieldActive = 1;
    } else {
        p->shieldActive = 0;
    }
    if (p->invincibleTimer > 0.0f) p->invincibleTimer -= dt;
    if (p->fireCooldown    > 0.0f) p->fireCooldown    -= dt;

    /* ── Engine animation ── */
    if (thrusting) {
        p->thrustAnim = MIN(1.0f, p->thrustAnim + dt * 8.0f);
    } else {
        p->thrustAnim = MAX(0.0f, p->thrustAnim - dt * 6.0f);
    }

    /* ── Weapon upgrade expiry ── */
    if (p->rapidFireTimer <= 0.0f && p->doubleShotTimer <= 0.0f && p->spreadShotTimer <= 0.0f) {
        p->weapon = WEAPON_SINGLE;
    } else if (p->rapidFireTimer > 0.0f) {
        p->weapon = WEAPON_RAPID;
    } else if (p->spreadShotTimer > 0.0f) {
        p->weapon = WEAPON_SPREAD;
    } else if (p->doubleShotTimer > 0.0f) {
        p->weapon = WEAPON_DOUBLE;
    }
}

/* ── Drawing ─────────────────────────────────────────── */

static void DrawEngineGlow(Vector2 pos, float rotation, float thrust) {
    if (thrust <= 0.01f) return;

    Vector2 forward = AngleToDir(rotation);
    /* Engine nozzle is at the back — opposite of forward */
    Vector2 back = { -forward.x, -forward.y };
    Vector2 nozzle = { pos.x + back.x * 14.0f, pos.y + back.y * 14.0f };

    unsigned char alpha = (unsigned char)(thrust * 220.0f);
    float gSize = thrust * 14.0f;

    DrawCircleV(nozzle, gSize,        (Color){0, 100, 255, alpha / 2});
    DrawCircleV(nozzle, gSize * 0.5f, (Color){100, 200, 255, alpha});
    DrawCircleV(nozzle, gSize * 0.2f, (Color){255, 255, 255, alpha});
}

static void DrawShield(Vector2 pos, float radius, float timer) {
    float pulse = 0.5f + 0.5f * sinf(timer * 4.0f);
    Color inner = { 0, 180, 255, (unsigned char)(60 + pulse * 60) };
    Color outer = { 0, 120, 220, (unsigned char)(30 + pulse * 40) };
    DrawCircleV(pos, radius + 8.0f, outer);
    DrawCircleV(pos, radius + 4.0f, inner);
    DrawCircleLinesV(pos, radius + 8.0f, (Color){100, 220, 255, (unsigned char)(150 + pulse * 105)});
}

void Player_Draw(const Player *p) {
    if (p->dead) return;

    /* Invulnerability flash — blink every 0.1s */
    if (p->iFrameTimer > 0.0f) {
        int blink = (int)(p->iFrameTimer * 10.0f) % 2;
        if (blink) return;
    }

    /* Shield */
    if (p->shieldActive) {
        DrawShield(p->pos, PLAYER_RADIUS, p->shieldTimer);
    }

    /* Invincibility aura */
    if (p->invincibleTimer > 0.0f) {
        float pulse = 0.5f + 0.5f * sinf(GetTime() * 8.0f);
        DrawCircleV(p->pos, PLAYER_RADIUS + 12.0f,
                    (Color){255, 215, 0, (unsigned char)(40 + pulse * 60)});
        DrawCircleLinesV(p->pos, PLAYER_RADIUS + 12.0f,
                         (Color){255, 255, 100, 180});
    }

    /* Engine glow */
    DrawEngineGlow(p->pos, p->rotation, p->thrustAnim);

    /* ── Ship body — procedural vector-style spacecraft ── */
    float rot = DEG2RAD(p->rotation);
    float cosR = cosf(rot);
    float sinR = sinf(rot);

    /* Helper: rotate point around ship center */
    #define RP(lx, ly) (Vector2){ p->pos.x + (lx)*cosR - (ly)*sinR, \
                                   p->pos.y + (lx)*sinR + (ly)*cosR }

    /* Main body — blue-white triangular hull */
    Color hullColor = (Color){30, 130, 255, 255};
    Color hullEdge  = (Color){80, 200, 255, 255};

    /* Hit flash */
    if (p->flashTimer > 0.0f) {
        float frac = p->flashTimer / 0.3f;
        hullColor = ColorLerp(hullColor, WHITE, frac);
        hullEdge  = WHITE;
    }

    /* Wing strakes */
    Vector2 wingL[3] = { RP(-16, 10), RP(-8, -4), RP(-4, 12) };
    Vector2 wingR[3] = { RP( 16, 10), RP( 8, -4), RP( 4, 12) };
    DrawTriangle(wingL[0], wingL[1], wingL[2], (Color){20, 80, 180, 220});
    DrawTriangle(wingR[1], wingR[0], wingR[2], (Color){20, 80, 180, 220});
    DrawTriangleLines(wingL[0], wingL[1], wingL[2], hullEdge);
    DrawTriangleLines(wingR[1], wingR[0], wingR[2], hullEdge);

    /* Main hull */
    Vector2 hull[3] = { RP(0, -20), RP(-12, 12), RP(12, 12) };
    DrawTriangle(hull[0], hull[1], hull[2], hullColor);
    DrawTriangleLines(hull[0], hull[1], hull[2], hullEdge);

    /* Cockpit canopy */
    DrawCircleV(RP(0, -5), 5.5f, (Color){100, 220, 255, 200});
    DrawCircleLinesV(RP(0, -5), 5.5f, (Color){160, 255, 255, 255});

    /* Weapon glow (double/rapid/spread) */
    if (p->weapon != WEAPON_SINGLE) {
        float pulse = 0.6f + 0.4f * sinf(GetTime() * 12.0f);
        Color wc;
        if (p->weapon == WEAPON_RAPID) {
            wc = (Color){255, 100, 0, (unsigned char)(100 * pulse)};
        } else if (p->weapon == WEAPON_SPREAD) {
            wc = (Color){255, 220, 0, (unsigned char)(100 * pulse)};
        } else {
            wc = (Color){0, 255, 180, (unsigned char)(100 * pulse)};
        }
        DrawCircleV(RP(-8, -2), 3.5f, wc);
        DrawCircleV(RP( 8, -2), 3.5f, wc);
    }

    #undef RP
}

/* ── Combat ──────────────────────────────────────────── */

void Player_TakeDamage(Player *p, int dmg) {
    if (p->dead) return;
    if (p->iFrameTimer > 0.0f) return;
    if (p->invincibleTimer > 0.0f) return;

    if (p->shieldActive) {
        /* Shield absorbs the hit */
        p->shieldTimer = 0.0f;
        p->shieldActive = 0;
        p->iFrameTimer = 0.6f;
        p->flashTimer  = 0.2f;
        return;
    }

    p->health -= dmg;
    p->flashTimer = 0.3f;
    p->iFrameTimer = PLAYER_IFRAMES;

    if (p->health <= 0) {
        p->health = 0;
        p->lives--;
        if (p->lives > 0) {
            /* Respawn — reset health but keep lives */
            p->health = PLAYER_MAX_HEALTH;
            p->iFrameTimer = 3.0f; /* longer grace period on respawn */
            p->vel = (Vector2){0, 0};
        } else {
            p->dead = 1;
            p->deathTimer = 0.0f;
        }
    }
}

int Player_TryShoot(Player *p, float dt) {
    (void)dt;
    if (p->dead) return 0;
    if (!IsKeyDown(KEY_SPACE)) return 0;

    float cooldown = BULLET_COOLDOWN;
    if (p->weapon == WEAPON_RAPID) cooldown *= 0.35f;

    if (p->fireCooldown > 0.0f) return 0;

    p->fireCooldown = cooldown;
    return 1;
}

void Player_ApplyPowerup(Player *p, int powerupType) {
    switch (powerupType) {
        case 0: /* POWERUP_HEALTH */
            p->health = MIN(PLAYER_MAX_HEALTH, p->health + POWERUP_HEALTH_RESTORE);
            break;
        case 1: /* POWERUP_SHIELD */
            p->shieldTimer = POWERUP_SHIELD_DURATION;
            p->shieldActive = 1;
            break;
        case 2: /* POWERUP_RAPIDFIRE */
            /* Cancel competing weapon timers so rapid-fire takes priority */
            p->doubleShotTimer = 0.0f;
            p->spreadShotTimer = 0.0f;
            p->rapidFireTimer  = POWERUP_RAPIDFIRE_DURATION;
            p->weapon = WEAPON_RAPID;
            break;
        case 3: /* POWERUP_DOUBLESHOT */
            /* Only apply if no higher-priority weapon is active */
            p->spreadShotTimer = 0.0f;
            if (p->rapidFireTimer <= 0.0f) {
                p->doubleShotTimer = POWERUP_DOUBLESHOT_DURATION;
                p->weapon = WEAPON_DOUBLE;
            }
            break;
        case 4: /* POWERUP_SPREAD */
            /* Cancel competing weapon timers */
            p->doubleShotTimer = 0.0f;
            if (p->rapidFireTimer <= 0.0f) {
                p->spreadShotTimer = POWERUP_SPREAD_DURATION;
                p->weapon = WEAPON_SPREAD;
            }
            break;
        case 5: /* POWERUP_INVINCIBLE */
            p->invincibleTimer = POWERUP_INVINCIBLE_DURATION;
            break;
        default: break;
    }
}

int Player_IsAlive(const Player *p) {
    return !p->dead;
}

