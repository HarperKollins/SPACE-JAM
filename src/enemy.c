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

static void MoveToward(Enemy *e, Vector2 target, float speed, float dt) {
    Vector2 dir = Vec2Norm(Vec2Sub(target, e->pos));
    float targetAngle = RAD2DEG(atan2f(dir.x, -dir.y));
    /* Smooth rotation */
    float diff = targetAngle - e->rotation;
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    e->rotation += diff * MIN(1.0f, dt * 4.0f);

    e->vel.x += dir.x * speed * dt;
    e->vel.y += dir.y * speed * dt;
    float maxSpd = kMeta[e->type].speed;
    float s = Vec2Len(e->vel);
    if (s > maxSpd) { e->vel = Vec2Scale(Vec2Norm(e->vel), maxSpd); }
}

static void ScoutAI(Enemy *e, Vector2 playerPos, float dt) {
    /* Scouts fly straight at the player, fast */
    MoveToward(e, playerPos, SCOUT_SPEED * 2.0f, dt);
}

static void InterceptorAI(Enemy *e, Vector2 playerPos, float dt) {
    e->stateTimer -= dt;
    if (e->stateTimer <= 0.0f) {
        e->state = (e->state == ESTATE_APPROACH) ? ESTATE_STRAFE : ESTATE_APPROACH;
        e->stateTimer = RandFloat(1.0f, 3.0f);
        e->strafeDir = -e->strafeDir;
    }
    if (e->state == ESTATE_APPROACH) {
        MoveToward(e, playerPos, INTERCEPTOR_SPEED, dt);
    } else {
        /* Strafe: perpendicular to player */
        Vector2 dir = Vec2Norm(Vec2Sub(playerPos, e->pos));
        Vector2 perp = { -dir.y * e->strafeDir, dir.x * e->strafeDir };
        e->vel.x += perp.x * INTERCEPTOR_SPEED * dt;
        e->vel.y += perp.y * INTERCEPTOR_SPEED * dt;
        float s = Vec2Len(e->vel);
        if (s > INTERCEPTOR_SPEED) e->vel = Vec2Scale(Vec2Norm(e->vel), INTERCEPTOR_SPEED);
        float targetAngle = RAD2DEG(atan2f(dir.x, -dir.y));
        float diff = targetAngle - e->rotation;
        while (diff >  180.0f) diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;
        e->rotation += diff * MIN(1.0f, dt * 3.0f);
    }
}

static void TankAI(Enemy *e, Vector2 playerPos, float dt) {
    MoveToward(e, playerPos, TANK_SPEED, dt);
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

static void DrawScout(const Enemy *e) {
    float rot = DEG2RAD(e->rotation);
    float c = cosf(rot), s = sinf(rot);
    #define RP(lx, ly) (Vector2){ e->pos.x + (lx)*c - (ly)*s, e->pos.y + (lx)*s + (ly)*c }

    Color hull = (Color){ 200, 60, 60, 255 };
    Color edge = (Color){ 255, 120, 80, 255 };
    Vector2 tip = RP(0, 12);  /* scouts face downward toward player */
    Vector2 lw  = RP(-10, -8);
    Vector2 rw  = RP( 10, -8);
    DrawTriangle(lw, tip, rw, hull);
    DrawTriangleLines(lw, tip, rw, edge);
    DrawCircleV(RP(0, 2), 3.0f, (Color){255, 80, 80, 200});
    #undef RP
}

static void DrawInterceptor(const Enemy *e) {
    float rot = DEG2RAD(e->rotation);
    float c = cosf(rot), s = sinf(rot);
    #define RP(lx, ly) (Vector2){ e->pos.x + (lx)*c - (ly)*s, e->pos.y + (lx)*s + (ly)*c }

    Color hull = (Color){ 180, 50, 180, 255 };
    Color edge = (Color){ 255, 100, 255, 255 };
    Vector2 pts[4] = { RP(0, 16), RP(-14, -6), RP(0, 0), RP(14, -6) };
    DrawTriangle(pts[1], pts[0], pts[2], hull);
    DrawTriangle(pts[2], pts[0], pts[3], hull);
    DrawTriangleLines(pts[1], pts[0], pts[2], edge);
    DrawTriangleLines(pts[2], pts[0], pts[3], edge);
    DrawCircleV(RP(0, 6), 4.5f, (Color){255, 80, 255, 200});
    #undef RP
}

static void DrawTank(const Enemy *e) {
    float rot = DEG2RAD(e->rotation);
    float c = cosf(rot), s = sinf(rot);
    #define RP(lx, ly) (Vector2){ e->pos.x + (lx)*c - (ly)*s, e->pos.y + (lx)*s + (ly)*c }

    Color hull  = (Color){ 60, 160, 60, 255 };
    Color edge  = (Color){ 100, 230, 100, 255 };
    /* Hexagon-ish tank shape */
    Vector2 hex[6] = {
        RP(0, 22), RP(-18, 10), RP(-18, -10),
        RP(0, -22), RP(18, -10), RP(18, 10)
    };
    for (int t = 1; t < 5; t++) {
        DrawTriangle(hex[0], hex[t], hex[t+1], hull);
        DrawLineV(hex[t], hex[t+1], edge);
    }
    DrawLineV(hex[0], hex[1], edge);
    DrawLineV(hex[0], hex[5], edge);
    DrawCircleV(e->pos, 8.0f, (Color){80, 200, 80, 200});
    #undef RP
}

static void DrawBomber(const Enemy *e) {
    float rot = DEG2RAD(e->rotation);
    float c = cosf(rot), s = sinf(rot);
    #define RP(lx, ly) (Vector2){ e->pos.x + (lx)*c - (ly)*s, e->pos.y + (lx)*s + (ly)*c }

    Color hull = (Color){ 200, 120, 20, 255 };
    Color edge = (Color){ 255, 180, 60, 255 };
    Vector2 body[3] = { RP(0, 16), RP(-16, -12), RP(16, -12) };
    Vector2 wingL[3] = { RP(-16, -12), RP(-28, 4), RP(-8, 4) };
    Vector2 wingR[3] = { RP(16, -12), RP(8, 4), RP(28, 4) };
    DrawTriangle(body[0], body[1], body[2], hull);
    DrawTriangle(wingL[0], wingL[1], wingL[2], (Color){160, 90, 10, 220});
    DrawTriangle(wingR[1], wingR[0], wingR[2], (Color){160, 90, 10, 220});
    DrawTriangleLines(body[0], body[1], body[2], edge);
    DrawTriangleLines(wingL[0], wingL[1], wingL[2], edge);
    DrawTriangleLines(wingR[1], wingR[0], wingR[2], edge);
    DrawCircleV(RP(0, 4), 5.0f, (Color){255, 160, 30, 200});
    #undef RP
}

static void DrawBoss(const Enemy *e) {
    float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 3.0f);
    float rot = DEG2RAD(e->rotation);
    float c = cosf(rot), s = sinf(rot);
    #define RP(lx, ly) (Vector2){ e->pos.x + (lx)*c - (ly)*s, e->pos.y + (lx)*s + (ly)*c }

    int phase = e->bossPhase;
    Color coreColor = (phase == 2) ? (Color){200, 30, 30, 255} : (Color){180, 20, 180, 255};
    Color edgeColor = (phase == 2) ? (Color){255, 80, 50, 255} : (Color){255, 80, 255, 255};
    Color glowColor = (phase == 2) ?
        (Color){255, 60, 0, (unsigned char)(80 + pulse * 100)} :
        (Color){200, 0, 255, (unsigned char)(60 + pulse * 100)};

    /* Outer glow */
    DrawCircleV(e->pos, e->radius + 12.0f * pulse, glowColor);

    /* Main hull — large diamond/star shape */
    float r  = e->radius;
    float r2 = r * 0.5f;
    Vector2 pts[8];
    for (int i = 0; i < 8; i++) {
        float a = DEG2RAD(i * 45.0f);
        float rr = (i % 2 == 0) ? r : r2;
        pts[i] = (Vector2){ e->pos.x + cosf(a) * rr, e->pos.y + sinf(a) * rr };
    }
    for (int i = 0; i < 8; i++) {
        DrawTriangle(e->pos, pts[i], pts[(i+1)%8], coreColor);
        DrawLineV(pts[i], pts[(i+1)%8], edgeColor);
    }

    /* Inner detail */
    DrawCircleV(e->pos, r * 0.35f, edgeColor);
    DrawCircleV(e->pos, r * 0.2f,  WHITE);

    /* Wing cannons */
    Vector2 canL = RP(-r * 0.8f, 0);
    Vector2 canR = RP( r * 0.8f, 0);
    DrawCircleV(canL, 7.0f, (phase == 2) ? (Color){255,80,0,220} : (Color){200,0,255,220});
    DrawCircleV(canR, 7.0f, (phase == 2) ? (Color){255,80,0,220} : (Color){200,0,255,220});

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

