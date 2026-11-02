/**
 * enemy.c - Enemy AI, rendering, spawning
 */
#include "enemy.h"
#include "utils.h"
#include "raylib.h"
#include <math.h>
#include <string.h>

/* ─── Type metadata lookup ───────────────────────────── */
typedef struct { float speed; int health; float radius; int score; } EnemyMeta;

static const EnemyMeta kMeta[ENEMY_TYPE_COUNT] = {
    [ENEMY_NONE]        = {0,               0,                  0,             0},
    [ENEMY_SCOUT]       = {SCOUT_SPEED,      SCOUT_HEALTH,       SCOUT_RADIUS,  SCOUT_SCORE},
    [ENEMY_INTERCEPTOR] = {INTERCEPTOR_SPEED,INTERCEPTOR_HEALTH, INTERCEPTOR_RADIUS,INTERCEPTOR_SCORE},
    [ENEMY_TANK]        = {TANK_SPEED,       TANK_HEALTH,        TANK_RADIUS,   TANK_SCORE},
    [ENEMY_BOMBER]      = {BOMBER_SPEED,     BOMBER_HEALTH,      BOMBER_RADIUS, BOMBER_SCORE},
    [ENEMY_BOSS]        = {BOSS_SPEED,       BOSS_HEALTH,        BOSS_RADIUS,   BOSS_SCORE},
};

/* ─── Pool management ────────────────────────────────── */

void Enemies_Init(EnemyPool *pool) {
    memset(pool, 0, sizeof(*pool));
}

void Enemies_Clear(EnemyPool *pool) {
    for (int i = 0; i < MAX_ENEMIES; i++) pool->enemies[i].active = 0;
    pool->count = 0;
}

static int FindFreeEnemy(EnemyPool *pool) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!pool->enemies[i].active) return i;
    }
    return -1;
}

/* ─── Spawn ──────────────────────────────────────────── */

static void InitEnemy(Enemy *e, EnemyType type, Vector2 pos) {
    memset(e, 0, sizeof(*e));
    e->type       = type;
    e->pos        = pos;
    e->health     = kMeta[type].health;
    e->healthMax  = e->health;
    e->radius     = kMeta[type].radius;
    e->score      = kMeta[type].score;
    e->active     = 1;
    e->state      = ESTATE_APPROACH;
    e->strafeDir  = (GetRandomValue(0,1) == 0) ? 1.0f : -1.0f;
    e->stateTimer = RandFloat(1.5f, 4.0f);
    e->fireTimer  = RandFloat(0.5f, 2.0f);
}

static Vector2 RandomEdgePos(int screenW, int screenH) {
    int edge = GetRandomValue(0, 3);
    float m  = ENEMY_SPAWN_MARGIN;
    switch (edge) {
        case 0: return (Vector2){RandFloat(0, (float)screenW), -m};
        case 1: return (Vector2){(float)screenW + m, RandFloat(0, (float)screenH)};
        case 2: return (Vector2){RandFloat(0, (float)screenW), (float)screenH + m};
        default: return (Vector2){-m, RandFloat(0, (float)screenH)};
    }
}

int Enemy_Spawn(EnemyPool *pool, EnemyType type, int screenW, int screenH) {
    int idx = FindFreeEnemy(pool);
    if (idx < 0) return 0;

    Vector2 pos = RandomEdgePos(screenW, screenH);
    InitEnemy(&pool->enemies[idx], type, pos);
    pool->count++;
    return 1;
}

void Enemy_SpawnBoss(EnemyPool *pool, int screenW, int screenH) {
    (void)screenH;
    int idx = FindFreeEnemy(pool);
    if (idx < 0) return;

    Enemy *e = &pool->enemies[idx];
    Vector2 pos = { (float)screenW * 0.5f, -(float)BOSS_RADIUS - 20.0f };
    InitEnemy(e, ENEMY_BOSS, pos);
    e->bossPhase = 1;
    e->bossMovAngle = 0.0f;
    pool->count++;
}

int Enemies_FindBoss(const EnemyPool *pool) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (pool->enemies[i].active && pool->enemies[i].type == ENEMY_BOSS)
            return i;
    }
    return -1;
}

/* ─── Damage ─────────────────────────────────────────── */

void Enemy_TakeDamage(Enemy *e, int dmg) {
    e->health -= dmg;
    if (e->health <= 0) {
        e->health = 0;
        e->state  = ESTATE_DYING;
        e->deathTimer = 0.0f;
    }
}

/* ─── AI helpers ─────────────────────────────────────── */

static void ScoutAI(Enemy *e, Vector2 playerPos, float dt) {
    /* Scouts (Synod Dagger Interceptors): Fast dogfighters that keep standoff
       distance, strafe, and peel away so they NEVER ram into the player */
    float dist = Vec2Dist(e->pos, playerPos);
    Vector2 dir = Vec2Norm(Vec2Sub(playerPos, e->pos));

    if (dist < 190.0f) {
        /* Too close: break off / peel away sharply to prevent hitting player */
        Vector2 escapeDir = { -dir.y * e->strafeDir, dir.x * e->strafeDir };
        e->vel.x += (escapeDir.x * 0.8f - dir.x * 0.6f) * SCOUT_SPEED * 2.2f * dt;
        e->vel.y += (escapeDir.y * 0.8f - dir.y * 0.6f) * SCOUT_SPEED * 2.2f * dt;
    } else if (dist > 270.0f) {
        /* Close distance to attack range */
        e->vel.x += dir.x * SCOUT_SPEED * 1.5f * dt;
        e->vel.y += dir.y * SCOUT_SPEED * 1.5f * dt;
    } else {
        /* Circle / dogfight at standoff range */
        Vector2 perp = { -dir.y * e->strafeDir, dir.x * e->strafeDir };
        e->vel.x += perp.x * SCOUT_SPEED * 1.3f * dt;
        e->vel.y += perp.y * SCOUT_SPEED * 1.3f * dt;
    }

    float s = Vec2Len(e->vel);
    if (s > SCOUT_SPEED * 1.3f) e->vel = Vec2Scale(Vec2Norm(e->vel), SCOUT_SPEED * 1.3f);

    /* Aim at player to fire cannons */
    float targetAngle = RAD2DEG(atan2f(dir.x, -dir.y));
    float diff = targetAngle - e->rotation;
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    e->rotation += diff * MIN(1.0f, dt * 5.0f);
}

static void InterceptorAI(Enemy *e, Vector2 playerPos, float dt) {
    /* Interceptors (Synod Jackal Heavy Fighters): Tactical flankers that maintain
       combat distance and reverse thrusters when the player flies near */
    float dist = Vec2Dist(e->pos, playerPos);
    Vector2 dir = Vec2Norm(Vec2Sub(playerPos, e->pos));

    e->stateTimer -= dt;
    if (e->stateTimer <= 0.0f) {
        e->stateTimer = RandFloat(2.0f, 4.0f);
        e->strafeDir = -e->strafeDir;
    }

    if (dist < 190.0f) {
        /* Active reverse thrust to prevent ramming */
        e->vel.x -= dir.x * INTERCEPTOR_SPEED * 1.8f * dt;
        e->vel.y -= dir.y * INTERCEPTOR_SPEED * 1.8f * dt;
    } else if (dist > 320.0f) {
        /* Approach to engagement envelope */
        e->vel.x += dir.x * INTERCEPTOR_SPEED * dt;
        e->vel.y += dir.y * INTERCEPTOR_SPEED * dt;
    } else {
        /* Flanking strafe */
        Vector2 perp = { -dir.y * e->strafeDir, dir.x * e->strafeDir };
        e->vel.x += perp.x * INTERCEPTOR_SPEED * 1.2f * dt;
        e->vel.y += perp.y * INTERCEPTOR_SPEED * 1.2f * dt;
    }

    float s = Vec2Len(e->vel);
    if (s > INTERCEPTOR_SPEED) e->vel = Vec2Scale(Vec2Norm(e->vel), INTERCEPTOR_SPEED);

    /* Track player */
    float targetAngle = RAD2DEG(atan2f(dir.x, -dir.y));
    float diff = targetAngle - e->rotation;
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    e->rotation += diff * MIN(1.0f, dt * 3.5f);
}

static void TankAI(Enemy *e, Vector2 playerPos, float dt) {
    /* Tanks (Synod Bulwark Ironclads): Heavy armored fortresses.
       Stands off at long range and rotates heavy turrets, never rams */
    float dist = Vec2Dist(e->pos, playerPos);
    Vector2 dir = Vec2Norm(Vec2Sub(playerPos, e->pos));

    if (dist > 360.0f) {
        /* Slow advance */
        e->vel.x += dir.x * TANK_SPEED * dt;
        e->vel.y += dir.y * TANK_SPEED * dt;
    } else if (dist < 260.0f) {
        /* Too close: back off to keep firing line */
        e->vel.x -= dir.x * TANK_SPEED * 1.4f * dt;
        e->vel.y -= dir.y * TANK_SPEED * 1.4f * dt;
    } else {
        /* Slow lateral orbit */
        Vector2 perp = { -dir.y * e->strafeDir, dir.x * e->strafeDir };
        e->vel.x += perp.x * TANK_SPEED * 0.6f * dt;
        e->vel.y += perp.y * TANK_SPEED * 0.6f * dt;
    }

    float s = Vec2Len(e->vel);
    if (s > TANK_SPEED) e->vel = Vec2Scale(Vec2Norm(e->vel), TANK_SPEED);

    /* Aim heavy turret at player */
    float targetAngle = RAD2DEG(atan2f(dir.x, -dir.y));
    float diff = targetAngle - e->rotation;
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    e->rotation += diff * MIN(1.0f, dt * 2.5f);
}

static void BomberAI(Enemy *e, Vector2 playerPos, float dt) {
    /* Maintain preferred distance */
    float dist = Vec2Dist(e->pos, playerPos);
    float prefDist = BOMBER_PREFERRED_DIST;

    Vector2 dir = Vec2Norm(Vec2Sub(playerPos, e->pos));
    if (dist < prefDist - 40.0f) {
        /* Too close — retreat */
        e->vel.x -= dir.x * BOMBER_SPEED * dt;
        e->vel.y -= dir.y * BOMBER_SPEED * dt;
    } else if (dist > prefDist + 40.0f) {
        /* Too far — approach */
        e->vel.x += dir.x * BOMBER_SPEED * dt;
        e->vel.y += dir.y * BOMBER_SPEED * dt;
    } else {
        /* Orbit */
        Vector2 perp = { -dir.y * e->strafeDir, dir.x * e->strafeDir };
        e->vel.x += perp.x * BOMBER_SPEED * 0.7f * dt;
        e->vel.y += perp.y * BOMBER_SPEED * 0.7f * dt;
    }
    float s = Vec2Len(e->vel);
    if (s > BOMBER_SPEED) e->vel = Vec2Scale(Vec2Norm(e->vel), BOMBER_SPEED);

    /* Face player */
    float targetAngle = RAD2DEG(atan2f(dir.x, -dir.y));
    float diff = targetAngle - e->rotation;
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    e->rotation += diff * MIN(1.0f, dt * 3.5f);
}

static void BossAI(Enemy *e, Vector2 playerPos, int screenW, int screenH, float dt) {
    /* Phase 2 at 50% HP */
    float hpFrac = (float)e->health / (float)e->healthMax;
    e->bossPhase = (hpFrac < BOSS_PHASE2_HP_THRESHOLD) ? 2 : 1;

    /* Oscillate across the top portion of the screen */
    e->bossMovAngle += dt * (e->bossPhase == 2 ? 1.4f : 0.8f);
    float targetX = (float)screenW * 0.5f + cosf(e->bossMovAngle) * (float)screenW * 0.35f;
    float targetY = (float)screenH * 0.18f + sinf(e->bossMovAngle * 0.5f) * (float)screenH * 0.08f;

    Vector2 target = { targetX, targetY };
    Vector2 dir = Vec2Norm(Vec2Sub(target, e->pos));
    float speed = BOSS_SPEED * (e->bossPhase == 2 ? 1.5f : 1.0f);
    e->vel.x += dir.x * speed * 3.0f * dt;
    e->vel.y += dir.y * speed * 3.0f * dt;
    float s = Vec2Len(e->vel);
    if (s > speed) e->vel = Vec2Scale(Vec2Norm(e->vel), speed);

    /* Face player */
    Vector2 toPlayer = Vec2Norm(Vec2Sub(playerPos, e->pos));
    float targetAngle = RAD2DEG(atan2f(toPlayer.x, -toPlayer.y));
    float diff = targetAngle - e->rotation;
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    e->rotation += diff * MIN(1.0f, dt * 2.0f);
}

/* ─── Update ─────────────────────────────────────────── */

void Enemies_Update(EnemyPool *pool, Vector2 playerPos, float dt,
                    int screenW, int screenH) {
    pool->count = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &pool->enemies[i];
        if (!e->active) continue;
        pool->count++;

        if (e->state == ESTATE_DYING) {
            e->deathTimer += dt;
            if (e->deathTimer > 0.5f) {
                e->active = 0;
                pool->count--;
            }
            continue;
        }

        /* AI movement */
        switch (e->type) {
            case ENEMY_SCOUT:       ScoutAI(e, playerPos, dt);                       break;
            case ENEMY_INTERCEPTOR: InterceptorAI(e, playerPos, dt);                 break;
            case ENEMY_TANK:        TankAI(e, playerPos, dt);                        break;
            case ENEMY_BOMBER:      BomberAI(e, playerPos, dt);                      break;
            case ENEMY_BOSS:        BossAI(e, playerPos, screenW, screenH, dt);     break;
            default: break;
        }

        /* Apply velocity */
        e->pos.x += e->vel.x * dt;
        e->pos.y += e->vel.y * dt;

        /* Light drag (frame-rate independent) */
        float drag = powf(0.97f, dt * 60.0f);
        e->vel.x *= drag;
        e->vel.y *= drag;

        /* Rotate ship sprite toward direction of travel */
        if (e->type != ENEMY_BOSS) {
            float speed = Vec2Len(e->vel);
            if (speed > 10.0f) {
                float travelAngle = RAD2DEG(atan2f(e->vel.x, -e->vel.y));
                float diff = travelAngle - e->rotation;
                while (diff >  180.0f) diff -= 360.0f;
                while (diff < -180.0f) diff += 360.0f;
                /* Blend visual rotation toward travel for scouts */
                if (e->type == ENEMY_SCOUT)
                    e->rotation += diff * MIN(1.0f, dt * 6.0f);
            }
        }

        /* Fire timer (handled in collision module for spawn access) */
        e->fireTimer -= dt;
    }
}

/* ─── Drawing ────────────────────────────────────────── */

/* Synod Faction Color Palette: Industrial Gunmetal, Cold Slate, Blood Crimson */
static const Color kSynodHullDark  = {  36,  42,  52, 255 }; /* Heavy battleship steel */
static const Color kSynodHullMid   = {  56,  64,  78, 255 }; /* Armor plates */
static const Color kSynodHullLight = {  88,  98, 116, 255 }; /* Panel highlights */
static const Color kSynodRed       = { 220,  30,  40, 255 }; /* Imperial red accents */
static const Color kSynodRedGlow   = { 255,  50,  60, 255 }; /* Glowing sensor visors */
static const Color kSynodRedDark   = { 120,  15,  22, 255 }; /* Crimson borders */

static void DrawScout(const Enemy *e) {
    /* Synod "Dagger" Interceptor — Fast, predatory chevron dagger */
    float rot = DEG2RAD(e->rotation);
    float c = cosf(rot), s = sinf(rot);
    #define RP(lx, ly) (Vector2){ e->pos.x + (lx)*c - (ly)*s, e->pos.y + (lx)*s + (ly)*c }

    /* Dual rear ion thrusters (red glow) */
    Vector2 tL = RP(-5, -9), tR = RP(5, -9);
    DrawCircleV(tL, 2.5f, kSynodRedGlow);
    DrawCircleV(tR, 2.5f, kSynodRedGlow);
    DrawCircleV(tL, 1.2f, WHITE);
    DrawCircleV(tR, 1.2f, WHITE);

    /* Main Dagger Hull (Pointed forward towards tip at (0, 14)) */
    Vector2 tip = RP(0, 14);
    Vector2 wingL = RP(-12, -7), wingR = RP(12, -7);
    Vector2 notch = RP(0, -3);

    /* Left and right swept wings */
    DrawTriangle(wingL, tip, notch, kSynodHullMid);
    DrawTriangle(notch, tip, wingR, kSynodHullMid);
    DrawTriangleLines(wingL, tip, notch, kSynodHullDark);
    DrawTriangleLines(notch, tip, wingR, kSynodHullDark);

    /* Crimson Leading Edge Plates */
    DrawLineEx(wingL, tip, 2.2f, kSynodRed);
    DrawLineEx(wingR, tip, 2.2f, kSynodRed);

    /* Central Red Optical Sensor Visor (Glowing slit) */
    Vector2 eye1 = RP(-3.5f, 5), eye2 = RP(3.5f, 5);
    DrawLineEx(eye1, eye2, 2.0f, kSynodRedGlow);
    DrawPixelV(RP(0, 5), WHITE);

    #undef RP
}

static void DrawInterceptor(const Enemy *e) {
    /* Synod "Jackal" Heavy Fighter — Twin-boom split fighter with dual cannons */
    float rot = DEG2RAD(e->rotation);
    float c = cosf(rot), s = sinf(rot);
    #define RP(lx, ly) (Vector2){ e->pos.x + (lx)*c - (ly)*s, e->pos.y + (lx)*s + (ly)*c }

    /* Left and right weapon sponson booms */
    Vector2 boomL1 = RP(-12, -8), boomL2 = RP(-12, 12);
    Vector2 boomR1 = RP( 12, -8), boomR2 = RP( 12, 12);
    DrawLineEx(boomL1, boomL2, 4.0f, kSynodHullDark);
    DrawLineEx(boomR1, boomR2, 4.0f, kSynodHullDark);
    /* Crimson blaster tips */
    DrawCircleV(boomL2, 2.5f, kSynodRed);
    DrawCircleV(boomR2, 2.5f, kSynodRed);

    /* Central Armored Fuselage */
    Vector2 body[4] = { RP(0, 14), RP(-8, -4), RP(0, -10), RP(8, -4) };
    DrawTriangle(body[1], body[0], body[3], kSynodHullMid);
    DrawTriangle(body[1], body[3], body[2], kSynodHullDark);
    DrawTriangleLines(body[1], body[0], body[3], kSynodRedDark);

    /* Connecting wing spars */
    DrawLineEx(RP(-12, 0), RP(12, 0), 3.0f, kSynodHullLight);

    /* Hexagonal red cockpit sensor */
    DrawCircleV(RP(0, 3), 3.8f, kSynodRedGlow);
    DrawCircleV(RP(0, 3), 1.8f, WHITE);

    #undef RP
}

static void DrawTank(const Enemy *e) {
    /* Synod "Bulwark" Ironclad — Heavy armored hexagonal fortress */
    float rot = DEG2RAD(e->rotation);
    float c = cosf(rot), s = sinf(rot);
    #define RP(lx, ly) (Vector2){ e->pos.x + (lx)*c - (ly)*s, e->pos.y + (lx)*s + (ly)*c }

    /* Multi-plated heavy hexagonal hull */
    Vector2 hex[6] = {
        RP(  0,  24), RP(-20,  12), RP(-20, -12),
        RP(  0, -24), RP( 20, -12), RP( 20,  12)
    };
    for (int t = 1; t < 5; t++) {
        DrawTriangle(hex[0], hex[t], hex[t+1], kSynodHullMid);
        DrawLineEx(hex[t], hex[t+1], 2.0f, kSynodHullLight);
    }
    DrawLineEx(hex[0], hex[1], 2.5f, kSynodRed);
    DrawLineEx(hex[0], hex[5], 2.5f, kSynodRed);

    /* Heavy Reinforced Prow Armor Plate */
    Vector2 prow[3] = { RP(0, 24), RP(-12, 10), RP(12, 10) };
    DrawTriangle(prow[0], prow[1], prow[2], kSynodHullDark);
    DrawTriangleLines(prow[0], prow[1], prow[2], kSynodRed);

    /* Heavy Dual-Turret Turbolaser on Top */
    Vector2 turretPos = RP(0, 0);
    DrawCircleV(turretPos, 9.0f, kSynodHullDark);
    DrawCircleLinesV(turretPos, 9.0f, kSynodHullLight);
    /* Twin heavy cannon barrels pointing forward */
    DrawLineEx(RP(-4, 0), RP(-4, 16), 2.5f, kSynodHullLight);
    DrawLineEx(RP( 4, 0), RP( 4, 16), 2.5f, kSynodHullLight);
    DrawCircleV(turretPos, 4.0f, kSynodRed);

    #undef RP
}

static void DrawBomber(const Enemy *e) {
    /* Synod "Goliath" Siege Platform — Heavy ordnance carrier with pulsing racks */
    float rot = DEG2RAD(e->rotation);
    float c = cosf(rot), s = sinf(rot);
    #define RP(lx, ly) (Vector2){ e->pos.x + (lx)*c - (ly)*s, e->pos.y + (lx)*s + (ly)*c }

    /* Heavy delta fuselage */
    Vector2 body[3] = { RP(0, 18), RP(-16, -14), RP(16, -14) };
    DrawTriangle(body[0], body[1], body[2], kSynodHullMid);
    DrawTriangleLines(body[0], body[1], body[2], kSynodHullDark);

    /* Massive Port and Starboard Missile Launch Pods */
    Vector2 podL[3] = { RP(-16, -14), RP(-30, 2), RP(-10, 2) };
    Vector2 podR[3] = { RP( 16, -14), RP( 10, 2), RP( 30, 2) };
    DrawTriangle(podL[0], podL[1], podL[2], kSynodHullDark);
    DrawTriangle(podR[0], podR[1], podR[2], kSynodHullDark);
    DrawTriangleLines(podL[0], podL[1], podL[2], kSynodRedDark);
    DrawTriangleLines(podR[0], podR[1], podR[2], kSynodRedDark);

    /* Pulsing Ordnance Silo Bays */
    float pulse = 0.5f + 0.5f * sinf(GetTime() * 8.0f);
    Color bayCol = (Color){ 255, (unsigned char)(100 + pulse * 100), 20, 255 };
    DrawCircleV(RP(-18, -4), 4.5f, bayCol);
    DrawCircleV(RP( 18, -4), 4.5f, bayCol);
    DrawCircleV(RP(-18, -4), 2.0f, WHITE);
    DrawCircleV(RP( 18, -4), 2.0f, WHITE);

    /* Bridge Command Visor */
    DrawLineEx(RP(-4, 8), RP(4, 8), 2.5f, kSynodRedGlow);

    #undef RP
}

static void DrawBoss(const Enemy *e) {
    /* Synod Dreadnought "Vindicator" — Monolithic Star Destroyer Capital Ship */
    float rot = DEG2RAD(e->rotation);
    float c = cosf(rot), s = sinf(rot);
    #define RP(lx, ly) (Vector2){ e->pos.x + (lx)*c - (ly)*s, e->pos.y + (lx)*s + (ly)*c }

    int phase = e->bossPhase;
    float pulse = 0.5f + 0.5f * sinf((float)GetTime() * (phase == 2 ? 8.0f : 4.0f));
    float r = e->radius;

    /* 1. Deflector Shield Aura */
    Color shieldColor = (phase == 2) ?
        (Color){ 255,  40,  20, (unsigned char)(60 + pulse * 90) } :
        (Color){ 180,  20, 220, (unsigned char)(40 + pulse * 60) };
    DrawCircleV(e->pos, r + 14.0f * pulse, shieldColor);

    /* 2. Massive Wedge-Shaped Dreadnought Hull (Arrowhead Prow forward at (0, r)) */
    Vector2 prowTip = RP(0, r);
    Vector2 flankL  = RP(-r * 0.95f, -r * 0.85f);
    Vector2 flankR  = RP( r * 0.95f, -r * 0.85f);
    Vector2 stern   = RP(0, -r * 0.65f);

    /* Port and Starboard Armor Wedges */
    DrawTriangle(prowTip, flankL, stern, kSynodHullMid);
    DrawTriangle(prowTip, stern, flankR, kSynodHullDark);
    DrawLineEx(prowTip, flankL, 3.0f, kSynodHullLight);
    DrawLineEx(prowTip, flankR, 3.0f, kSynodHullLight);
    DrawLineEx(flankL, stern, 2.0f, kSynodRedDark);
    DrawLineEx(flankR, stern, 2.0f, kSynodRedDark);

    /* Center Spine Ridge */
    DrawLineEx(prowTip, stern, 2.5f, kSynodHullLight);

    /* 3. Multi-Tiered Command Bridge Superstructure */
    Vector2 bridgeL = RP(-r * 0.35f, -r * 0.40f);
    Vector2 bridgeR = RP( r * 0.35f, -r * 0.40f);
    Vector2 bridgeF = RP(0, -r * 0.05f);
    DrawTriangle(bridgeF, bridgeL, bridgeR, kSynodHullDark);
    DrawTriangleLines(bridgeF, bridgeL, bridgeR, kSynodRed);

    /* Glowing Command Bridge Viewport (Horizontal crimson strip) */
    DrawLineEx(RP(-r * 0.22f, -r * 0.25f), RP(r * 0.22f, -r * 0.25f), 3.0f, kSynodRedGlow);

    /* 4. Heavy Flank Turbolaser Batteries (4 battery turrets along trenches) */
    for (int k = 1; k <= 3; k++) {
        float fFrac = (float)k * 0.22f;
        Vector2 batL = RP(-r * (0.80f - fFrac * 0.6f), -r * 0.70f + fFrac * r * 1.2f);
        Vector2 batR = RP( r * (0.80f - fFrac * 0.6f), -r * 0.70f + fFrac * r * 1.2f);
        DrawCircleV(batL, 4.0f, kSynodHullDark);
        DrawCircleV(batR, 4.0f, kSynodHullDark);
        DrawCircleV(batL, 2.0f, kSynodRed);
        DrawCircleV(batR, 2.0f, kSynodRed);
    }

    /* 5. Exposed Hyper-Plasma Reactor Core (Phase 2 Critical Breach!) */
    if (phase == 2) {
        Color coreGlow = (Color){ 255, 60, 20, (unsigned char)(160 + pulse * 95) };
        DrawCircleV(e->pos, r * 0.30f, coreGlow);
        DrawCircleV(e->pos, r * 0.16f, WHITE);
        DrawCircleLinesV(e->pos, r * 0.30f, (Color){ 255, 200, 50, 255 });
    }

    /* 6. Triple Heavy Ion Engines at Stern */
    for (int eIdx = -1; eIdx <= 1; eIdx++) {
        Vector2 engPos = RP((float)eIdx * r * 0.35f, -r * 0.85f);
        DrawCircleV(engPos, 5.5f, kSynodHullDark);
        DrawCircleV(engPos, 3.5f, kSynodRedGlow);
        DrawCircleV(engPos, 1.5f, WHITE);
    }

    #undef RP
}

void DrawEnemy(const Enemy *e) {
    switch (e->type) {
        case ENEMY_SCOUT:       DrawScout(e);       break;
        case ENEMY_INTERCEPTOR: DrawInterceptor(e); break;
        case ENEMY_TANK:        DrawTank(e);        break;
        case ENEMY_BOMBER:      DrawBomber(e);      break;
        case ENEMY_BOSS:        DrawBoss(e);        break;
        default: break;
    }

    /* Health bar for non-scouts above threshold */
    if (e->type != ENEMY_SCOUT && e->healthMax > 0) {
        float frac = (float)e->health / (float)e->healthMax;
        float bw = e->radius * 2.2f;
        float bh = 4.0f;
        float bx = e->pos.x - bw * 0.5f;
        float by = e->pos.y - e->radius - 10.0f;
        DrawRectangle((int)bx, (int)by, (int)bw, (int)bh, (Color){60, 0, 0, 200});
        DrawRectangle((int)bx, (int)by, (int)(bw * frac), (int)bh,
                      frac > 0.5f ? (Color){0, 200, 80, 255} : (Color){255, 80, 0, 255});
    }
}

void Enemies_Draw(const EnemyPool *pool) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy *e = &pool->enemies[i];
        if (!e->active) continue;
        if (e->state == ESTATE_DYING) continue; /* particles handle this */
        DrawEnemy(e);
    }
}

