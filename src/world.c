/**
 * world.c - Dynamic celestial environments, 4 planetary sectors,
 *           starfield parallax, and hyperspace warp jump effects.
 */
#include "world.h"
#include "utils.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static const float kSpeeds[STAR_LAYERS] = {
    STAR_SPEED_LAYER0,
    STAR_SPEED_LAYER1,
    STAR_SPEED_LAYER2
};

WorldSector Starfield_GetSectorForWave(int wave) {
    if (wave <= 3) return SECTOR_TERRA_NOVUS;
    if (wave <= 6) return SECTOR_SOLAR_CORONA;
    if (wave <= 9) return SECTOR_XYLOS_RINGS;
    return SECTOR_SYNOD_CITADEL;
}

const char *Starfield_GetSectorName(WorldSector sector) {
    switch (sector) {
        case SECTOR_TERRA_NOVUS:  return "SECTOR 1 // TERRA NOVUS";
        case SECTOR_SOLAR_CORONA: return "SECTOR 2 // THE SOLAR CORONA";
        case SECTOR_XYLOS_RINGS:  return "SECTOR 3 // ICE RINGS OF XYLOS";
        case SECTOR_SYNOD_CITADEL:return "SECTOR 4 // SYNOD FORTRESS CITADEL";
        default: return "DEEP SPACE";
    }
}

const char *Starfield_GetSectorSubtitle(WorldSector sector) {
    switch (sector) {
        case SECTOR_TERRA_NOVUS:  return "CRADLE WORLD IN FLAMES - PROTECT EVACUATION FLEET";
        case SECTOR_SOLAR_CORONA: return "SUPERHEATED SOLAR FLARES - HUNT THE SIEGE CRUSHER";
        case SECTOR_XYLOS_RINGS:  return "FROZEN GAS GIANT ORBIT - BREAK THE CARRIER BLOCKADE";
        case SECTOR_SYNOD_CITADEL:return "HEART OF THE ENEMY - DESTROY THE CITADEL FLAGSHIP";
        default: return "CONTESTED SPACE";
    }
}

void Starfield_Init(Starfield *sf, int screenW, int screenH) {
    sf->time = 0.0f;
    sf->sector = SECTOR_TERRA_NOVUS;
    sf->warpTimer = 0.0f;
    sf->warpDuration = 1.6f;

    /* Initialize stars */
    for (int layer = 0; layer < STAR_LAYERS; layer++) {
        sf->speeds[layer] = kSpeeds[layer];
        for (int i = 0; i < STARS_PER_LAYER; i++) {
            sf->layers[layer][i].pos.x = (float)GetRandomValue(0, screenW);
            sf->layers[layer][i].pos.y = (float)GetRandomValue(0, screenH);

            float baseBright = 0.35f + (float)layer * 0.22f;
            sf->layers[layer][i].brightness   = RandFloat(baseBright, baseBright + 0.35f);
            sf->layers[layer][i].size         = 0.8f + (float)layer * 0.6f + RandFloat(0.0f, 0.6f);
            sf->layers[layer][i].twinkleSpeed = RandFloat(2.0f, 6.0f);
            sf->layers[layer][i].twinklePhase = RandFloat(0.0f, 6.28f);
            sf->layers[layer][i].hasGlint     = (layer >= 1 && (GetRandomValue(0, 100) < 12));
        }
    }

    /* Initialize ambient nebulae */
    for (int i = 0; i < MAX_NEBULA_CLOUDS; i++) {
        sf->nebulae[i].pos = (Vector2){
            RandFloat(-80.0f, (float)screenW + 80.0f),
            RandFloat(-80.0f, (float)screenH + 80.0f)
        };
        sf->nebulae[i].radius = RandFloat(140.0f, 280.0f);
        sf->nebulae[i].speed  = RandFloat(8.0f, 18.0f);
        sf->nebulae[i].color  = (Color){ 10, 30, 80, 24 };
    }
}

void Starfield_SetSector(Starfield *sf, WorldSector sector, int triggerWarp) {
    if (sf->sector != sector && triggerWarp) {
        sf->warpTimer = sf->warpDuration;
    }
    sf->sector = sector;
}

void Starfield_Update(Starfield *sf, float dt, int screenW, int screenH) {
    sf->time += dt;

    if (sf->warpTimer > 0.0f) {
        sf->warpTimer -= dt;
        if (sf->warpTimer < 0.0f) sf->warpTimer = 0.0f;
    }

    float speedMult = 1.0f;
    if (sf->warpTimer > 0.0f) {
        float frac = sf->warpTimer / sf->warpDuration;
        speedMult = 1.0f + sinf(frac * (float)M_PI) * 18.0f;
    }

    /* Update stars */
    for (int layer = 0; layer < STAR_LAYERS; layer++) {
        float speed = sf->speeds[layer] * speedMult;
        for (int i = 0; i < STARS_PER_LAYER; i++) {
            sf->layers[layer][i].pos.y += speed * dt;
            if (sf->layers[layer][i].pos.y > (float)screenH + 12.0f) {
                sf->layers[layer][i].pos.y = -12.0f;
                sf->layers[layer][i].pos.x = (float)GetRandomValue(0, screenW);
            }
        }
    }

    /* Update nebula clouds */
    for (int i = 0; i < MAX_NEBULA_CLOUDS; i++) {
        sf->nebulae[i].pos.y += sf->nebulae[i].speed * speedMult * dt;
        if (sf->nebulae[i].pos.y - sf->nebulae[i].radius > (float)screenH + 50.0f) {
            sf->nebulae[i].pos.y = -sf->nebulae[i].radius - 30.0f;
            sf->nebulae[i].pos.x = RandFloat(-60.0f, (float)screenW + 60.0f);
        }
    }
}

/* ══════════════════════════════════════════════════════
   PLANETARY WORLD RENDERING
══════════════════════════════════════════════════════ */

/* Sector 1: Terra Novus (Cradle World in Flames) */
static void DrawTerraNovus(int screenW, int screenH, float time) {
    (void)time;
    /* 1. Distant cratered moon in upper right */
    Vector2 moonPos = { (float)screenW * 0.86f, 100.0f };
    float moonR = 34.0f;
    DrawCircleV(moonPos, moonR + 8.0f, (Color){ 40, 60, 100, 20 });
    DrawCircleV(moonPos, moonR,        (Color){ 165, 175, 195, 255 });
    DrawCircleV((Vector2){ moonPos.x - 9, moonPos.y - 7 },  6.0f, (Color){ 130, 140, 160, 255 });
    DrawCircleV((Vector2){ moonPos.x + 7, moonPos.y - 10 }, 5.0f, (Color){ 125, 135, 155, 255 });
    DrawCircleV((Vector2){ moonPos.x - 4, moonPos.y + 9 },  7.0f, (Color){ 120, 130, 150, 255 });
    DrawCircleSector(moonPos, moonR, 90.0f, 270.0f, 24, (Color){ 10, 14, 28, 200 });

    /* 2. Massive curved Terra Novus horizon at bottom */
    float planetR = (float)screenW * 1.30f;
    Vector2 planetC = { (float)screenW * 0.65f, (float)screenH + planetR - 135.0f };

    /* Atmospheric Glow Rings */
    DrawCircleV(planetC, planetR + 38.0f, (Color){   0,  60, 180,  18 });
    DrawCircleV(planetC, planetR + 22.0f, (Color){   0, 130, 255,  40 });
    DrawCircleV(planetC, planetR + 10.0f, (Color){  30, 190, 255,  80 });
    DrawCircleV(planetC, planetR +  4.0f, (Color){ 140, 240, 255, 160 });

    /* Deep Ocean Body */
    DrawCircleV(planetC, planetR, (Color){ 14, 48, 130, 255 });

    /* Continents (Emerald and deep olive landmasses) */
    DrawCircleSector(planetC, planetR - 12.0f, 215.0f, 315.0f, 48, (Color){ 24, 85, 70, 220 });
    DrawCircleSector(planetC, planetR - 35.0f, 225.0f, 305.0f, 48, (Color){ 18, 70, 60, 240 });

    /* Glowing Lava Fractures & Burning Continent Chasms */
    float pulse = 0.85f + 0.15f * sinf(time * 6.0f);
    Color lavaGlow = (Color){ 255, 110, 20, (unsigned char)(210 * pulse) };
    Color lavaCore = (Color){ 255, 230, 80, (unsigned char)(240 * pulse) };

    Vector2 f1 = { (float)screenW * 0.35f, (float)screenH - 55.0f };
    Vector2 f2 = { (float)screenW * 0.44f, (float)screenH - 40.0f };
    Vector2 f3 = { (float)screenW * 0.52f, (float)screenH - 48.0f };
    Vector2 f4 = { (float)screenW * 0.62f, (float)screenH - 32.0f };
    DrawLineEx(f1, f2, 4.0f, lavaGlow);
    DrawLineEx(f2, f3, 3.5f, lavaGlow);
    DrawLineEx(f3, f4, 3.0f, lavaGlow);
    DrawLineEx(f1, f2, 1.8f, lavaCore);
    DrawLineEx(f2, f3, 1.5f, lavaCore);
    DrawLineEx(f3, f4, 1.2f, lavaCore);

    /* Burning city clusters (gold pinpricks on nightside) */
    DrawCircleV((Vector2){ (float)screenW * 0.32f, (float)screenH - 70 }, 2.0f, (Color){ 255, 200, 80, 220 });
    DrawCircleV((Vector2){ (float)screenW * 0.38f, (float)screenH - 65 }, 1.8f, (Color){ 255, 210, 90, 240 });
    DrawCircleV((Vector2){ (float)screenW * 0.58f, (float)screenH - 45 }, 2.2f, (Color){ 255, 180, 60, 200 });

    /* Atmospheric horizon rim highlight */
    DrawCircleSectorLines(planetC, planetR + 1.0f, 208.0f, 332.0f, 64, (Color){ 160, 245, 255, 240 });
}

/* Sector 2: The Solar Corona (Blazing Sun & Heat Flares) */
static void DrawSolarCorona(int screenW, int screenH, float time) {
    (void)screenH;
    Vector2 sunC = { (float)screenW * 0.78f, -40.0f };
    float sunR   = 160.0f;

    /* Layered Solar Corona (Blazing Amber/Gold Haze) */
    float flarePulse = sinf(time * 3.5f) * 12.0f;
    DrawCircleV(sunC, sunR + 180.0f + flarePulse, (Color){ 255,  50,   0,  15 });
    DrawCircleV(sunC, sunR + 120.0f + flarePulse, (Color){ 255,  90,   0,  30 });
    DrawCircleV(sunC, sunR +  70.0f,              (Color){ 255, 150,  20,  60 });
    DrawCircleV(sunC, sunR +  30.0f,              (Color){ 255, 200,  50, 110 });
    DrawCircleV(sunC, sunR +  10.0f,              (Color){ 255, 240, 120, 180 });

    /* Blinding Photosphere Core */
    DrawCircleV(sunC, sunR, (Color){ 255, 252, 220, 255 });

    /* Prominences: Solar Flame Arcs leaping into space */
    for (int i = 0; i < 4; i++) {
        float angle = 120.0f + (float)i * 35.0f + sinf(time * 2.0f + (float)i) * 6.0f;
        float rad = DEG2RAD(angle);
        float armLen = sunR + 40.0f + sinf(time * 4.0f + (float)i * 1.5f) * 25.0f;
        Vector2 tip = { sunC.x + cosf(rad) * armLen, sunC.y + sinf(rad) * armLen };
        Vector2 mid = { sunC.x + cosf(rad + 0.15f) * (armLen * 0.75f),
                        sunC.y + sinf(rad + 0.15f) * (armLen * 0.75f) };
        DrawLineEx(sunC, mid, 6.0f, (Color){ 255, 120, 0, 180 });
        DrawLineEx(mid,  tip, 3.5f, (Color){ 255, 220, 40, 220 });
        DrawCircleV(tip, 5.0f, (Color){ 255, 240, 100, 255 });
    }
}

/* Sector 3: Ice Rings of Xylos (Emerald Gas Giant & Crystalline Rings) */
static void DrawXylosRings(int screenW, int screenH, float time) {
    (void)time;
    /* Colossal Emerald Gas Giant placed on mid-left */
    Vector2 planetC = { (float)screenW * 0.12f, (float)screenH * 0.45f };
    float planetR   = 120.0f;

    /* Atmospheric Glow */
    DrawCircleV(planetC, planetR + 25.0f, (Color){   0, 180, 140,  25 });
    DrawCircleV(planetC, planetR + 10.0f, (Color){  20, 220, 180,  60 });

    /* Emerald Base */
    DrawCircleV(planetC, planetR, (Color){ 12, 85, 75, 255 });

    /* Atmospheric Cloud Bands */
    DrawCircleSector(planetC, planetR -  8.0f, 260.0f, 440.0f, 32, (Color){  20, 120, 100, 220 });
    DrawCircleSector(planetC, planetR - 28.0f, 260.0f, 440.0f, 32, (Color){  14,  95,  85, 240 });
    DrawCircleSector(planetC, planetR - 55.0f, 260.0f, 440.0f, 32, (Color){  30, 145, 125, 200 });

    /* Nightside shadow crescent */
    DrawCircleSector(planetC, planetR, 80.0f, 260.0f, 32, (Color){ 4, 16, 20, 210 });

    /* Translucent Crystalline Ice Rings extending diagonally across space */
    for (int r = 0; r < 5; r++) {
        float ringW = 320.0f + (float)r * 40.0f;
        float ringH =  70.0f + (float)r *  9.0f;
        Color ringCol = (r % 2 == 0) ? (Color){ 140, 235, 255, 80 } : (Color){ 180, 255, 240, 55 };
        DrawEllipseLines((int)planetC.x, (int)planetC.y, ringW, ringH, ringCol);
    }
}

/* Sector 4: The Synod Citadel (Abyssal Void & Fortress Megastructure) */
static void DrawSynodCitadel(int screenW, int screenH, float time) {
    (void)screenH;
    /* Looming mechanical orbital fortress in upper center */
    Vector2 citC = { (float)screenW * 0.50f, 80.0f };

    /* Massive circular armor ring */
    float ringR = 190.0f;
    DrawCircleLinesV(citC, ringR,        (Color){ 50,  60,  75, 180 });
    DrawCircleLinesV(citC, ringR + 8.0f, (Color){ 70,  82, 100, 160 });

    /* Industrial Spoke Superstructures */
    for (int i = 0; i < 8; i++) {
        float ang = (float)i * 45.0f;
        float rad = DEG2RAD(ang);
        Vector2 inner = { citC.x + cosf(rad) * 45.0f,  citC.y + sinf(rad) * 45.0f };
        Vector2 outer = { citC.x + cosf(rad) * ringR,  citC.y + sinf(rad) * ringR };
        DrawLineEx(inner, outer, 4.0f, (Color){ 36, 44, 56, 240 });
        DrawLineEx(inner, outer, 1.5f, (Color){ 80, 95, 120, 255 });

        /* Pulsing warning beacon lights on the outer rim */
        float bPulse = 0.5f + 0.5f * sinf(time * 8.0f + (float)i * 0.8f);
        if (bPulse > 0.6f) {
            DrawCircleV(outer, 3.2f, (Color){ 255, 40, 40, (unsigned char)(bPulse * 255) });
        }
    }

    /* Central Hyperspace Conduit Core */
    float corePulse = 0.8f + 0.2f * sinf(time * 10.0f);
    DrawCircleV(citC, 32.0f * corePulse, (Color){  80,   0, 180,  80 });
    DrawCircleV(citC, 18.0f * corePulse, (Color){ 140,  40, 255, 180 });
    DrawCircleV(citC,  8.0f * corePulse, (Color){ 230, 200, 255, 255 });
}

/* ══════════════════════════════════════════════════════
   MAIN STARFIELD DRAW WITH WARP JUMP EFFECT
══════════════════════════════════════════════════════ */

void Starfield_Draw(const Starfield *sf) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    /* 1. Deep Space Sky Gradient (Pure crisp obsidian black to midnight navy) */
    DrawRectangleGradientV(-20, -20, sw + 40, sh + 40,
                           (Color){ 1,  2,  8, 255 },
                           (Color){ 4,  6, 18, 255 });

    /* 2. Sector-Specific Ambient Nebula Hues */
    Color nebBase;
    switch (sf->sector) {
        case SECTOR_TERRA_NOVUS:  nebBase = (Color){  10,  40, 110, 20 }; break; /* Ocean blues */
        case SECTOR_SOLAR_CORONA: nebBase = (Color){ 110,  50,  10, 22 }; break; /* Solar amber */
        case SECTOR_XYLOS_RINGS:  nebBase = (Color){  10,  80,  70, 22 }; break; /* Emerald auroras */
        case SECTOR_SYNOD_CITADEL:nebBase = (Color){  40,  15,  70, 18 }; break; /* Abyssal violet */
        default: nebBase = (Color){ 20, 30, 60, 20 }; break;
    }

    for (int i = 0; i < MAX_NEBULA_CLOUDS; i++) {
        const NebulaCloud *nc = &sf->nebulae[i];
        Color c = nebBase;
        c.a = (unsigned char)(nebBase.a + (i % 3) * 6);
        DrawCircleV(nc->pos, nc->radius,         c);
        DrawCircleV(nc->pos, nc->radius * 0.6f, ColorAlpha(c, (float)c.a * 1.6f / 255.0f));
    }

    /* 3. Celestial Planetary World Backdrop */
    switch (sf->sector) {
        case SECTOR_TERRA_NOVUS:  DrawTerraNovus(sw, sh, sf->time);   break;
        case SECTOR_SOLAR_CORONA: DrawSolarCorona(sw, sh, sf->time);  break;
        case SECTOR_XYLOS_RINGS:  DrawXylosRings(sw, sh, sf->time);   break;
        case SECTOR_SYNOD_CITADEL:DrawSynodCitadel(sw, sh, sf->time); break;
        default: break;
    }

    /* 4. Multi-Layered Stars (Normal twinkling points OR Hyperspace Warp Speedlines) */
    int isWarping = (sf->warpTimer > 0.0f);
    float warpIntensity = isWarping ? sinf((sf->warpTimer / sf->warpDuration) * (float)M_PI) : 0.0f;

    for (int layer = 0; layer < STAR_LAYERS; layer++) {
        for (int i = 0; i < STARS_PER_LAYER; i++) {
            const Star *s = &sf->layers[layer][i];

            if (isWarping && warpIntensity > 0.1f) {
                /* Hyperspace Warp Lines: Streaking lasers through the cosmos */
                float streakLen = (18.0f + (float)layer * 30.0f) * warpIntensity;
                Vector2 p1 = s->pos;
                Vector2 p2 = { s->pos.x, s->pos.y + streakLen };
                DrawLineEx(p1, p2, 1.2f + (float)layer * 0.8f,
                           ColorAlpha((Color){ 0, 220, 255, 255 }, 0.5f + warpIntensity * 0.45f));
                DrawLineEx(p1, p2, 0.8f, WHITE);
            } else {
                /* Normal Starfield: Twinkling diamonds */
                float tw = sinf(sf->time * s->twinkleSpeed + s->twinklePhase);
                float bright = CLAMP(s->brightness + tw * 0.20f, 0.15f, 1.0f);
                unsigned char lum = (unsigned char)(bright * 255.0f);

                Color col;
                if (i % 6 == 0)      col = (Color){ (unsigned char)(lum * 0.85f), (unsigned char)(lum * 0.95f), lum, 255 }; /* Icy cyan */
                else if (i % 9 == 0) col = (Color){ lum, (unsigned char)(lum * 0.92f), (unsigned char)(lum * 0.70f), 255 }; /* Warm gold */
                else                 col = (Color){ lum, lum, (unsigned char)MIN(255, lum + 30), 255 };                     /* Diamond white */

                float r = s->size;
                if (r <= 1.2f) {
                    DrawPixelV(s->pos, col);
                } else {
                    DrawCircleV(s->pos, r, col);
                }

                /* Retro 4-point glint */
                if (s->hasGlint && bright > 0.68f) {
                    float gLen = (r * 3.2f + 2.5f) * (0.8f + tw * 0.25f);
                    Color gCol = ColorAlpha(col, bright * 0.80f);
                    DrawLineV((Vector2){ s->pos.x - gLen, s->pos.y }, (Vector2){ s->pos.x + gLen, s->pos.y }, gCol);
                    DrawLineV((Vector2){ s->pos.x, s->pos.y - gLen }, (Vector2){ s->pos.x, s->pos.y + gLen }, gCol);
                }
            }
        }
    }
}
