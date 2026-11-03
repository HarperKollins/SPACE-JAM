/**
 * player.c - Player spacecraft logic, upgrades, gamepad, and rendering
 */
#include "player.h"
#include "progress.h"
#include "utils.h"
#include "config.h"
#include "raylib.h"
#include <stdio.h>
#include <math.h>

/* -- Init ------------------------------------------------------------------ */

static Texture2D LoadPlayerTextureWithFallback(const char *relPath) {
    char buf[512];
    if (FileExists(relPath)) return LoadTexture(relPath);

    const char *appDir = GetApplicationDirectory();
    if (appDir && appDir[0] != '\0') {
        snprintf(buf, sizeof(buf), "%s%s", appDir, relPath);
        if (FileExists(buf)) return LoadTexture(buf);

        snprintf(buf, sizeof(buf), "%s../%s", appDir, relPath);
        if (FileExists(buf)) return LoadTexture(buf);
    }

    snprintf(buf, sizeof(buf), "SPACE_JAM/%s", relPath);
    if (FileExists(buf)) return LoadTexture(buf);

    snprintf(buf, sizeof(buf), "../%s", relPath);
    if (FileExists(buf)) return LoadTexture(buf);

    return LoadTexture(relPath);
}

void Player_Init(Player *p, int screenW, int screenH) {
    p->pos.x     = (float)screenW  * 0.5f;
    p->pos.y     = (float)screenH  * 0.7f;
    p->vel.x     = 0.0f;
    p->vel.y     = 0.0f;
    p->rotation  = 0.0f;

    p->maxHealth = PLAYER_MAX_HEALTH;
    p->health    = PLAYER_MAX_HEALTH;
    p->shield    = 0;
    p->maxShield = 0;
    p->shieldRechargeTimer = 0.0f;
    p->armorReduction = 0.0f;
    p->lives     = PLAYER_MAX_LIVES;
    p->iFrameTimer = 0.0f;
    p->dead      = 0;
    p->deathTimer = 0.0f;

    p->weapon       = WEAPON_SINGLE;
    p->shipTier     = SHIP_TIER_MK1;
    p->fireCooldown = 0.0f;
    p->rapidFireTimer   = 0.0f;
    p->doubleShotTimer  = 0.0f;
    p->spreadShotTimer  = 0.0f;
    p->shieldTimer      = 0.0f;
    p->invincibleTimer  = 0.0f;
    p->plasmaDamage     = BULLET_DAMAGE;

    p->torpedoLevel    = 0;
    p->torpedoCooldown = 3.5f;
    p->torpedoTimer    = 0.0f;

    p->hasGamepad   = 0;
    p->thrustAnim   = 0.45f; /* Always active base nitro flex */
    p->boostAnim    = 0.0f;
    p->bankAngle    = 0.0f;
    p->flashTimer   = 0.0f;
    p->shieldActive = 0;
    p->isBoosting   = 0;

    if (!p->spriteLoaded) {
        p->texSprite = LoadPlayerTextureWithFallback("assets/textures/vanguard.png");
        if (p->texSprite.id > 0) p->spriteLoaded = 1;
    }
}

void Player_Shutdown(Player *p) {
    if (p->texSprite.id > 0) {
        UnloadTexture(p->texSprite);
        p->texSprite.id = 0;
    }
    p->spriteLoaded = 0;
}

void Player_ApplyUpgrades(Player *p, const PlayerProgress *prog) {
    if (!prog) return;
    int hullLvl   = prog->upgradeLevels[UPGRADE_HULL];
    int shieldLvl = prog->upgradeLevels[UPGRADE_SHIELD];
    int gunLvl    = prog->upgradeLevels[UPGRADE_OVERCHARGE];
    int torpLvl   = prog->upgradeLevels[UPGRADE_TORPEDOES];

    p->maxHealth = 100 + hullLvl * 25;
    if (p->health > p->maxHealth) p->health = p->maxHealth;
    p->armorReduction = hullLvl * 0.05f;

    p->maxShield = shieldLvl * 20;
    if (p->shield == 0 && p->maxShield > 0) p->shield = p->maxShield;
    if (p->shield > p->maxShield) p->shield = p->maxShield;

    p->plasmaDamage = 12 + gunLvl * 3;
    p->torpedoLevel = torpLvl;
    p->torpedoCooldown = 3.8f - torpLvl * 0.4f;
}

void Player_SetShipTier(Player *p, ShipTier tier) {
    if (tier > p->shipTier) {
        int bonus = (tier == SHIP_TIER_MK2) ? 20 : (tier == SHIP_TIER_MK3) ? 30 : 50;
        p->health = MIN(p->maxHealth + bonus, p->health + bonus);
    }
    p->shipTier = tier;
}

void Player_ResetPosition(Player *p, int screenW, int screenH) {
    p->pos.x = (float)screenW * 0.5f;
    p->pos.y = (float)screenH * 0.7f;
    p->vel   = (Vector2){0, 0};
    p->rotation = 0.0f;
}

/* -- Input & Physics ------------------------------------------------------- */

void Player_Update(Player *p, float dt, int screenW, int screenH) {
    if (p->dead) {
        p->deathTimer += dt;
        return;
    }

    /* Check Gamepad (slot 0) */
    p->hasGamepad = IsGamepadAvailable(0);
    float padStickX = 0.0f;
    float padStickY = 0.0f;
    int padBoost = 0;
    int padThrust = 0;

    if (p->hasGamepad) {
        padStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        padStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        /* Apply deadzone */
        if (fabsf(padStickX) < 0.20f) padStickX = 0.0f;
        if (fabsf(padStickY) < 0.20f) padStickY = 0.0f;

        float padRT = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_TRIGGER);
        float padLT = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_TRIGGER);
        if (padLT > 0.25f || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) padBoost = 1;
        if (padRT > 0.25f || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) padThrust = 1;
    }

    /* -- Rotation & 2.5D Visual Banking -------------------- */
    float rot = 0.0f;
    float targetBank = 0.0f;

    if (p->hasGamepad && (padStickX != 0.0f || padStickY != 0.0f)) {
        /* Direct 360 degree analog stick flight heading */
        float targetAngle = RAD2DEG(atan2f(padStickX, -padStickY));
        float diff = targetAngle - p->rotation;
        while (diff < -180.0f) diff += 360.0f;
        while (diff > 180.0f)  diff -= 360.0f;
        p->rotation += diff * MIN(1.0f, dt * 14.0f);
        targetBank = CLAMP(diff / 45.0f, -1.0f, 1.0f);
    } else {
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT) ||
            (p->hasGamepad && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT))) {
            rot -= PLAYER_ROT_SPEED;
            targetBank -= 1.0f;
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT) ||
            (p->hasGamepad && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))) {
            rot += PLAYER_ROT_SPEED;
            targetBank += 1.0f;
        }
        p->rotation += rot * dt;
    }

    /* Smoothly interpolate visual bank angle for snappy 2.5D roll */
    p->bankAngle += (targetBank - p->bankAngle) * MIN(1.0f, dt * 12.0f);

    /* -- Thrust & Hyper-Nitro Afterburner ------------------ */
    Vector2 forward = AngleToDir(p->rotation);
    float stickMag = sqrtf(padStickX * padStickX + padStickY * padStickY);
    int thrustForward = (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) ||
                         (p->hasGamepad && (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP) || stickMag > 0.35f)));
    int hyperNitro    = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) ||
                         IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || padBoost);

    p->isBoosting = (thrustForward && hyperNitro) || (hyperNitro && Vec2Len(p->vel) > 30.0f);

    float accel = PLAYER_ACCEL;
    float maxSpeed = PLAYER_MAX_SPEED;

    if (p->isBoosting) {
        accel = PLAYER_BOOST_ACCEL;
        maxSpeed = PLAYER_BOOST_SPEED;
    }

    if (thrustForward || p->isBoosting) {
        p->vel.x += forward.x * accel * dt;
        p->vel.y += forward.y * accel * dt;
    }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN) ||
        (p->hasGamepad && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN))) {
        p->vel.x -= forward.x * accel * 0.55f * dt;
        p->vel.y -= forward.y * accel * 0.55f * dt;
    }

    /* -- Speed cap ----------------------------------------- */
    float speed = Vec2Len(p->vel);
    if (speed > maxSpeed) {
        p->vel = Vec2Scale(Vec2Norm(p->vel), maxSpeed);
    }

    /* -- Damping (frame-rate independent) ------------------ */
    float damping = powf(PLAYER_DAMPING, dt * 60.0f);
    p->vel.x *= damping;
    p->vel.y *= damping;

    /* -- Move ---------------------------------------------- */
    p->pos.x += p->vel.x * dt;
    p->pos.y += p->vel.y * dt;

    /* -- Screen boundaries (wrap) -------------------------- */
    float pad = PLAYER_RADIUS + 10.0f;
    if (p->pos.x < -pad) p->pos.x = (float)screenW + pad;
    if (p->pos.x > (float)screenW + pad) p->pos.x = -pad;
    if (p->pos.y < -pad) p->pos.y = (float)screenH + pad;
    if (p->pos.y > (float)screenH + pad) p->pos.y = -pad;

    /* -- Timers -------------------------------------------- */
    if (p->fireCooldown    > 0.0f) p->fireCooldown    -= dt;
    if (p->iFrameTimer     > 0.0f) p->iFrameTimer     -= dt;
    if (p->flashTimer      > 0.0f) p->flashTimer      -= dt;
    if (p->torpedoTimer    > 0.0f) p->torpedoTimer    -= dt;

    /* Deflector Shield Auto-Recharge */
    if (p->maxShield > 0) {
        if (p->shieldRechargeTimer > 0.0f) {
            p->shieldRechargeTimer -= dt;
        } else if (p->shield < p->maxShield) {
            float rechargeRate = 12.0f + p->maxShield * 0.15f;
            p->shield = MIN(p->maxShield, p->shield + (int)(rechargeRate * dt + 0.5f));
        }
    }

    /* -- Always-Active Nitro Flex Animation ---------------- */
    float targetFlex = (thrustForward || p->isBoosting) ? 1.0f : 0.40f;
    p->thrustAnim += (targetFlex - p->thrustAnim) * MIN(1.0f, dt * 10.0f);

    if (p->isBoosting) {
        p->boostAnim = MIN(1.0f, p->boostAnim + dt * 12.0f);
    } else {
        p->boostAnim = MAX(0.0f, p->boostAnim - dt * 6.0f);
    }

    /* -- Weapon upgrade expiry ----------------------------- */
    if (p->rapidFireTimer > 0.0f) {
        p->rapidFireTimer -= dt;
        if (p->rapidFireTimer <= 0.0f) p->weapon = WEAPON_SINGLE;
    }
    if (p->doubleShotTimer > 0.0f) {
        p->doubleShotTimer -= dt;
        if (p->doubleShotTimer <= 0.0f) p->weapon = WEAPON_SINGLE;
    }
    if (p->spreadShotTimer > 0.0f) {
        p->spreadShotTimer -= dt;
        if (p->spreadShotTimer <= 0.0f) p->weapon = WEAPON_SINGLE;
    }
    if (p->shieldTimer > 0.0f) {
        p->shieldTimer -= dt;
        if (p->shieldTimer <= 0.0f) p->shieldActive = 0;
    }
    if (p->invincibleTimer > 0.0f) {
        p->invincibleTimer -= dt;
    }
}

/* -- Drawing --------------------------------------------------------------- */

void Player_Draw(const Player *p) {
    if (p->dead) return;

    /* Invulnerability flash - blink every 0.1s */
    if (p->iFrameTimer > 0.0f) {
        int blink = (int)(p->iFrameTimer * 10.0f) % 2;
        if (blink == 0) return;
    }

    /* Draw Deflector Shield Bubble */
    if (p->shieldActive || p->shield > 0) {
        float shieldPulse = 0.85f + 0.15f * sinf(GetTime() * 6.0f);
        float sRadius = PLAYER_RADIUS + 12.0f;
        Color sCol = p->shieldActive ? (Color){0, 240, 255, (unsigned char)(90 * shieldPulse)}
                                     : (Color){60, 160, 255, (unsigned char)(70 * shieldPulse)};
        DrawCircleV(p->pos, sRadius, ColorAlpha(sCol, 0.25f));
        DrawCircleLines((int)p->pos.x, (int)p->pos.y, sRadius, sCol);
        DrawCircleLines((int)p->pos.x, (int)p->pos.y, sRadius - 2.0f, ColorAlpha(sCol, 0.5f));
    }

    /* Invincibility aura */
    if (p->invincibleTimer > 0.0f) {
        float auraPulse = 0.5f + 0.5f * sinf(GetTime() * 12.0f);
        DrawCircleV(p->pos, PLAYER_RADIUS + 8.0f,
                    ColorAlpha((Color){255, 220, 50, 255}, 0.25f * auraPulse));
        DrawCircleLines((int)p->pos.x, (int)p->pos.y, PLAYER_RADIUS + 8.0f,
                        ColorAlpha((Color){255, 240, 100, 255}, 0.7f * auraPulse));
    }

    /* -- Afterburner Engine Flare -------------------------- */
    Vector2 backDir = AngleToDir(p->rotation + 180.0f);
    float flareLen = 14.0f + p->thrustAnim * 16.0f + p->boostAnim * 28.0f;
    float flareW   = 6.0f + p->thrustAnim * 4.0f + p->boostAnim * 8.0f;

    Vector2 enginePos = Vec2Add(p->pos, Vec2Scale(backDir, 16.0f));
    Vector2 flareTip  = Vec2Add(enginePos, Vec2Scale(backDir, flareLen));
    Vector2 rightDir  = AngleToDir(p->rotation + 90.0f);

    Vector2 fl1 = Vec2Add(enginePos, Vec2Scale(rightDir, flareW * 0.5f));
    Vector2 fl2 = Vec2Add(enginePos, Vec2Scale(rightDir, -flareW * 0.5f));

    Color flareCol = p->isBoosting ? (Color){ 255, 120, 30, 255 } : (Color){ 0, 200, 255, 230 };
    DrawTriangle(fl1, fl2, flareTip, flareCol);
    DrawCircleV(enginePos, flareW * 0.6f, (Color){ 255, 255, 255, 240 });

    /* -- HK-01 Vanguard Starfighter Render ----------------- */
    float rot = DEG2RAD(p->rotation);
    float c = cosf(rot);
    float s = sinf(rot);
    float bankScale = 1.0f - fabsf(p->bankAngle) * 0.28f;

    #define RP(ox, oy) ((Vector2){ p->pos.x + ((ox) * c - (oy) * s), p->pos.y + ((ox) * s + (oy) * c) })
    #define RPB(ox, oy) ((Vector2){ p->pos.x + (((ox) * bankScale) * c - (oy) * s), p->pos.y + (((ox) * bankScale) * s + (oy) * c) })

    /* Hull shadow / underglow */
    DrawTriangle(RPB(-22, 14), RPB(22, 14), RP(0, -22), (Color){ 10, 18, 35, 220 });

    /* Wing Panels */
    DrawTriangle(RPB(-24, 12), RPB(-6, 2), RP(0, -18), (Color){ 45, 65, 95, 255 });
    DrawTriangle(RPB( 24, 12), RP(0, -18), RPB( 6, 2), (Color){ 45, 65, 95, 255 });

    /* Main Titanium Fuselage */
    DrawTriangle(RPB(-10, 10), RPB(10, 10), RP(0, -24), (Color){ 215, 230, 245, 255 });

    /* Cockpit Canopy */
    DrawTriangle(RPB(-5, -2), RPB(5, -2), RP(0, -16), (Color){ 0, 230, 255, 255 });
    DrawCircleV(RP(0, -8), 3.0f, (Color){ 200, 255, 255, 255 });

    /* Torpedo Hardpoint Pods */
    if (p->torpedoLevel > 0) {
        DrawRectangleV(RPB(-18, 2), (Vector2){ 3.0f, 7.0f }, (Color){ 255, 140, 20, 255 });
        DrawRectangleV(RPB( 15, 2), (Vector2){ 3.0f, 7.0f }, (Color){ 255, 140, 20, 255 });
    }

    #undef RP
    #undef RPB
}

/* -- Combat ---------------------------------------------------------------- */

void Player_TakeDamage(Player *p, int dmg) {
    if (p->dead || p->iFrameTimer > 0.0f || p->invincibleTimer > 0.0f) return;

    p->shieldRechargeTimer = 4.5f; /* Reset shield recharge cooldown */

    /* Deflector Shield absorbs damage first */
    if (p->shieldActive) {
        p->shieldActive = 0;
        p->iFrameTimer = 0.6f;
        p->flashTimer  = 0.2f;
        return;
    }

    if (p->shield > 0) {
        if (p->shield >= dmg) {
            p->shield -= dmg;
            p->flashTimer = 0.2f;
            p->iFrameTimer = 0.35f;
            return;
        } else {
            dmg -= p->shield;
            p->shield = 0;
        }
    }

    /* Apply Armor damage reduction */
    int effectiveDmg = (int)((float)dmg * (1.0f - p->armorReduction));
    if (effectiveDmg < 1) effectiveDmg = 1;

    p->health -= effectiveDmg;
    p->flashTimer = 0.3f;
    p->iFrameTimer = PLAYER_IFRAMES;

    if (p->health <= 0) {
        p->health = 0;
        p->lives--;
        if (p->lives > 0) {
            /* Respawn - restore health & shield */
            p->health = p->maxHealth;
            p->shield = p->maxShield;
            p->iFrameTimer = 3.0f;
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

    int mouseFire = 0;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mpos = GetMousePosition();
        int sw = GetScreenWidth();
        if (!(mpos.x > (float)(sw - 260) && mpos.y < 65.0f)) {
            mouseFire = 1;
        }
    }

    int padFire = 0;
    if (p->hasGamepad) {
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||
            GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_TRIGGER) > 0.25f) {
            padFire = 1;
        }
    }

    if (!IsKeyDown(KEY_SPACE) && !IsKeyDown(KEY_J) && !IsKeyDown(KEY_Z) && !mouseFire && !padFire) return 0;

    float cooldown = BULLET_COOLDOWN;
    if (p->weapon == WEAPON_RAPID) cooldown *= 0.35f;

    if (p->fireCooldown > 0.0f) return 0;

    p->fireCooldown = cooldown;
    return 1;
}

int Player_CanFireTorpedo(const Player *p) {
    if (p->dead || p->torpedoLevel <= 0) return 0;
    if (p->torpedoTimer > 0.0f) return 0;

    if (IsKeyPressed(KEY_C) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) return 1;

    if (p->hasGamepad && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) ||
                          IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1))) {
        return 1;
    }
    return 0;
}

void Player_TriggerTorpedoCooldown(Player *p) {
    p->torpedoTimer = p->torpedoCooldown;
}

void Player_ApplyPowerup(Player *p, int powerupType) {
    switch (powerupType) {
        case 0: /* POWERUP_HEALTH */
            p->health = MIN(p->maxHealth, p->health + POWERUP_HEALTH_RESTORE);
            break;
        case 1: /* POWERUP_SHIELD */
            p->shieldTimer = POWERUP_SHIELD_DURATION;
            p->shieldActive = 1;
            p->shield = p->maxShield;
            break;
        case 2: /* POWERUP_RAPIDFIRE */
            p->doubleShotTimer = 0.0f;
            p->spreadShotTimer = 0.0f;
            p->rapidFireTimer  = POWERUP_RAPIDFIRE_DURATION;
            p->weapon = WEAPON_RAPID;
            break;
        case 3: /* POWERUP_DOUBLESHOT */
            p->spreadShotTimer = 0.0f;
            if (p->rapidFireTimer <= 0.0f) {
                p->doubleShotTimer = POWERUP_DOUBLESHOT_DURATION;
                p->weapon = WEAPON_DOUBLE;
            }
            break;
        case 4: /* POWERUP_SPREAD */
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
