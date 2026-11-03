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
    /* ── Top Bar: Glassmorphic Tactical Command Frame ── */
    DrawRectangle(0, 0, screenW, 46, (Color){ 6, 10, 20, 200 });
    DrawLine(0, 46, screenW, 46, (Color){ 0, 180, 255, 120 });

    /* ── Top Left: Tactical Sector Directive ── */
    int sectorIdx = (ctx->wave <= 3) ? 0 : (ctx->wave <= 6) ? 1 : (ctx->wave <= 9) ? 2 : 3;
    const char *secNames[] = { "TERRA NOVUS", "SOLAR CORONA", "ICE RINGS OF XYLOS", "SYNOD CITADEL" };
    const char *secDirectives[] = {
        "DEFEND REFUGEE FLEET", "PURGE SIEGE CRUSHER", "BREAK CARRIER LINE", "ASSAULT CITADEL CORE"
    };

    /* Pulsing status beacon */
    float beaconPulse = 0.5f + 0.5f * sinf((float)GetTime() * 6.0f);
    DrawCircle(20, 23, 4.0f, (Color){ 0, 255, 180, (unsigned char)(160 + beaconPulse * 95) });

    char dirBuf[128];
    snprintf(dirBuf, sizeof(dirBuf), "SECTOR %d: %s // %s", sectorIdx + 1, secNames[sectorIdx], secDirectives[sectorIdx]);
    DrawText(dirBuf, 32, 16, 12, (Color){ 0, 240, 255, 230 });

    /* ── Top Center: Score & High Score ── */
    char buf[64];
    snprintf(buf, sizeof(buf), "%06d", ctx->score);
    int scW = MeasureText(buf, 24);
    DrawText("SCORE", screenW / 2 - scW / 2 - 62, 18, 11, (Color){ 120, 150, 180, 200 });
    DrawText(buf, screenW / 2 - scW / 2, 12, 24, (Color){ 255, 220, 40, 255 });

    snprintf(buf, sizeof(buf), "HI: %06d", ctx->highScore);
    DrawText(buf, screenW / 2 + scW / 2 + 18, 18, 11, (Color){ 100, 180, 220, 180 });

    /* ── Top Right: Wave Counter & Elimination Meter ── */
    snprintf(buf, sizeof(buf), "WAVE %d", ctx->wave);
    int ww = MeasureText(buf, 18);
    DrawText(buf, screenW - ww - 240, 14, 18, (Color){ 0, 220, 255, 255 });

    /* Mini wave progress meter */
    int progW = 75, progH = 8;
    int progX = screenW - 230;
    int progY = 20;
    float wFrac = (ctx->waveEnemiesRequired > 0) ?
                  (float)ctx->waveEnemiesKilled / (float)ctx->waveEnemiesRequired : 0.0f;
    DrawBar(progX, progY, progW, progH, wFrac, (Color){ 20, 30, 45, 220 }, (Color){ 0, 240, 255, 255 }, (Color){ 40, 80, 120, 180 });

    DrawText("[F1] HELP", screenW - 140, 16, 11, (Color){ 140, 160, 200, 190 });
    DrawText("[ESC] PAUSE", screenW - 74, 16, 11, (Color){ 140, 160, 200, 190 });

    /* ── Bottom Left: Armor / Hull & Kinetic Shield Gauges ── */
    int barW = 210, barH = 14;
    int barX = 22;
    int barY = screenH - 34;

    /* Frame backing */
    DrawRectangleRounded((Rectangle){ 14.0f, (float)(screenH - 74), 290.0f, 62.0f }, 0.12f, 6, (Color){ 6, 12, 24, 210 });
    DrawRectangleRoundedLines((Rectangle){ 14.0f, (float)(screenH - 74), 290.0f, 62.0f }, 0.12f, 6, (Color){ 0, 180, 255, 140 });

    float hFrac = (float)player->health / (float)PLAYER_MAX_HEALTH;
    Color hColor = (hFrac > 0.55f) ? (Color){ 0, 230, 100, 255 } :
                   (hFrac > 0.25f) ? (Color){ 255, 180, 20, 255 } : (Color){ 255, 40, 40, 255 };

    char hpBuf[32];
    snprintf(hpBuf, sizeof(hpBuf), "HULL %d%%", (int)(hFrac * 100.0f));
    DrawText(hpBuf, barX, barY - 16, 11, hColor);
    DrawBar(barX, barY, barW, barH, hFrac, (Color){ 25, 10, 15, 230 }, hColor, (Color){ 80, 90, 110, 200 });

    /* Segmented notches on health bar */
    for (int s = 1; s < 5; s++) {
        int sx = barX + s * (barW / 5);
        DrawLine(sx, barY, sx, barY + barH, (Color){ 0, 0, 0, 160 });
    }

    /* Kinetic Over-Shield indicator */
    if (player->shieldActive) {
        float sFrac = CLAMP(player->shieldTimer / 10.0f, 0.0f, 1.0f);
        DrawBar(barX, barY - 24, barW, 4, sFrac, (Color){ 0, 30, 60, 200 }, (Color){ 0, 210, 255, 255 }, (Color){ 0, 180, 255, 255 });
        DrawText("SHIELD ONLINE", barX + 80, barY - 34, 9, (Color){ 0, 220, 255, 255 });
    }

    /* Lives: Miniature HK-01 Silhouettes */
    int livesX = barX + barW + 12;
    for (int i = 0; i < player->lives; i++) {
        float lx = (float)livesX + (float)i * 14.0f;
        float ly = (float)barY + 6.0f;
        DrawTriangle((Vector2){ lx, ly - 6 }, (Vector2){ lx - 5, ly + 5 }, (Vector2){ lx + 5, ly + 5 }, (Color){ 0, 220, 255, 240 });
    }

    /* ── Bottom Center: Tactical 360-Degree Mini-Radar Scanner ── */
    float radarRadius = 42.0f;
    Vector2 radarCenter = { (float)screenW * 0.5f, (float)screenH - 52.0f };

    DrawCircleV(radarCenter, radarRadius + 2.0f, (Color){ 4, 8, 18, 220 });
    DrawCircleLinesV(radarCenter, radarRadius, (Color){ 0, 180, 255, 130 });
    DrawCircleLinesV(radarCenter, radarRadius * 0.5f, (Color){ 0, 180, 255, 60 });
    DrawLineV((Vector2){ radarCenter.x - radarRadius, radarCenter.y }, (Vector2){ radarCenter.x + radarRadius, radarCenter.y }, (Color){ 0, 180, 255, 45 });
    DrawLineV((Vector2){ radarCenter.x, radarCenter.y - radarRadius }, (Vector2){ radarCenter.x, radarCenter.y + radarRadius }, (Color){ 0, 180, 255, 45 });

    /* Radar sweep line */
    float sweepAngle = (float)GetTime() * 3.5f;
    Vector2 sweepTip = { radarCenter.x + cosf(sweepAngle) * radarRadius, radarCenter.y + sinf(sweepAngle) * radarRadius };
    DrawLineV(radarCenter, sweepTip, (Color){ 0, 240, 255, 160 });

    /* Player center blip */
    DrawCircleV(radarCenter, 2.5f, (Color){ 0, 220, 255, 255 });

    /* Threat blips */
    float radarScale = radarRadius / ((float)screenW * 0.65f);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies->enemies[i].active) continue;
        const Enemy *e = &enemies->enemies[i];
        float dx = (e->pos.x - player->pos.x) * radarScale;
        float dy = (e->pos.y - player->pos.y) * radarScale;
        if (dx * dx + dy * dy < (radarRadius - 3.0f) * (radarRadius - 3.0f)) {
            Vector2 blip = { radarCenter.x + dx, radarCenter.y + dy };
            if (e->type == CIVILIAN_SHUTTLE) {
                DrawCircleV(blip, 2.8f, (Color){ 0, 255, 140, 255 }); /* Green refugee shuttle */
            } else if (e->type == ENEMY_BOSS) {
                DrawCircleV(blip, 4.0f, (Color){ 255, 215, 0, 255 });   /* Gold Boss dreadnought */
                DrawCircleLinesV(blip, 5.5f, (Color){ 255, 60, 20, 255 });
            } else {
                DrawCircleV(blip, 2.2f, (Color){ 255, 50, 50, 255 });   /* Red hostile */
            }
        }
    }
    DrawText("RADAR", (int)radarCenter.x - 14, (int)radarCenter.y + (int)radarRadius + 3, 9, (Color){ 0, 180, 255, 180 });

    /* ── Active power-ups row ── */
    int puY = screenH - 72;
    int puX = screenW - 14;

    if (player->shieldTimer > 0.0f) {
        snprintf(buf, sizeof(buf), "SHIELD %.1fs", player->shieldTimer);
        int tw = MeasureText(buf, 11);
        DrawText(buf, puX - tw, puY, 11, (Color){0, 180, 255, 230});
        puX -= tw + 12;
    }
    if (player->rapidFireTimer > 0.0f) {
        snprintf(buf, sizeof(buf), "RAPID %.1fs", player->rapidFireTimer);
        int tw = MeasureText(buf, 11);
        DrawText(buf, puX - tw, puY, 11, (Color){255, 150, 20, 230});
        puX -= tw + 12;
    }
    if (player->doubleShotTimer > 0.0f) {
        snprintf(buf, sizeof(buf), "DOUBLE %.1fs", player->doubleShotTimer);
        int tw = MeasureText(buf, 11);
        DrawText(buf, puX - tw, puY, 11, (Color){0, 255, 190, 230});
        puX -= tw + 12;
    }
    if (player->spreadShotTimer > 0.0f) {
        snprintf(buf, sizeof(buf), "SPREAD %.1fs", player->spreadShotTimer);
        int tw = MeasureText(buf, 11);
        DrawText(buf, puX - tw, puY, 11, (Color){255, 220, 0, 230});
        puX -= tw + 12;
    }
    if (player->invincibleTimer > 0.0f) {
        snprintf(buf, sizeof(buf), "INVINC %.1fs", player->invincibleTimer);
        int tw = MeasureText(buf, 11);
        DrawText(buf, puX - tw, puY, 11, (Color){255, 230, 40, 230});
    }

    /* ── Boss bar ── */
    int bossIdx = Enemies_FindBoss(enemies);
    if (bossIdx >= 0) {
        HUD_DrawBossBar(&enemies->enemies[bossIdx], ctx->wave, screenW, screenH);
    }
}

