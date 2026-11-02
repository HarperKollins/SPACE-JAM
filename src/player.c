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
    p->shipTier     = SHIP_TIER_MK1;
    p->fireCooldown = 0.0f;
    p->rapidFireTimer   = 0.0f;
    p->doubleShotTimer  = 0.0f;
    p->spreadShotTimer  = 0.0f;
    p->shieldTimer      = 0.0f;
    p->invincibleTimer  = 0.0f;
    p->thrustAnim   = 0.45f; /* Always active base nitro flex */
    p->boostAnim    = 0.0f;
    p->bankAngle    = 0.0f;
    p->flashTimer   = 0.0f;
    p->shieldActive = 0;
    p->isBoosting   = 0;
}

void Player_SetShipTier(Player *p, ShipTier tier) {
    if (tier > p->shipTier) {
        /* Hull upgrade bonus */
        int bonus = (tier == SHIP_TIER_MK2) ? 20 : (tier == SHIP_TIER_MK3) ? 30 : 50;
        p->health = MIN(PLAYER_MAX_HEALTH + bonus, p->health + bonus);
    }
    p->shipTier = tier;
}

/* ── Input & Physics ─────────────────────────────────── */

void Player_Update(Player *p, float dt, int screenW, int screenH) {
    if (p->dead) {
        p->deathTimer += dt;
        return;
    }

    /* ── Rotation & 2.5D Visual Banking ── */
    float rot = 0.0f;
    float targetBank = 0.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
        rot -= PLAYER_ROT_SPEED;
        targetBank -= 1.0f;
    }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
        rot += PLAYER_ROT_SPEED;
        targetBank += 1.0f;
    }
    p->rotation += rot * dt;

    /* Smoothly interpolate visual bank angle for snappy 2.5D roll */
    p->bankAngle += (targetBank - p->bankAngle) * MIN(1.0f, dt * 12.0f);

    /* ── Thrust & Hyper-Nitro Afterburner ── */
    Vector2 forward = AngleToDir(p->rotation);
    int thrustForward = (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP));
    int hyperNitro    = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) ||
                         IsMouseButtonDown(MOUSE_BUTTON_RIGHT));

    p->isBoosting = (thrustForward && hyperNitro) || (hyperNitro && Vec2Len(p->vel) > 30.0f);

    float accel = PLAYER_ACCEL;
    float maxSpeed = PLAYER_MAX_SPEED;

    if (p->isBoosting) {
        accel = PLAYER_BOOST_ACCEL;
        maxSpeed = PLAYER_BOOST_SPEED;
    }

    int thrusting = 0;
    if (thrustForward || p->isBoosting) {
        p->vel.x += forward.x * accel * dt;
        p->vel.y += forward.y * accel * dt;
        thrusting = 1;
    }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
        p->vel.x -= forward.x * accel * 0.55f * dt;
        p->vel.y -= forward.y * accel * 0.55f * dt;
    }

    /* ── Speed cap ── */
    float speed = Vec2Len(p->vel);
    if (speed > maxSpeed) {
        p->vel = Vec2Scale(Vec2Norm(p->vel), maxSpeed);
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
    if (p->pos.x < -margin)          p->pos.x = (float)screenW + margin;
    if (p->pos.x > screenW + margin) p->pos.x = -margin;
    if (p->pos.y < -margin)          p->pos.y = (float)screenH + margin;
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

    /* ── Always-Active Nitro Flex Animation ── */
    /* Base engine is ALWAYS flexing with vivid plasma and hot core */
    float targetThrust = thrusting ? 1.0f : 0.42f;
    float thrustRate = thrusting ? 14.0f : 6.0f;
    p->thrustAnim += (targetThrust - p->thrustAnim) * MIN(1.0f, dt * thrustRate);

    /* Nitro overdrive flare */
    float targetBoost = p->isBoosting ? 1.0f : 0.0f;
    p->boostAnim += (targetBoost - p->boostAnim) * MIN(1.0f, dt * 14.0f);

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

static void DrawEngineGlow(Vector2 pos, float rotation, float thrust, float boost) {
    Vector2 forward = AngleToDir(rotation);
    Vector2 back    = { -forward.x, -forward.y };
    Vector2 perp    = { -forward.y, forward.x };
    Vector2 nozzle  = { pos.x + back.x * 14.0f, pos.y + back.y * 14.0f };

    /* 1. Hot Turbine Core: Always flexing glowing orange combustion chamber */
    float timeSec = (float)GetTime();
    float corePulse = 0.88f + 0.12f * sinf(timeSec * 22.0f);
    float coreRadius = 4.8f + boost * 2.2f;
    DrawCircleV(nozzle, coreRadius * corePulse, (Color){ 255, 130, 20, 240 });
    DrawCircleV(nozzle, (coreRadius * 0.55f) * corePulse, (Color){ 255, 235, 90, 255 });
    DrawCircleV(nozzle, (coreRadius * 0.25f), WHITE);

    /* 2. Nitro Flame Dimensions:
          - Idle: 22-26px electric-cyan plasma blade (always alive & flexing)
          - Forward thrust: 50-60px
          - Hyper-nitro boost: 80-98px supersonic blowtorch! */
    float flutter = sinf(timeSec * 45.0f) * 2.0f + RandFloat(-1.2f, 1.2f);
    float jetLength = 22.0f + thrust * 32.0f + boost * 42.0f + flutter;
    float jetWidth  =  6.5f + thrust *  5.0f + boost *  6.5f + RandFloat(-0.5f, 0.5f);

    /* Outer Deep Blue Ion Aura */
    Vector2 haloTip = { nozzle.x + back.x * (jetLength * 1.25f), nozzle.y + back.y * (jetLength * 1.25f) };
    Vector2 haloL   = { nozzle.x - perp.x * (jetWidth * 1.45f),  nozzle.y - perp.y * (jetWidth * 1.45f) };
    Vector2 haloR   = { nozzle.x + perp.x * (jetWidth * 1.45f),  nozzle.y + perp.y * (jetWidth * 1.45f) };
    unsigned char haloAlpha = (unsigned char)(120 + thrust * 80 + boost * 55);
    DrawTriangle(nozzle, haloL, haloTip, (Color){   0, 110, 255, haloAlpha });
    DrawTriangle(nozzle, haloTip, haloR, (Color){   0, 110, 255, haloAlpha });

    /* Mid Electric-Cyan Diamond Jet (The iconic HK-01 nitro flame) */
    Vector2 jetTip = { nozzle.x + back.x * jetLength, nozzle.y + back.y * jetLength };
    Vector2 jetL   = { nozzle.x - perp.x * jetWidth,  nozzle.y - perp.y * jetWidth };
    Vector2 jetR   = { nozzle.x + perp.x * jetWidth,  nozzle.y + perp.y * jetWidth };
    unsigned char cyanAlpha = (unsigned char)(210 + (int)(boost * 45.0f));
    DrawTriangle(nozzle, jetL, jetTip, (Color){   0, 225, 255, cyanAlpha });
    DrawTriangle(nozzle, jetTip, jetR, (Color){   0, 225, 255, cyanAlpha });

    /* Inner Super-Hot White/Laser Core */
    float innerLen = jetLength * (0.45f + boost * 0.15f);
    float innerWid = jetWidth  * 0.40f;
    Vector2 coreTip = { nozzle.x + back.x * innerLen, nozzle.y + back.y * innerLen };
    Vector2 coreL   = { nozzle.x - perp.x * innerWid, nozzle.y - perp.y * innerWid };
    Vector2 coreR   = { nozzle.x + perp.x * innerWid, nozzle.y + perp.y * innerWid };
    DrawTriangle(nozzle, coreL, coreTip, (Color){ 255, 255, 255, 250 });
    DrawTriangle(nozzle, coreTip, coreR, (Color){ 255, 255, 255, 250 });

    /* 3. Supersonic Mach Shock Diamonds (Flexing nitro afterburner nodes) */
    if (thrust > 0.35f || boost > 0.1f) {
        int numDiamonds = (boost > 0.2f) ? 3 : 2;
        for (int d = 1; d <= numDiamonds; d++) {
            float distAlong = (float)d * (jetLength / (float)(numDiamonds + 1));
            Vector2 dCenter = { nozzle.x + back.x * distAlong, nozzle.y + back.y * distAlong };
            float dSize = (3.5f - (float)d * 0.6f) * (0.8f + boost * 0.5f);
            Vector2 dN = { dCenter.x - back.x * dSize, dCenter.y - back.y * dSize };
            Vector2 dS = { dCenter.x + back.x * dSize, dCenter.y + back.y * dSize };
            Vector2 dE = { dCenter.x + perp.x * dSize * 0.85f, dCenter.y + perp.y * dSize * 0.85f };
            Vector2 dW = { dCenter.x - perp.x * dSize * 0.85f, dCenter.y - perp.y * dSize * 0.85f };
            DrawTriangle(dN, dW, dS, (Color){ 220, 255, 255, 240 });
            DrawTriangle(dN, dS, dE, (Color){ 220, 255, 255, 240 });
        }
    }
}

static void DrawShield(Vector2 pos, float radius, float timer) {
    float pulse = 0.5f + 0.5f * sinf(timer * 4.0f);
    Color inner = { 0, 180, 255, (unsigned char)(60 + pulse * 60) };
    Color outer = { 0, 120, 220, (unsigned char)(30 + pulse * 40) };
    DrawCircleV(pos, radius + 8.0f, outer);
    DrawCircleV(pos, radius + 4.0f, inner);
    DrawCircleLinesV(pos, radius + 8.0f, (Color){100, 220, 255, (unsigned char)(150 + pulse * 105)});
}

static inline Vector2 RotateBankPoint(Vector2 pos, float lx, float ly, float cosR, float sinR, float bank) {
    float bx = lx;
    if (bx < -0.1f) {
        bx *= (bank < 0.0f) ? (1.0f - (-bank) * 0.28f) : (1.0f + bank * 0.10f);
    } else if (bx > 0.1f) {
        bx *= (bank > 0.0f) ? (1.0f - bank * 0.28f) : (1.0f + (-bank) * 0.10f);
    }
    Vector2 res;
    res.x = pos.x + bx * cosR - ly * sinR;
    res.y = pos.y + bx * sinR + ly * cosR;
    return res;
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

    /* Engine plasma jet (drawn behind the ship) */
    DrawEngineGlow(p->pos, p->rotation, p->thrustAnim, p->boostAnim);

    /* ── HK-01 Interceptor Top-Down Pixel-Style Rendering with 2.5D Banking ── */
    float rot = DEG2RAD(p->rotation);
    float cosR = cosf(rot);
    float sinR = sinf(rot);
    float bank = p->bankAngle; /* -1.0 to +1.0 for 3D roll perspective */

    /* Helper macros: rotate local point around ship center */
    #define RPB(lx, ly) RotateBankPoint(p->pos, (float)(lx), (float)(ly), cosR, sinR, bank)
    #define RP(lx, ly)  (Vector2){ p->pos.x + (lx)*cosR - (ly)*sinR, \
                                   p->pos.y + (lx)*sinR + (ly)*cosR }

    /* Color Palette matching reference image HK-01 */
    Color colHullWhite = { 236, 240, 248, 255 }; /* Pristine off-white hull plates */
    Color colHullShade = { 180, 192, 208, 255 }; /* Shaded panel shadow */
    Color colDarkFrame = {  48,  54,  68, 255 }; /* Mechanical panel seams & gun barrels */
    Color colRedTrim   = { 220,  34,  46, 255 }; /* Signature HK-01 Racing Red */
    Color colRedDark   = { 142,  16,  26, 255 }; /* Dark crimson border */
    Color colVentGlow  = { 255, 160,  20, 255 }; /* Glowing amber heatsink radiators */
    Color colVentCore  = { 255, 230,  80, 255 }; /* Radiant heatsink core */
    Color colCockpit   = {   0, 210, 255, 230 }; /* Glossy cyan cockpit canopy */
    Color colCockpitGlint = { 220, 255, 255, 255 }; /* Specular reflection */

    /* Hit flash when taking damage */
    if (p->flashTimer > 0.0f) {
        float frac = p->flashTimer / 0.3f;
        colHullWhite = ColorLerp(colHullWhite, WHITE, frac);
        colRedTrim   = ColorLerp(colRedTrim,   WHITE, frac);
        colDarkFrame = ColorLerp(colDarkFrame, WHITE, frac);
    }

    /* 1. Swept Delta Wings (White base with underside shadow and banking) */
    Vector2 wingL[3] = { RP( 0, -6), RPB(-21,  9), RPB(-6, 14) };
    Vector2 wingR[3] = { RP( 0, -6), RPB( 21,  9), RPB( 6, 14) };
    DrawTriangle(wingL[0], wingL[1], wingL[2], colHullWhite);
    DrawTriangle(wingR[0], wingR[2], wingR[1], colHullWhite);
    /* Wing panel shadow trim */
    DrawLineV(wingL[1], wingL[2], colHullShade);
    DrawLineV(wingR[1], wingR[2], colHullShade);

    /* 2. Bold Crimson Red Wing Leading Edge Trim (Iconic HK-01 Styling) */
    Vector2 redStripeL[3] = { RP( 0, -4), RPB(-21,  9), RPB(-15,  7) };
    Vector2 redStripeR[3] = { RP( 0, -4), RPB( 15,  7), RPB( 21,  9) };
    DrawTriangle(redStripeL[0], redStripeL[1], redStripeL[2], colRedTrim);
    DrawTriangle(redStripeR[0], redStripeR[1], redStripeR[2], colRedTrim);
    DrawLineV(redStripeL[0], redStripeL[1], colRedDark);
    DrawLineV(redStripeR[0], redStripeR[2], colRedDark);

    /* 3. Wingtip Autocannons */
    Vector2 gunL1 = RPB(-20,  9), gunL2 = RPB(-20, -2);
    Vector2 gunR1 = RPB( 20,  9), gunR2 = RPB( 20, -2);
    DrawLineEx(gunL1, gunL2, 2.5f, colDarkFrame);
    DrawLineEx(gunR1, gunR2, 2.5f, colDarkFrame);
    DrawCircleV(gunL2, 1.8f, (Color){ 0, 220, 255, 255 });
    DrawCircleV(gunR2, 1.8f, (Color){ 0, 220, 255, 255 });

    /* 4. Twin Vertical Tail Stabilizer Fins (Tilted during bank roll) */
    float finTilt = bank * 3.5f;
    Vector2 finL[3] = { RPB(-9 + finTilt, 8), RPB(-12 + finTilt, 16), RPB(-7 + finTilt, 16) };
    Vector2 finR[3] = { RPB( 9 + finTilt, 8), RPB(  7 + finTilt, 16), RPB(12 + finTilt, 16) };
    DrawTriangle(finL[0], finL[1], finL[2], colHullWhite);
    DrawTriangle(finR[0], finR[1], finR[2], colHullWhite);
    DrawLineV(finL[1], finL[2], colRedTrim);
    DrawLineV(finR[1], finR[2], colRedTrim);

    /* 5. Central Needle Fuselage (White forward hull) */
    Vector2 noseP[3] = { RP(0, -22), RP(-7, 9), RP(7, 9) };
    DrawTriangle(noseP[0], noseP[1], noseP[2], colHullWhite);
    DrawTriangleLines(noseP[0], noseP[1], noseP[2], colDarkFrame);

    /* Crimson Nose Chevron */
    Vector2 noseRed[3] = { RP(0, -22), RP(-3.5f, -14), RP(3.5f, -14) };
    DrawTriangle(noseRed[0], noseRed[1], noseRed[2], colRedTrim);

    /* 6. Glowing Amber/Orange Heatsink Vents (Left & Right Flanks) */
    float ventPulse = 0.85f + 0.15f * sinf(GetTime() * 14.0f) + p->boostAnim * 0.4f;
    Color ventColor = ColorAlpha(colVentGlow, MIN(1.0f, ventPulse));
    Vector2 ventL1 = RPB(-6, 2), ventL2 = RPB(-6, 7);
    Vector2 ventR1 = RPB( 6, 2), ventR2 = RPB( 6, 7);
    DrawLineEx(ventL1, ventL2, 3.2f + p->boostAnim * 1.2f, ventColor);
    DrawLineEx(ventR1, ventR2, 3.2f + p->boostAnim * 1.2f, ventColor);
    DrawLineEx(ventL1, ventL2, 1.4f, colVentCore);
    DrawLineEx(ventR1, ventR2, 1.4f, colVentCore);

    /* 7. Glossy Cyan Cockpit Canopy with Specular Glint */
    Vector2 cockpitPos = RP(bank * 1.2f, -5);
    DrawCircleV(cockpitPos, 5.2f, colDarkFrame);
    DrawCircleV(cockpitPos, 4.2f, colCockpit);
    /* Specular glass reflection streak */
    DrawLineV(RP(-1.5f + bank, -7.0f), RP(1.5f + bank, -4.5f), colCockpitGlint);
    DrawPixelV(RP(0.5f + bank, -6.5f), WHITE);

    /* 8. Rear Cylindrical Engine Turbine Nozzle */
    Vector2 nozzlePos = RP(0, 13);
    DrawRectanglePro(
        (Rectangle){ nozzlePos.x, nozzlePos.y, 8.0f, 4.0f },
        (Vector2){ 4.0f, 2.0f },
        p->rotation,
        colDarkFrame
    );
    DrawCircleV(RP(0, 14), 3.0f, (Color){ 255, 140, 20, 255 });

    /* 9. Upgraded Weapon Glow Indicator */
    if (p->weapon != WEAPON_SINGLE) {
        float pulse = 0.6f + 0.4f * sinf(GetTime() * 12.0f);
        Color wc;
        if (p->weapon == WEAPON_RAPID) {
            wc = (Color){255, 100, 0, (unsigned char)(140 * pulse)};
        } else if (p->weapon == WEAPON_SPREAD) {
            wc = (Color){255, 220, 0, (unsigned char)(140 * pulse)};
        } else {
            wc = (Color){0, 255, 180, (unsigned char)(140 * pulse)};
        }
        DrawCircleV(RP(-19, 0), 4.0f, wc);
        DrawCircleV(RP( 19, 0), 4.0f, wc);
    }

    /* 10. Visual Tier Hull Upgrades (Mk II, Mk III, Mk IV) */
    if (p->shipTier >= SHIP_TIER_MK2) {
        /* Mk II: Reinforced Titanium Prow Plating & Flank Sponsons */
        Vector2 armorL[3] = { RP(-4.0f, -14.0f), RPB(-9.0f, -2.0f), RP(-4.0f, 2.0f) };
        Vector2 armorR[3] = { RP( 4.0f, -14.0f), RPB( 9.0f, -2.0f), RP( 4.0f, 2.0f) };
        DrawTriangle(armorL[0], armorL[1], armorL[2], (Color){ 70, 80, 100, 255 });
        DrawTriangle(armorR[0], armorR[2], armorR[1], (Color){ 70, 80, 100, 255 });
        DrawLineV(armorL[0], armorL[1], (Color){ 140, 160, 190, 255 });
        DrawLineV(armorR[0], armorR[1], (Color){ 140, 160, 190, 255 });
    }

    if (p->shipTier >= SHIP_TIER_MK3) {
        /* Mk III: Glowing Cyan Plasma Conduits etched into the swept wings */
        float plasmaPulse = 0.7f + 0.3f * sinf(GetTime() * 12.0f);
        Color conduitCol = (Color){ 0, 240, 255, (unsigned char)(220 * plasmaPulse) };
        DrawLineEx(RP(0, -6), RPB(-16, 8), 1.8f, conduitCol);
        DrawLineEx(RP(0, -6), RPB( 16, 8), 1.8f, conduitCol);
        DrawCircleV(RPB(-20, -2), 2.5f, (Color){ 0, 255, 220, 255 });
        DrawCircleV(RPB( 20, -2), 2.5f, (Color){ 0, 255, 220, 255 });
    }

    if (p->shipTier >= SHIP_TIER_MK4) {
        /* Mk IV: Ascendant Royal Gold Trims & Particle Energy Radiance */
        float goldPulse = 0.8f + 0.2f * sinf(GetTime() * 8.0f);
        Color goldCol = (Color){ 255, 215, 0, (unsigned char)(230 * goldPulse) };
        DrawLineEx(RP(0, -22), RPB(-21, 9), 2.2f, goldCol);
        DrawLineEx(RP(0, -22), RPB( 21, 9), 2.2f, goldCol);
        DrawCircleV(RP(0, -5), 3.0f, goldCol);
    }

    #undef RP
    #undef RPB
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

    int mouseFire = 0;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mpos = GetMousePosition();
        int sw = GetScreenWidth();
        /* Do not fire if clicking HUD buttons in top right */
        if (!(mpos.x > (float)(sw - 260) && mpos.y < 65.0f)) {
            mouseFire = 1;
        }
    }

    if (!IsKeyDown(KEY_SPACE) && !IsKeyDown(KEY_J) && !IsKeyDown(KEY_Z) && !mouseFire) return 0;

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

