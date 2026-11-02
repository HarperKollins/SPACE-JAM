/**
 * world.c - Parallax starfield background implementation
 */
#include "world.h"
#include "utils.h"
#include <math.h>

static const float kSpeeds[STAR_LAYERS] = {
    STAR_SPEED_LAYER0,
    STAR_SPEED_LAYER1,
    STAR_SPEED_LAYER2
};

void Starfield_Init(Starfield *sf, int screenW, int screenH) {
    for (int layer = 0; layer < STAR_LAYERS; layer++) {
        sf->speeds[layer] = kSpeeds[layer];
        for (int i = 0; i < STARS_PER_LAYER; i++) {
            sf->layers[layer][i].pos.x = (float)GetRandomValue(0, screenW);
            sf->layers[layer][i].pos.y = (float)GetRandomValue(0, screenH);
            /* Brighter and larger for closer layers */
            float baseBright = 0.35f + (float)layer * 0.22f;
            sf->layers[layer][i].brightness = RandFloat(baseBright, baseBright + 0.3f);
            sf->layers[layer][i].size = 0.8f + (float)layer * 0.7f + RandFloat(0.0f, 0.8f);
        }
    }
}

void Starfield_Update(Starfield *sf, float dt, int screenW, int screenH) {
    for (int layer = 0; layer < STAR_LAYERS; layer++) {
        float speed = sf->speeds[layer];
        for (int i = 0; i < STARS_PER_LAYER; i++) {
            sf->layers[layer][i].pos.y += speed * dt;
            /* Wrap star back to top when it exits the bottom */
            if (sf->layers[layer][i].pos.y > (float)screenH + 4.0f) {
                sf->layers[layer][i].pos.y = -4.0f;
                sf->layers[layer][i].pos.x = (float)GetRandomValue(0, screenW);
            }
        }
    }
}

void Starfield_Draw(const Starfield *sf) {
    for (int layer = 0; layer < STAR_LAYERS; layer++) {
        for (int i = 0; i < STARS_PER_LAYER; i++) {
            const Star *s = &sf->layers[layer][i];
            unsigned char lum = (unsigned char)(s->brightness * 255.0f);
            Color col = { lum, lum, (unsigned char)(MIN(255, lum + 40)), 255 };

            float r = s->size;
            if (r <= 1.2f) {
                DrawPixelV(s->pos, col);
            } else {
                /* Closest layer: draw a tiny cross/glint for atmosphere */
                DrawCircleV(s->pos, r, col);
                if (layer == STAR_LAYERS - 1) {
                    Color glint = ColorAlpha(WHITE, s->brightness * 0.4f);
                    DrawLineV(
                        (Vector2){s->pos.x - r * 1.8f, s->pos.y},
                        (Vector2){s->pos.x + r * 1.8f, s->pos.y},
                        glint
                    );
                    DrawLineV(
                        (Vector2){s->pos.x, s->pos.y - r * 1.8f},
                        (Vector2){s->pos.x, s->pos.y + r * 1.8f},
                        glint
                    );
                }
            }
        }
    }
}

