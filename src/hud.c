/**
 * hud.c - HUD rendering
 */
#include "hud.h"
#include "utils.h"
#include "config.h"
#include "powerup.h"
#include "progress.h"
#include "raylib.h"
#include <stdio.h>
#include <math.h>

/* Draw a horizontal bar */
static void DrawBar(int x, int y, int w, int h, float frac,
                    Color bg, Color fill, Color border) {
    DrawRectangle(x, y, w, h, bg);
    DrawRectangle(x, y, (int)((float)w * CLAMP(frac, 0.0f, 1.0f)), h, fill);
    DrawRectangleLines(x, y, w, h, border);
}

void HUD_DrawBossBar(const Enemy *boss, int wave, int screenW, int screenH) {
    if (!boss || !boss->active) return;

    float frac = (float)boss->health / (float)boss->healthMax;
    int bw = (int)((float)screenW * 0.5f);
    int bh = 20;
    int bx = screenW / 2 - bw / 2;
    int by = screenH - 50;

    /* Background panel */
    DrawRectangle(bx - 6, by - 24, bw + 12, bh + 30, (Color){10, 0, 20, 180});

    /* Sector Boss Label */
    int sector = (wave <= 3) ? 0 : (wave <= 6) ? 1 : (wave <= 9) ? 2 : 3;
    const char *bossName = Enemy_GetBossName(sector);
    char bossLabel[160];
    snprintf(bossLabel, sizeof(bossLabel), "%s %s", bossName,
             boss->bossPhase == 2 ? "// [CRITICAL REACTOR BREACH]" : "// [CAPITAL FLAGSHIP]");

    int labelWidth = MeasureText(bossLabel, 12);
    DrawText(bossLabel, bx + bw / 2 - labelWidth / 2, by - 18, 12,
             boss->bossPhase == 2 ? (Color){255, 60, 60, 255} : (Color){255, 215, 0, 255});

    /* Bar */
    Color barColor = (frac > 0.5f) ? (Color){180, 0, 255, 255} :
                     (frac > 0.25f) ? (Color){255, 100, 0, 255} :
                                      (Color){255, 30, 30, 255};
    DrawBar(bx, by, bw, bh, frac, (Color){40, 0, 60, 220}, barColor,
            (Color){200, 100, 255, 200});
}

void HUD_DrawMedalPopup(int medalId, float timer, int screenW) {
    if (timer <= 0.0f || medalId < 0 || medalId >= MEDAL_COUNT) return;

    float alpha = CLAMP(timer * 1.5f, 0.0f, 1.0f);
    int pw = 420, ph = 46;
    int px = screenW / 2 - pw / 2;
    int py = 70;

    DrawRectangleRounded((Rectangle){ (float)px, (float)py, (float)pw, (float)ph }, 0.15f, 6,
                         (Color){ 6, 14, 30, (unsigned char)(alpha * 230) });
    DrawRectangleRoundedLines((Rectangle){ (float)px, (float)py, (float)pw, (float)ph }, 0.15f, 6,
                              (Color){ 255, 215, 0, (unsigned char)(alpha * 255) });

    const char *icon = Progress_GetMedalIcon((MedalId)medalId);
    const char *title = Progress_GetMedalTitle((MedalId)medalId);

    char banner[128];
    snprintf(banner, sizeof(banner), "HONOR UNLOCKED: %s %s", icon, title);
    int tw = MeasureText(banner, 13);
    DrawText(banner, screenW / 2 - tw / 2, py + 16, 13, (Color){ 255, 220, 40, (unsigned char)(alpha * 255) });
}

void HUD_Draw(const GameContext *ctx, const Player *player,
              const EnemyPool *enemies, int screenW, int screenH) {
    /* ── Top left: Score ── */
    char buf[64];
    snprintf(buf, sizeof(buf), "SCORE: %d", ctx->score);
    DrawText(buf, 16, 12, 22, (Color){0, 220, 255, 255});
    snprintf(buf, sizeof(buf), "HI: %d", ctx->highScore);
    DrawText(buf, 16, 36, 14, (Color){100, 180, 220, 200});

    /* ── Top right: Wave & Menu Hints ── */
    snprintf(buf, sizeof(buf), "WAVE %d", ctx->wave);
    int ww = MeasureText(buf, 22);
    DrawText(buf, screenW - ww - 16, 12, 22, (Color){255, 200, 0, 255});
    DrawText("[ESC] PAUSE / QUIT", screenW - 146, 38, 12, (Color){140, 160, 200, 180});
    DrawText("[F1] CONTROLS", screenW - 246, 38, 12, (Color){140, 160, 200, 180});

    /* ── Bottom left: Health bar ── */
    int barW = 200;
    int barH = 16;
    int barX = 16;
    int barY = screenH - 40;
    float hFrac = (float)player->health / (float)PLAYER_MAX_HEALTH;
    Color hColor = hFrac > 0.6f ? (Color){0, 220, 80, 255} :
                   hFrac > 0.3f ? (Color){255, 180, 0, 255} :
                                  (Color){255, 40, 40, 255};
    DrawText("HP", barX, barY - 18, 13, (Color){180, 180, 180, 200});
    DrawBar(barX, barY, barW, barH, hFrac,
            (Color){30, 0, 0, 200}, hColor, (Color){100, 100, 100, 180});

    /* ── Lives ── */
    int livesX = barX + barW + 18;
    DrawText("LIVES:", livesX, barY - 18, 13, (Color){180, 180, 180, 200});
    for (int i = 0; i < player->lives; i++) {
        float lx = (float)(livesX) + (float)i * 22.0f;
        float ly = (float)barY + 8.0f;
        /* Tiny ship icon */
        DrawTriangle(
            (Vector2){lx, ly - 7},
            (Vector2){lx - 6, ly + 5},
            (Vector2){lx + 6, ly + 5},
            (Color){30, 130, 255, 200}
        );
    }

    /* ── Active power-ups row ── */
    int puY = screenH - 70;
    int puX = screenW - 10;
    float now = (float)GetTime();

    /* Shield */
    if (player->shieldTimer > 0.0f) {
        snprintf(buf, sizeof(buf), "SHIELD %.1fs", player->shieldTimer);
        int tw = MeasureText(buf, 12);
        DrawText(buf, puX - tw, puY, 12, (Color){0, 160, 255, 230});
        puX -= tw + 12;
    }
    /* Rapid fire */
    if (player->rapidFireTimer > 0.0f) {
        snprintf(buf, sizeof(buf), "RAPID %.1fs", player->rapidFireTimer);
        int tw = MeasureText(buf, 12);
        DrawText(buf, puX - tw, puY, 12, (Color){255, 160, 0, 230});
        puX -= tw + 12;
    }
    /* Double shot */
    if (player->doubleShotTimer > 0.0f) {
        snprintf(buf, sizeof(buf), "DOUBLE %.1fs", player->doubleShotTimer);
        int tw = MeasureText(buf, 12);
        DrawText(buf, puX - tw, puY, 12, (Color){0, 255, 200, 230});
        puX -= tw + 12;
    }
    /* Spread shot */
    if (player->spreadShotTimer > 0.0f) {
        snprintf(buf, sizeof(buf), "SPREAD %.1fs", player->spreadShotTimer);
        int tw = MeasureText(buf, 12);
        DrawText(buf, puX - tw, puY, 12, (Color){255, 220, 0, 230});
        puX -= tw + 12;
    }
    /* Invincible */
    if (player->invincibleTimer > 0.0f) {
        snprintf(buf, sizeof(buf), "INVINC %.1fs", player->invincibleTimer);
        int tw = MeasureText(buf, 12);
        DrawText(buf, puX - tw, puY, 12, (Color){255, 230, 0, 230});
    }

    /* ── Boss bar ── */
    int bossIdx = Enemies_FindBoss(enemies);
    if (bossIdx >= 0) {
        HUD_DrawBossBar(&enemies->enemies[bossIdx], ctx->wave, screenW, screenH);
    }

    (void)now;
    (void)ctx;
}

