/**
 * world.c - Rich cosmic environment: Parallax starfield, drifting nebulae,
 *           distant cratered moon, and curved glowing blue planet horizon.
 */
#include "world.h"
#include "utils.h"
#include <math.h>

static const float kSpeeds[STAR_LAYERS] = {
    STAR_SPEED_LAYER0,
    STAR_SPEED_LAYER1,
    STAR_SPEED_LAYER2
};

/* Nebula cloud base color palette: deep purples, violets, and cosmic indigos */
static const Color kNebulaPalette[] = {
    { 140,  30, 180,  32 }, /* Vibrant violet */
    {  90,  15, 150,  36 }, /* Deep purple */
    { 180,  40, 190,  24 }, /* Bright magenta dust */
    {  40,  30, 130,  30 }, /* Cosmic indigo */
    {  20,  80, 180,  20 }, /* Faint blue haze */
    { 120,  20, 140,  28 }, /* Royal violet */
};
#define NEBULA_PALETTE_COUNT 6

void Starfield_Init(Starfield *sf, int screenW, int screenH) {
    sf->time = 0.0f;

    /* Initialize stars */
    for (int layer = 0; layer < STAR_LAYERS; layer++) {
        sf->speeds[layer] = kSpeeds[layer];
        for (int i = 0; i < STARS_PER_LAYER; i++) {
            sf->layers[layer][i].pos.x = (float)GetRandomValue(0, screenW);
            sf->layers[layer][i].pos.y = (float)GetRandomValue(0, screenH);
            
            float baseBright = 0.35f + (float)layer * 0.22f;
            sf->layers[layer][i].brightness   = RandFloat(baseBright, baseBright + 0.35f);
            sf->layers[layer][i].size         = 0.8f + (float)layer * 0.7f + RandFloat(0.0f, 0.7f);
            sf->layers[layer][i].twinkleSpeed = RandFloat(2.0f, 6.0f);
            sf->layers[layer][i].twinklePhase = RandFloat(0.0f, 6.28f);
            /* Brightest ~8% of stars in closest layers get retro cross-glints */
            sf->layers[layer][i].hasGlint = (layer >= 1 && (GetRandomValue(0, 100) < 10));
        }
    }

    /* Initialize drifting nebula dust clouds */
    for (int i = 0; i < MAX_NEBULA_CLOUDS; i++) {
        sf->nebulae[i].pos = (Vector2){
            RandFloat(-100.0f, (float)screenW + 100.0f),
            RandFloat(-100.0f, (float)screenH + 100.0f)
        };
        sf->nebulae[i].radius = RandFloat(120.0f, 260.0f);
        sf->nebulae[i].color  = kNebulaPalette[i % NEBULA_PALETTE_COUNT];
        sf->nebulae[i].speed  = RandFloat(6.0f, 15.0f); /* Very slow majestic drift */
    }
}

void Starfield_Update(Starfield *sf, float dt, int screenW, int screenH) {
    sf->time += dt;

    /* Update stars */
    for (int layer = 0; layer < STAR_LAYERS; layer++) {
        float speed = sf->speeds[layer];
        for (int i = 0; i < STARS_PER_LAYER; i++) {
            sf->layers[layer][i].pos.y += speed * dt;
            if (sf->layers[layer][i].pos.y > (float)screenH + 8.0f) {
                sf->layers[layer][i].pos.y = -8.0f;
                sf->layers[layer][i].pos.x = (float)GetRandomValue(0, screenW);
            }
        }
    }

    /* Update nebula clouds */
    for (int i = 0; i < MAX_NEBULA_CLOUDS; i++) {
        sf->nebulae[i].pos.y += sf->nebulae[i].speed * dt;
        if (sf->nebulae[i].pos.y - sf->nebulae[i].radius > (float)screenH + 40.0f) {
            sf->nebulae[i].pos.y = -sf->nebulae[i].radius - 30.0f;
            sf->nebulae[i].pos.x = RandFloat(-60.0f, (float)screenW + 60.0f);
        }
    }
}

/* ── Draw Celestial Moon (Upper Right) ────────────────────────── */
static void DrawDistantMoon(int screenW, int screenH) {
    (void)screenH;
    Vector2 moonPos = { (float)screenW * 0.88f, 110.0f };
    float moonRadius = 38.0f;

    /* Subtle atmospheric / back glow */
    DrawCircleV(moonPos, moonRadius + 10.0f, (Color){ 70, 80, 120, 18 });
    DrawCircleV(moonPos, moonRadius + 4.0f,  (Color){ 100, 120, 170, 30 });

    /* Moon base sphere (cool grey) */
    DrawCircleV(moonPos, moonRadius, (Color){ 160, 175, 195, 255 });

    /* Craters & Maria details */
    DrawCircleV((Vector2){ moonPos.x - 10, moonPos.y - 8 },  7.0f, (Color){ 130, 142, 162, 255 });
    DrawCircleV((Vector2){ moonPos.x + 8,  moonPos.y - 12 }, 5.5f, (Color){ 125, 138, 158, 255 });
    DrawCircleV((Vector2){ moonPos.x - 4,  moonPos.y + 10 }, 8.0f, (Color){ 120, 132, 152, 255 });
    DrawCircleV((Vector2){ moonPos.x + 14, moonPos.y + 8 },  4.5f, (Color){ 135, 148, 168, 255 });
    DrawCircleV((Vector2){ moonPos.x - 18, moonPos.y + 6 },  5.0f, (Color){ 130, 142, 162, 255 });

    /* Shadow / Crescent night-side terminator (matches the lit side from right) */
    /* Draw dark crescent on the left */
    DrawCircleSector(moonPos, moonRadius, 90.0f, 270.0f, 32, (Color){ 12, 16, 32, 195 });
    DrawCircleV((Vector2){ moonPos.x + 6, moonPos.y }, moonRadius * 0.94f, (Color){ 12, 16, 32, 110 });

    /* Fine pixel highlight rim on the sunny side */
    DrawCircleSectorLines(moonPos, moonRadius, -45.0f, 45.0f, 16, (Color){ 220, 235, 255, 180 });
}

/* ── Draw Giant Blue Planet Horizon (Bottom) ─────────────────── */
static void DrawPlanetHorizon(int screenW, int screenH) {
    /* Large curved sphere centered far beneath the screen */
    float planetRadius = (float)screenW * 1.35f;
    Vector2 planetCenter = { (float)screenW * 0.72f, (float)screenH + planetRadius - 130.0f };

    /* Atmospheric Glow Rings (layered luminous cyan/blue) */
    DrawCircleV(planetCenter, planetRadius + 44.0f, (Color){   0,  60, 180,  15 });
    DrawCircleV(planetCenter, planetRadius + 28.0f, (Color){   0, 120, 255,  35 });
    DrawCircleV(planetCenter, planetRadius + 14.0f, (Color){  40, 180, 255,  65 });
    DrawCircleV(planetCenter, planetRadius +  5.0f, (Color){ 120, 230, 255, 130 });

    /* Planet Body (Deep oceanic blue) */
    DrawCircleV(planetCenter, planetRadius, (Color){ 16, 50, 140, 255 });

    /* Cloud bands and continent accents (curved concentric bands) */
    DrawCircleSector(planetCenter, planetRadius - 18.0f, 210.0f, 320.0f, 48, (Color){ 30, 95, 195, 160 });
    DrawCircleSector(planetCenter, planetRadius - 40.0f, 218.0f, 310.0f, 48, (Color){ 20, 75, 175, 180 });

    /* Wispy swirl cloud patches */
    DrawCircleV((Vector2){ (float)screenW * 0.50f, (float)screenH - 35 }, 55.0f, (Color){ 180, 220, 255, 75 });
    DrawCircleV((Vector2){ (float)screenW * 0.75f, (float)screenH - 55 }, 80.0f, (Color){ 190, 230, 255, 85 });
    DrawCircleV((Vector2){ (float)screenW * 0.90f, (float)screenH - 25 }, 65.0f, (Color){ 170, 215, 255, 65 });

    /* Bright blue atmospheric edge line */
    DrawCircleSectorLines(planetCenter, planetRadius + 1.0f, 205.0f, 335.0f, 64, (Color){ 140, 240, 255, 230 });
}

/* ── Main World Draw ─────────────────────────────────────────── */
void Starfield_Draw(const Starfield *sf) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    /* 1. Deep Space Sky Gradient (dark violet-black to midnight blue) */
    DrawRectangleGradientV(-40, -40, sw + 80, sh + 80,
                           (Color){ 3, 2, 14, 255 },
                           (Color){ 7, 10, 28, 255 });

    /* 2. Volumetric Glowing Nebulae (Deep purple & magenta clouds) */
    for (int i = 0; i < MAX_NEBULA_CLOUDS; i++) {
        const NebulaCloud *nc = &sf->nebulae[i];
        /* Layered concentric radial falloff for smooth gaseous appearance */
        DrawCircleV(nc->pos, nc->radius,          nc->color);
        DrawCircleV(nc->pos, nc->radius * 0.65f,  ColorAlpha(nc->color, nc->color.a * 1.5f / 255.0f));
        DrawCircleV(nc->pos, nc->radius * 0.35f,  ColorAlpha(nc->color, nc->color.a * 2.2f / 255.0f));
    }

    /* 3. Celestial Bodies (Distant Moon & Giant Blue Planet Horizon) */
    DrawDistantMoon(sw, sh);
    DrawPlanetHorizon(sw, sh);

    /* 4. Multi-Layered Stars with Twinkling & Retro 4-Point Glints */
    for (int layer = 0; layer < STAR_LAYERS; layer++) {
        for (int i = 0; i < STARS_PER_LAYER; i++) {
            const Star *s = &sf->layers[layer][i];

            /* Twinkle brightness modulation */
            float tw = sinf(sf->time * s->twinkleSpeed + s->twinklePhase);
            float bright = CLAMP(s->brightness + tw * 0.20f, 0.15f, 1.0f);
            unsigned char lum = (unsigned char)(bright * 255.0f);

            /* Subtle cosmic hue tinting (some icy blue, some white, some warm) */
            Color col;
            if (i % 5 == 0) {
                col = (Color){ (unsigned char)(lum * 0.85f), (unsigned char)(lum * 0.92f), lum, 255 }; /* Icy blue */
            } else if (i % 7 == 0) {
                col = (Color){ lum, (unsigned char)(lum * 0.85f), (unsigned char)(lum * 0.95f), 255 }; /* Pale violet */
            } else {
                col = (Color){ lum, lum, (unsigned char)MIN(255, lum + 35), 255 };                       /* Crisp white */
            }

            float r = s->size;
            if (r <= 1.2f) {
                DrawPixelV(s->pos, col);
            } else {
                DrawCircleV(s->pos, r, col);
            }

            /* Retro 4-Point Star Glints (Diamond Cross Starburst) */
            if (s->hasGlint && bright > 0.65f) {
                float glintLen = (r * 3.5f + 3.0f) * (0.8f + tw * 0.25f);
                Color glintCol = ColorAlpha(col, bright * 0.85f);
                Color glintGlow = ColorAlpha(WHITE, bright * 0.40f);

                /* Horizontal beam */
                DrawLineV((Vector2){ s->pos.x - glintLen, s->pos.y },
                          (Vector2){ s->pos.x + glintLen, s->pos.y }, glintCol);
                /* Vertical beam */
                DrawLineV((Vector2){ s->pos.x, s->pos.y - glintLen },
                          (Vector2){ s->pos.x, s->pos.y + glintLen }, glintCol);
                /* Center sparkle */
                DrawCircleV(s->pos, r * 0.6f, glintGlow);
            }
        }
    }
}
