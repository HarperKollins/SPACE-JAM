/**
 * menu.c - Menu screens (Main, Pause, Game Over, Settings)
 */
#include "menu.h"
#include "audio.h"
#include "utils.h"
#include "config.h"
#include "settings.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ── Helper: draw a centred, animated menu item ──────── */
static void DrawMenuItem(const char *text, int x, int y, int fontSize,
                          int selected, float animTime) {
    Color col;
    if (selected) {
        float pulse = 0.5f + 0.5f * sinf(animTime * 6.0f);
        col = (Color){
            (unsigned char)(0   + 80  * pulse),
            (unsigned char)(180 + 75  * pulse),
            (unsigned char)(220 + 35  * pulse),
            255
        };
        /* Selection marker */
        int tw = MeasureText(text, fontSize);
        DrawText(">", x - tw / 2 - fontSize - 4, y, fontSize, col);
        DrawText("<", x + tw / 2 + 4, y, fontSize, col);
    } else {
        col = (Color){ 160, 160, 190, 200 };
    }
    int tw = MeasureText(text, fontSize);
    DrawText(text, x - tw / 2, y, fontSize, col);
}

/* ── Animated SPACE JAM title ─────────────────────────── */
static void DrawTitle(int screenW, int screenH, float time) {
    /* Glow layers */
    const char *title = "SPACE JAM";
    int fontSize = 72;
    int tw = MeasureText(title, fontSize);
    int tx = screenW / 2 - tw / 2;
    int ty = (int)((float)screenH * 0.18f);

    float pulse = 0.5f + 0.5f * sinf(time * 2.2f);

    /* Outer electric blue glow */
    DrawText(title, tx + 3, ty + 3, fontSize, (Color){0, 80, 200, (unsigned char)(60 + pulse * 80)});
    DrawText(title, tx - 2, ty + 2, fontSize, (Color){0, 80, 200, (unsigned char)(60 + pulse * 80)});
    /* Neon cyan mid glow */
    DrawText(title, tx + 1, ty + 1, fontSize, (Color){0, 200, 255, (unsigned char)(100 + pulse * 100)});
    DrawText(title, tx - 1, ty - 1, fontSize, (Color){0, 200, 255, (unsigned char)(100 + pulse * 100)});
    /* White core */
    DrawText(title, tx, ty, fontSize, WHITE);

    /* Subtitle */
    const char *sub = "EPISODE I: THE LAST VECTOR";
    int sw = MeasureText(sub, 20);
    DrawText(sub, screenW / 2 - sw / 2, ty + fontSize + 8, 20,
             (Color){255, 215, 0, (unsigned char)(180 + pulse * 75)});

    /* Pilot & Ship Callout */
    const char *callout = "PILOT: HARPER KOLLINS  //  STRIKE CRAFT: HK-01 VANGUARD";
    int cw = MeasureText(callout, 12);
    DrawText(callout, screenW / 2 - cw / 2, ty + fontSize + 32, 12,
             (Color){0, 220, 255, (unsigned char)(140 + pulse * 100)});

    /* Decorative lines */
    DrawLine(screenW / 2 - tw / 2 - 20, ty + fontSize + 48,
             screenW / 2 + tw / 2 + 20, ty + fontSize + 48,
             (Color){0, 150, 255, 180});
}

/* ── Helper: Robust Asset Texture Loader with Multi-Path Fallback ── */
static Texture2D LoadTextureWithFallback(const char *relPath) {
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

/* ── Menu init ───────────────────────────────────────── */

void Menu_Init(MenuState *ms) {
    memset(ms, 0, sizeof(*ms));

    /* Load character portrait textures with resilient path fallback */
    ms->portraitsLoaded = 0;
    ms->texHarper = LoadTextureWithFallback("assets/portraits/harper.jpg");
    ms->texVance  = LoadTextureWithFallback("assets/portraits/vance.jpg");
    ms->texArchon = LoadTextureWithFallback("assets/portraits/archon.jpg");
    ms->texNadia  = LoadTextureWithFallback("assets/portraits/nadia.jpg");
    ms->texAmara  = LoadTextureWithFallback("assets/portraits/amara.jpg");
    ms->texMira   = LoadTextureWithFallback("assets/portraits/mira.jpg");
    if (ms->texHarper.id > 0) ms->portraitsLoaded = 1;
}

void Menu_Shutdown(MenuState *ms) {
    if (ms->texHarper.id > 0) UnloadTexture(ms->texHarper);
    if (ms->texVance.id  > 0) UnloadTexture(ms->texVance);
    if (ms->texArchon.id > 0) UnloadTexture(ms->texArchon);
    if (ms->texNadia.id  > 0) UnloadTexture(ms->texNadia);
    if (ms->texAmara.id  > 0) UnloadTexture(ms->texAmara);
    if (ms->texMira.id   > 0) UnloadTexture(ms->texMira);
    ms->portraitsLoaded = 0;
}

/* ══════════════════════════════════════════════════════
   MAIN MENU
══════════════════════════════════════════════════════ */

static const char *kMainItems[] = { "START MISSION", "SERVICE RECORD", "CONTROLS", "SETTINGS", "EXIT" };
static const int   kMainCount   = 5;

void Menu_UpdateMain(MenuState *ms, GameContext *ctx, float dt) {
    ms->titleAnim += dt;
    for (int i = 0; i < kMainCount; i++) ms->itemAnim[i] += dt;

    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) {
        ms->selectedItem = (ms->selectedItem - 1 + kMainCount) % kMainCount;
        Game_PlaySFX(SFX_MENU_SELECT);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        ms->selectedItem = (ms->selectedItem + 1) % kMainCount;
        Game_PlaySFX(SFX_MENU_SELECT);
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        Game_PlaySFX(SFX_MENU_CONFIRM);
        switch (ms->selectedItem) {
            case 0: Menu_StartPrologue(ms); Game_ChangeState(ctx, STATE_PROLOGUE); break;
            case 1: Game_ChangeState(ctx, STATE_MEDALS); break;
            case 2: Game_ChangeState(ctx, STATE_CONTROLS); break;
            case 3: Game_ChangeState(ctx, STATE_SETTINGS); break;
            case 4: ctx->running = 0; break;
        }
    }
    /* Mouse */
    int screenW = GetScreenWidth(), screenH = GetScreenHeight();
    int menuY = (int)((float)screenH * 0.48f);
    for (int i = 0; i < kMainCount; i++) {
        int tw = MeasureText(kMainItems[i], 26);
        Rectangle r = { (float)(screenW / 2 - tw / 2 - 30), (float)(menuY + i * 48 - 4),
                        (float)(tw + 60), 38.0f };
        if (CheckCollisionPointRec(GetMousePosition(), r)) {
            if (ms->selectedItem != i) {
                ms->selectedItem = i;
                Game_PlaySFX(SFX_MENU_SELECT);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Game_PlaySFX(SFX_MENU_CONFIRM);
                switch (i) {
                    case 0: Menu_StartPrologue(ms); Game_ChangeState(ctx, STATE_PROLOGUE); break;
                    case 1: Game_ChangeState(ctx, STATE_MEDALS); break;
                    case 2: Game_ChangeState(ctx, STATE_CONTROLS); break;
                    case 3: Game_ChangeState(ctx, STATE_SETTINGS); break;
                    case 4: ctx->running = 0; break;
                }
            }
        }
    }
}

void Menu_DrawMain(const MenuState *ms, const GameContext *ctx,
                   int screenW, int screenH) {
    DrawTitle(screenW, screenH, ms->titleAnim);

    int menuY = (int)((float)screenH * 0.48f);
    for (int i = 0; i < kMainCount; i++) {
        DrawMenuItem(kMainItems[i], screenW / 2, menuY + i * 48,
                     26, ms->selectedItem == i, ms->itemAnim[i]);
    }

    /* High score display */
    if (ctx->highScore > 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "BEST SCORE: %d", ctx->highScore);
        int tw = MeasureText(buf, 15);
        DrawText(buf, screenW / 2 - tw / 2,
                 menuY + kMainCount * 48 + 14, 15, (Color){255, 200, 80, 200});
    }

    /* Controls hint */
    const char *hint = "W/S or UP/DOWN  |  ENTER to select  |  Mouse click supported";
    int hw = MeasureText(hint, 12);
    DrawText(hint, screenW / 2 - hw / 2, screenH - 24, 12,
             (Color){100, 100, 130, 180});
}

/* ══════════════════════════════════════════════════════
   PAUSE MENU
══════════════════════════════════════════════════════ */

static const char *kPauseItems[] = { "RESUME", "RESTART", "SETTINGS", "MAIN MENU", "QUIT GAME" };
static const int   kPauseCount   = 5;

void Menu_UpdatePause(MenuState *ms, GameContext *ctx, float dt) {
    ms->titleAnim += dt;
    for (int i = 0; i < kPauseCount; i++) ms->itemAnim[i] += dt;

    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) {
        ms->selectedItem = (ms->selectedItem - 1 + kPauseCount) % kPauseCount;
        Game_PlaySFX(SFX_MENU_SELECT);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        ms->selectedItem = (ms->selectedItem + 1) % kPauseCount;
        Game_PlaySFX(SFX_MENU_SELECT);
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        Game_PlaySFX(SFX_MENU_CONFIRM);
        Game_ChangeState(ctx, STATE_PLAYING);
        return;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        Game_PlaySFX(SFX_MENU_CONFIRM);
        switch (ms->selectedItem) {
            case 0: Game_ChangeState(ctx, STATE_PLAYING);  break;
            case 1: Game_StartNewGame(ctx); Game_ChangeState(ctx, STATE_PLAYING); break;
            case 2: Game_ChangeState(ctx, STATE_SETTINGS); break;
            case 3: Game_ChangeState(ctx, STATE_MAIN_MENU); break;
            case 4: ctx->running = 0; break;
        }
    }

    /* Mouse support */
    int screenW = GetScreenWidth(), screenH = GetScreenHeight();
    int ph = kPauseCount * 56 + 80;
    int py = screenH / 2 - ph / 2;
    int menuY = py + 66;
    for (int i = 0; i < kPauseCount; i++) {
        int tw = MeasureText(kPauseItems[i], 24);
        Rectangle r = { (float)(screenW / 2 - tw / 2 - 20), (float)(menuY + i * 52 - 4),
                        (float)(tw + 40), 36.0f };
        if (CheckCollisionPointRec(GetMousePosition(), r)) {
            if (ms->selectedItem != i) {
                ms->selectedItem = i;
                Game_PlaySFX(SFX_MENU_SELECT);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Game_PlaySFX(SFX_MENU_CONFIRM);
                switch (i) {
                    case 0: Game_ChangeState(ctx, STATE_PLAYING);  break;
                    case 1: Game_StartNewGame(ctx); Game_ChangeState(ctx, STATE_PLAYING); break;
                    case 2: Game_ChangeState(ctx, STATE_SETTINGS); break;
                    case 3: Game_ChangeState(ctx, STATE_MAIN_MENU); break;
                    case 4: ctx->running = 0; break;
                }
            }
        }
    }
}

void Menu_DrawPause(const MenuState *ms, int screenW, int screenH) {
    /* Dim overlay */
    DrawRectangle(0, 0, screenW, screenH, (Color){0, 0, 20, 160});

    /* Panel */
    int pw = 320, ph = kPauseCount * 56 + 80;
    int px = screenW / 2 - pw / 2;
    int py = screenH / 2 - ph / 2;
    DrawRectangleRounded((Rectangle){(float)px, (float)py, (float)pw, (float)ph},
                         0.12f, 8, (Color){10, 10, 30, 220});
    DrawRectangleRoundedLines((Rectangle){(float)px, (float)py, (float)pw, (float)ph},
                               0.12f, 8, (Color){0, 150, 255, 180});

    /* Title */
    const char *t = "PAUSED";
    int tw = MeasureText(t, 32);
    DrawText(t, screenW / 2 - tw / 2, py + 18, 32, (Color){0, 220, 255, 255});

    int menuY = py + 66;
    for (int i = 0; i < kPauseCount; i++) {
        DrawMenuItem(kPauseItems[i], screenW / 2, menuY + i * 52, 24,
                     ms->selectedItem == i, ms->itemAnim[i]);
    }
}

/* ══════════════════════════════════════════════════════
   GAME OVER
══════════════════════════════════════════════════════ */

static const char *kGoItems[] = { "RESTART", "MAIN MENU", "QUIT GAME" };
static const int   kGoCount   = 3;

void Menu_UpdateGameOver(MenuState *ms, GameContext *ctx, float dt) {
    ms->titleAnim += dt;
    for (int i = 0; i < kGoCount; i++) ms->itemAnim[i] += dt;

    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) {
        ms->selectedItem = (ms->selectedItem - 1 + kGoCount) % kGoCount;
        Game_PlaySFX(SFX_MENU_SELECT);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        ms->selectedItem = (ms->selectedItem + 1) % kGoCount;
        Game_PlaySFX(SFX_MENU_SELECT);
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        Game_PlaySFX(SFX_MENU_CONFIRM);
        switch (ms->selectedItem) {
            case 0: Game_StartNewGame(ctx); Game_ChangeState(ctx, STATE_PLAYING);   break;
            case 1: Game_ChangeState(ctx, STATE_MAIN_MENU); break;
            case 2: ctx->running = 0; break;
        }
    }

    /* Mouse support */
    int screenW = GetScreenWidth(), screenH = GetScreenHeight();
    int ty = (int)((float)screenH * 0.18f);
    int statY = ty + 56 + 36;
    int menuY = statY + 98;
    for (int i = 0; i < kGoCount; i++) {
        int tw = MeasureText(kGoItems[i], 28);
        Rectangle r = { (float)(screenW / 2 - tw / 2 - 20), (float)(menuY + i * 52 - 4),
                        (float)(tw + 40), 40.0f };
        if (CheckCollisionPointRec(GetMousePosition(), r)) {
            if (ms->selectedItem != i) {
                ms->selectedItem = i;
                Game_PlaySFX(SFX_MENU_SELECT);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Game_PlaySFX(SFX_MENU_CONFIRM);
                switch (i) {
                    case 0: Game_StartNewGame(ctx); Game_ChangeState(ctx, STATE_PLAYING); break;
                    case 1: Game_ChangeState(ctx, STATE_MAIN_MENU); break;
                    case 2: ctx->running = 0; break;
                }
            }
        }
    }
}

void Menu_DrawGameOver(const MenuState *ms, const GameContext *ctx,
                       int screenW, int screenH) {
    DrawRectangle(0, 0, screenW, screenH, (Color){0, 0, 10, 200});

    float pulse = 0.5f + 0.5f * sinf(ms->titleAnim * 3.0f);

    const char *goText = "GAME OVER";
    int fontSize = 64;
    int tw = MeasureText(goText, fontSize);
    int ty = (int)((float)screenH * 0.22f);

    DrawText(goText, screenW / 2 - tw / 2 + 2, ty + 2, fontSize,
             (Color){200, 0, 0, (unsigned char)(100 + pulse * 80)});
    DrawText(goText, screenW / 2 - tw / 2, ty, fontSize,
             (Color){255, 50, 50, 255});

    /* Stats */
    char buf[80];
    int statY = ty + fontSize + 30;
    snprintf(buf, sizeof(buf), "SCORE:  %d", ctx->score);
    tw = MeasureText(buf, 24);
    DrawText(buf, screenW / 2 - tw / 2, statY, 24, WHITE);

    snprintf(buf, sizeof(buf), "BEST:   %d", ctx->highScore);
    tw = MeasureText(buf, 20);
    DrawText(buf, screenW / 2 - tw / 2, statY + 32, 20, (Color){255, 200, 80, 220});

    snprintf(buf, sizeof(buf), "WAVE:   %d", ctx->wave);
    tw = MeasureText(buf, 20);
    DrawText(buf, screenW / 2 - tw / 2, statY + 60, 20, (Color){0, 200, 255, 200});

    int menuY = statY + 110;
    for (int i = 0; i < kGoCount; i++) {
        DrawMenuItem(kGoItems[i], screenW / 2, menuY + i * 52, 28,
                     ms->selectedItem == i, ms->itemAnim[i]);
    }
}

/* ══════════════════════════════════════════════════════
   SETTINGS
══════════════════════════════════════════════════════ */

void Menu_UpdateSettings(MenuState *ms, GameContext *ctx, float dt) {
    ms->titleAnim += dt;

    /* 6 settings items + back */
    int count = 7;
    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W))
        ms->selectedItem = (ms->selectedItem - 1 + count) % count;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        ms->selectedItem = (ms->selectedItem + 1) % count;

    GameSettings *s = &ctx->settings;
    float step = 0.05f;

    switch (ms->selectedItem) {
        case 0: /* Master vol */
            if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) s->masterVolume = CLAMP(s->masterVolume - step, 0.0f, 1.0f);
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) s->masterVolume = CLAMP(s->masterVolume + step, 0.0f, 1.0f);
            break;
        case 1: /* Music vol */
            if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) s->musicVolume = CLAMP(s->musicVolume - step, 0.0f, 1.0f);
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) s->musicVolume = CLAMP(s->musicVolume + step, 0.0f, 1.0f);
            break;
        case 2: /* SFX vol */
            if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) s->sfxVolume = CLAMP(s->sfxVolume - step, 0.0f, 1.0f);
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) s->sfxVolume = CLAMP(s->sfxVolume + step, 0.0f, 1.0f);
            break;
        case 3: /* Fullscreen */
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
                s->fullscreen ^= 1;
                if (s->fullscreen) ToggleFullscreen();
                else if (IsWindowFullscreen()) ToggleFullscreen();
            }
            break;
        case 4: /* Screen shake */
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))
                s->screenShake ^= 1;
            break;
        case 5: /* Particle quality */
            if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) s->particleQuality = MAX(0, s->particleQuality - 1);
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) s->particleQuality = MIN(2, s->particleQuality + 1);
            break;
        case 6: /* Back */
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                Settings_Save(s);
                Game_ChangeState(ctx, ctx->prevState);
            }
            break;
        default: break;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        Settings_Save(s);
        Game_ChangeState(ctx, ctx->prevState);
    }
}

static void DrawSlider(int cx, int y, float val, Color col) {
    int bw = 200, bh = 12;
    int bx = cx - bw / 2;
    DrawRectangle(bx, y, bw, bh, (Color){30, 30, 60, 200});
    DrawRectangle(bx, y, (int)(val * bw), bh, col);
    DrawRectangleLines(bx, y, bw, bh, (Color){100, 100, 180, 180});
    char pct[8];
    snprintf(pct, sizeof(pct), "%d%%", (int)(val * 100));
    DrawText(pct, cx + bw / 2 + 8, y, 12, (Color){200, 200, 255, 200});
}

void Menu_DrawSettings(const MenuState *ms, const GameContext *ctx,
                       int screenW, int screenH) {
    DrawRectangle(0, 0, screenW, screenH, (Color){0, 0, 10, 180});

    const char *title = "SETTINGS";
    int tw = MeasureText(title, 36);
    DrawText(title, screenW / 2 - tw / 2, (int)((float)screenH * 0.1f), 36,
             (Color){0, 200, 255, 255});

    const GameSettings *s = &ctx->settings;
    const char *pqLabels[] = { "LOW", "MEDIUM", "HIGH" };

    struct { const char *label; } rows[] = {
        {"MASTER VOLUME"}, {"MUSIC VOLUME"}, {"SFX VOLUME"},
        {"FULLSCREEN"}, {"SCREEN SHAKE"}, {"PARTICLES"}, {"< BACK"}
    };
    int count = 7;
    int startY = (int)((float)screenH * 0.26f);
    int rowH = 58;

    for (int i = 0; i < count; i++) {
        int cy = startY + i * rowH;
        int selected = (ms->selectedItem == i);
        Color labelCol = selected ? (Color){0, 220, 255, 255} : (Color){160, 160, 200, 200};

        int lw = MeasureText(rows[i].label, 18);
        DrawText(rows[i].label, screenW / 2 - lw / 2, cy, 18, labelCol);

        int vy = cy + 22;
        switch (i) {
            case 0: DrawSlider(screenW / 2, vy, s->masterVolume, (Color){0, 200, 255, 255}); break;
            case 1: DrawSlider(screenW / 2, vy, s->musicVolume,  (Color){120, 80, 255, 255}); break;
            case 2: DrawSlider(screenW / 2, vy, s->sfxVolume,    (Color){255, 150, 0, 255});  break;
            case 3: {
                const char *fs = s->fullscreen ? "ON" : "OFF";
                int fsw = MeasureText(fs, 16);
                DrawText(fs, screenW / 2 - fsw / 2, vy, 16, s->fullscreen ? GREEN : RED); break;
            }
            case 4: {
                const char *sh = s->screenShake ? "ON" : "OFF";
                int shw = MeasureText(sh, 16);
                DrawText(sh, screenW / 2 - shw / 2, vy, 16, s->screenShake ? GREEN : RED); break;
            }
            case 5: {
                const char *pq = pqLabels[CLAMP(s->particleQuality, 0, 2)];
                int pqw = MeasureText(pq, 16);
                DrawText(pq, screenW / 2 - pqw / 2, vy, 16, (Color){0, 220, 160, 255}); break;
            }
            default: break;
        }
    }

    const char *hint = "UP/DOWN navigate  |  LEFT/RIGHT adjust  |  ENTER confirm  |  ESC back";
    int hw = MeasureText(hint, 12);
    DrawText(hint, screenW / 2 - hw / 2, screenH - 24, 12, (Color){100, 100, 130, 180});
}


/* ══════════════════════════════════════════════════════
   CONTROLS SCREEN
══════════════════════════════════════════════════════ */

void Menu_UpdateControls(MenuState *ms, GameContext *ctx, float dt) {
    ms->titleAnim += dt;
    /* Any key or click returns to main menu */
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) ||
        IsKeyPressed(KEY_SPACE)  || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Game_ChangeState(ctx, STATE_MAIN_MENU);
    }
}

void Menu_DrawControls(const MenuState *ms, int screenW, int screenH) {
    (void)ms;

    DrawRectangle(0, 0, screenW, screenH, (Color){0, 0, 10, 210});

    /* Title */
    const char *title = "CONTROLS";
    int tw = MeasureText(title, 40);
    DrawText(title, screenW / 2 - tw / 2, (int)((float)screenH * 0.08f), 40,
             (Color){0, 200, 255, 255});

    /* Decorative line */
    int lineY = (int)((float)screenH * 0.08f) + 48;
    DrawLine(screenW / 2 - 180, lineY, screenW / 2 + 180, lineY,
             (Color){0, 150, 255, 160});

    /* Panel background */
    int panW = 580, panH = 410;
    int panX = screenW / 2 - panW / 2;
    int panY = (int)((float)screenH * 0.16f);
    DrawRectangleRounded((Rectangle){(float)panX, (float)panY, (float)panW, (float)panH},
                         0.08f, 8, (Color){8, 8, 28, 200});
    DrawRectangleRoundedLines((Rectangle){(float)panX, (float)panY, (float)panW, (float)panH},
                               0.08f, 8, (Color){0, 100, 200, 140});

    /* Column layout: KEY on left, ACTION on right */
    int col1 = panX + 28;          /* key label */
    int col2 = panX + 230;         /* action label */
    int rowH  = 32;
    int startY = panY + 20;
    int fs    = 17;

    Color keyCol    = (Color){0, 220, 255, 255};
    Color actCol    = (Color){200, 200, 220, 230};
    Color headCol   = (Color){255, 200, 0, 255};
    Color divCol    = (Color){0, 80, 160, 120};

    /* ── Movement section ── */
    DrawText("FLIGHT & PROPULSION", col1, startY, 14, headCol);
    DrawLine(col1, startY + 18, panX + panW - 28, startY + 18, divCol);
    startY += 24;

    struct { const char *key; const char *action; } rows[] = {
        { "W  /  UP ARROW",       "Thrust forward (Nitro)"  },
        { "SHIFT  /  RMB",        "Hyper-Nitro Overdrive"   },
        { "S  /  DOWN ARROW",     "Brake / reverse thrusters" },
        { "A / D  (or ARROWS)",   "Rotate & 2.5D visual bank" },
    };
    for (int i = 0; i < 4; i++) {
        DrawText(rows[i].key,    col1, startY + i * rowH, fs, keyCol);
        DrawText(rows[i].action, col2, startY + i * rowH, fs, actCol);
    }
    startY += 4 * rowH + 8;

    /* ── Combat section ── */
    DrawText("COMBAT SYSTEMS", col1, startY, 14, headCol);
    DrawLine(col1, startY + 18, panX + panW - 28, startY + 18, divCol);
    startY += 24;

    struct { const char *key; const char *action; } combat[] = {
        { "SPACE / LEFT CLICK",  "Twin Wingtip Cannons" },
        { "ESC",                 "Pause / leave game"   },
        { "F1",                  "Toggle controls hint" },
    };
    for (int i = 0; i < 3; i++) {
        DrawText(combat[i].key,    col1, startY + i * rowH, fs, keyCol);
        DrawText(combat[i].action, col2, startY + i * rowH, fs, actCol);
    }
    startY += 3 * rowH + 10;

    /* ── Menu navigation section ── */
    DrawText("MENUS", col1, startY, 14, headCol);
    DrawLine(col1, startY + 18, panX + panW - 30, startY + 18, divCol);
    startY += 26;

    struct { const char *key; const char *action; } menus[] = {
        { "W/S  or  UP/DOWN", "Navigate items"  },
        { "ENTER  or  SPACE", "Confirm / select" },
        { "ESC",              "Back / pause"     },
        { "Mouse click",      "Also works everywhere" },
    };
    for (int i = 0; i < 4; i++) {
        DrawText(menus[i].key,    col1, startY + i * rowH, fs, keyCol);
        DrawText(menus[i].action, col2, startY + i * rowH, fs, actCol);
    }

    /* Back hint */
    const char *back = "Press  ESC / ENTER / SPACE / CLICK  to go back";
    int bw = MeasureText(back, 13);
    DrawText(back, screenW / 2 - bw / 2, screenH - 30, 13,
             (Color){120, 120, 160, 200});
}

/* ══════════════════════════════════════════════════════
   PROLOGUE STORY CRAWL (STAR WARS STYLE)
══════════════════════════════════════════════════════ */

typedef struct {
    const char *text;
    int         fontSize;
    Color       color;
    int         spacingAfter;
} CrawlLine;

static const CrawlLine kCrawl[] = {
    { "A LONG TIME AGO IN THE DEEP VOID...", 18, { 0, 220, 255, 255 }, 44 },
    { "SPACE JAM", 64, { 255, 215, 0, 255 }, 12 },
    { "EPISODE I: THE LAST VECTOR", 22, { 255, 180, 0, 255 }, 52 },
    { "The peaceful Free Colonies have fallen.", 22, { 255, 230, 140, 255 }, 26 },
    { "A ruthless galactic empire known as the", 20, { 235, 235, 250, 255 }, 8 },
    { "IRON SYNOD has crushed the defense fleet", 20, { 235, 235, 250, 255 }, 8 },
    { "with an endless armada of mass-produced war machines.", 20, { 235, 235, 250, 255 }, 30 },
    { "Only one advanced prototype starfighter escaped the orbital purge:", 20, { 235, 235, 250, 255 }, 8 },
    { "the HK-01 VANGUARD, commanded by ace test pilot", 20, { 0, 220, 255, 255 }, 8 },
    { "HARPER KOLLINS.", 24, { 255, 215, 0, 255 }, 30 },
    { "Outnumbered and deep behind enemy lines, you are the last", 20, { 235, 235, 250, 255 }, 8 },
    { "line of defense protecting an unarmed convoy of civilian", 20, { 235, 235, 250, 255 }, 8 },
    { "refugees fleeing toward the Outer Rim.", 20, { 235, 235, 250, 255 }, 38 },
    { "Power up your sub-light engines.", 22, { 255, 140, 40, 255 }, 8 },
    { "Clear a path through the asteroid fields,", 22, { 255, 140, 40, 255 }, 8 },
    { "and tear down the Imperial blockade...", 24, { 255, 80, 80, 255 }, 80 },
};
#define CRAWL_LINE_COUNT (sizeof(kCrawl) / sizeof(kCrawl[0]))

void Menu_StartPrologue(MenuState *ms) {
    ms->prologueScroll = 0.0f;
}

void Menu_UpdatePrologue(MenuState *ms, GameContext *ctx, float dt) {
    ms->prologueScroll += dt * 36.0f; /* Smooth, readable scroll */

    /* Skip on any key or click -> transition to tactical comms transmission */
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
        IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Menu_StartComms(ms);
        Game_ChangeState(ctx, STATE_COMMS);
        return;
    }

    /* Auto-launch comms once crawl finishes */
    float totalHeight = 0.0f;
    for (size_t i = 0; i < CRAWL_LINE_COUNT; i++) {
        totalHeight += (float)kCrawl[i].fontSize + (float)kCrawl[i].spacingAfter;
    }
    if (ms->prologueScroll > totalHeight + (float)GetScreenHeight() * 0.7f) {
        Menu_StartComms(ms);
        Game_ChangeState(ctx, STATE_COMMS);
    }
}

void Menu_DrawPrologue(const MenuState *ms, int screenW, int screenH) {
    /* Dim overlay so stars in background remain visible */
    DrawRectangle(0, 0, screenW, screenH, (Color){ 2, 2, 10, 185 });

    float curY = (float)screenH * 0.75f - ms->prologueScroll;

    for (size_t i = 0; i < CRAWL_LINE_COUNT; i++) {
        const CrawlLine *cl = &kCrawl[i];
        float lineY = curY;
        curY += (float)cl->fontSize + (float)cl->spacingAfter;

        /* Skip off-screen lines */
        if (lineY < -60.0f || lineY > (float)screenH + 40.0f) continue;

        /* Cinematic edge fading near top and bottom */
        float alpha = 1.0f;
        if (lineY < (float)screenH * 0.22f) {
            alpha = CLAMP((lineY + 40.0f) / ((float)screenH * 0.26f), 0.0f, 1.0f);
        } else if (lineY > (float)screenH * 0.85f) {
            alpha = CLAMP(((float)screenH - lineY) / ((float)screenH * 0.15f), 0.0f, 1.0f);
        }

        Color col = ColorAlpha(cl->color, alpha);
        int tw = MeasureText(cl->text, cl->fontSize);
        DrawText(cl->text, screenW / 2 - tw / 2, (int)lineY, cl->fontSize, col);
    }

    /* Skip / Launch prompt at bottom */
    float pulse = 0.5f + 0.5f * sinf(GetTime() * 4.0f);
    const char *prompt = "[ PRESS SPACE / ENTER / CLICK TO LAUNCH ]";
    int pw = MeasureText(prompt, 14);
    DrawText(prompt, screenW / 2 - pw / 2, screenH - 32, 14,
             (Color){ 0, 220, 255, (unsigned char)(140 + pulse * 115) });
}

/* ══════════════════════════════════════════════════════
   TACTICAL RADIO COMMUNICATIONS CUTSCENE
══════════════════════════════════════════════════════ */

typedef enum {
    SPEAKER_VANCE = 0,
    SPEAKER_HARPER,
    SPEAKER_ARCHON,
    SPEAKER_NADIA,
    SPEAKER_AMARA,
    SPEAKER_MIRA
} SpeakerId;

typedef struct {
    SpeakerId   speakerId;
    const char *speaker;
    const char *callsign;
    const char *text;
    int         onRight; /* 0 = left panel, 1 = right panel */
} CommsDialogue;

static const CommsDialogue kCommsDialogue[] = {
    { SPEAKER_VANCE, "COMMANDER VANCE", "FLEET COMMAND // TERRA NOVUS DEFENSE",
      "Vanguard-1, do you copy? Harper, do you read me?", 0 },

    { SPEAKER_HARPER, "HARPER KOLLINS", "CALL-SIGN: VANGUARD-1 // STRIKE CRAFT HK-01",
      "Loud and clear, Command. Atmospheric descent complete. What is the tactical situation?", 1 },

    { SPEAKER_VANCE, "COMMANDER VANCE", "FLEET COMMAND // TERRA NOVUS DEFENSE",
      "Catastrophic breach! The Iron Synod armada has broken our orbital defense grid. Cities are burning.", 0 },

    { SPEAKER_NADIA, "NADIA", "EARTH CIVILIAN CORPS // FREQUENCY INTERCEPT",
      "Harper, be careful! These Synod signatures aren't standard invaders—they are scanning for something ancient!", 0 },

    { SPEAKER_HARPER, "HARPER KOLLINS", "CALL-SIGN: VANGUARD-1 // STRIKE CRAFT HK-01",
      "Nadia, get to the evacuation transport with Mira and Mom! I'm going to hold the orbital corridor open.", 1 },

    { SPEAKER_VANCE, "COMMANDER VANCE", "FLEET COMMAND // TERRA NOVUS DEFENSE",
      "Refugee shuttles are launching through the firestorm! Synod hunter wings are vectoring to intercept. Protect those transports, Ace!", 0 },

    { SPEAKER_HARPER, "HARPER KOLLINS", "CALL-SIGN: VANGUARD-1 // STRIKE CRAFT HK-01",
      "Weapons hot. Twin wingtip autocannons primed. No Imperial warship touches those shuttles. Engaging now!", 1 },
};
#define COMMS_TOTAL_LINES 7

static void DrawSpeakerPortrait(const MenuState *ms, SpeakerId speaker,
                                const char *name, const char *role,
                                int x, int y, int w, int h,
                                int isActive, float freqAnim) {
    Texture2D tex = {0};
    Color primary = (Color){0, 220, 255, 255};
    Color subCol  = (Color){100, 150, 200, 180};

    switch (speaker) {
        case SPEAKER_VANCE:
            tex = ms->texVance;
            primary = (Color){0, 220, 255, 255};
            subCol  = (Color){100, 150, 200, 180};
            break;
        case SPEAKER_HARPER:
            tex = ms->texHarper;
            primary = (Color){255, 200, 40, 255};
            subCol  = (Color){200, 180, 100, 180};
            break;
        case SPEAKER_ARCHON:
            tex = ms->texArchon;
            primary = (Color){255, 60, 80, 255};
            subCol  = (Color){200, 100, 140, 180};
            break;
        case SPEAKER_NADIA:
            tex = ms->texNadia;
            primary = (Color){255, 175, 90, 255};
            subCol  = (Color){220, 150, 120, 180};
            break;
        case SPEAKER_AMARA:
            tex = ms->texAmara;
            primary = (Color){180, 130, 255, 255};
            subCol  = (Color){160, 140, 210, 180};
            break;
        case SPEAKER_MIRA:
            tex = ms->texMira;
            primary = (Color){0, 240, 200, 255};
            subCol  = (Color){120, 200, 180, 180};
            break;
    }

    Color borderCol = isActive ? primary : (Color){60, 80, 110, 140};
    DrawRectangleRounded((Rectangle){(float)x, (float)y, (float)w, (float)h}, 0.08f, 6,
                         (Color){6, 12, 28, 230});
    DrawRectangleRoundedLines((Rectangle){(float)x, (float)y, (float)w, (float)h}, 0.08f, 6, borderCol);

    /* Portrait Image or animated Waveform fallback */
    if (ms->portraitsLoaded && tex.id > 0) {
        int imgSize = h - 40;
        float scale = (float)imgSize / (float)(tex.height > 0 ? tex.height : 1);
        int dstW = (int)((float)tex.width * scale);
        if (dstW > w - 8) { scale = (float)(w - 8) / (float)tex.width; dstW = w - 8; }
        int dstH = (int)((float)tex.height * scale);
        int imgX = x + w / 2 - dstW / 2;
        Color tint = isActive ? WHITE : (Color){120, 140, 160, 170};
        DrawTexturePro(tex,
                       (Rectangle){0, 0, (float)tex.width, (float)tex.height},
                       (Rectangle){(float)imgX, (float)(y + 4), (float)dstW, (float)dstH},
                       (Vector2){0, 0}, 0.0f, tint);
    } else {
        for (int bar = 0; bar < 32; bar++) {
            float waveH = isActive ? fabsf(sinf(freqAnim + (float)bar * 0.45f)) * 28.0f + 4.0f : 4.0f;
            int wx = x + 24 + bar * 7;
            DrawLine(wx, y + 95 - (int)(waveH * 0.5f), wx, y + 95 + (int)(waveH * 0.5f),
                     isActive ? primary : (Color){40, 70, 100, 140});
        }
    }

    /* Nameplate */
    DrawText(name, x + 8, y + h - 36, 13, isActive ? primary : (Color){140, 160, 180, 180});
    DrawText(role, x + 8, y + h - 20, 10, subCol);

    /* Active Pulse Border */
    if (isActive) {
        float pulse = 0.5f + 0.5f * sinf(freqAnim * 1.6f);
        DrawRectangleRoundedLines(
            (Rectangle){(float)(x - 2), (float)(y - 2), (float)(w + 4), (float)(h + 4)},
            0.08f, 6, ColorAlpha(primary, 0.4f + pulse * 0.5f));
    }
}

void Menu_StartComms(MenuState *ms) {
    ms->comms.currentLine   = 0;
    ms->comms.charTimer     = 0.0f;
    ms->comms.charsRevealed = 0;
    ms->comms.beepTimer     = 0.0f;
    ms->comms.freqAnim      = 0.0f;
    Game_PlaySFX(SFX_RADIO_CHIRP);
    Game_PlaySFX(SFX_RADIO_STATIC);
}

void Menu_UpdateComms(MenuState *ms, GameContext *ctx, float dt) {
    ms->comms.freqAnim += dt * 8.0f;

    if (ms->comms.currentLine >= COMMS_TOTAL_LINES) {
        Game_PlaySFX(SFX_WARP_JUMP);
        Game_StartNewGame(ctx);
        Game_ChangeState(ctx, STATE_PLAYING);
        return;
    }

    const CommsDialogue *cd = &kCommsDialogue[ms->comms.currentLine];
    int textLen = (int)strlen(cd->text);

    /* Typewriter text advance */
    if (ms->comms.charsRevealed < textLen) {
        ms->comms.charTimer += dt;
        while (ms->comms.charTimer >= 0.022f && ms->comms.charsRevealed < textLen) {
            ms->comms.charTimer -= 0.022f;
            ms->comms.charsRevealed++;
            if (ms->comms.charsRevealed % 3 == 0) {
                Game_PlaySFX(SFX_RADIO_BEEP);
            }
        }
    }

    /* Input: advance dialogue */
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (ms->comms.charsRevealed < textLen) {
            /* Reveal whole line immediately */
            ms->comms.charsRevealed = textLen;
        } else {
            /* Advance to next dialogue line */
            ms->comms.currentLine++;
            ms->comms.charsRevealed = 0;
            ms->comms.charTimer     = 0.0f;
            if (ms->comms.currentLine < COMMS_TOTAL_LINES) {
                Game_PlaySFX(SFX_RADIO_CHIRP);
            } else {
                Game_PlaySFX(SFX_WARP_JUMP);
                Game_StartNewGame(ctx);
                Game_ChangeState(ctx, STATE_PLAYING);
            }
        }
    }

    /* ESC: skip directly to gameplay */
    if (IsKeyPressed(KEY_ESCAPE)) {
        Game_PlaySFX(SFX_WARP_JUMP);
        Game_StartNewGame(ctx);
        Game_ChangeState(ctx, STATE_PLAYING);
    }
}

void Menu_DrawComms(const MenuState *ms, int screenW, int screenH) {
    /* Dim overlay */
    DrawRectangle(0, 0, screenW, screenH, (Color){ 2, 4, 14, 220 });

    if (ms->comms.currentLine >= COMMS_TOTAL_LINES) return;
    const CommsDialogue *cd = &kCommsDialogue[ms->comms.currentLine];

    /* 1. Header Bar: Frequency & Security Clearance */
    int topY = (int)((float)screenH * 0.08f);
    DrawRectangle(0, topY - 14, screenW, 36, (Color){ 4, 12, 28, 220 });
    DrawLine(0, topY + 22, screenW, topY + 22, (Color){ 0, 180, 255, 140 });

    const char *headerTitle = "TACTICAL COMMS // MILITARY FREQUENCY 104.8 MHz // [ QUANTUM ENCRYPTED ]";
    int htw = MeasureText(headerTitle, 14);
    DrawText(headerTitle, screenW / 2 - htw / 2, topY - 5, 14, (Color){ 0, 240, 200, 255 });

    /* 2. Speaker Portrait Panels (Incoming on Left, Harper on Right) */
    int panW = 260, panH = 190;
    int panY = (int)((float)screenH * 0.17f);
    int leftX = (int)((float)screenW * 0.10f);
    int rightX = screenW - leftX - panW;

    /* Determine speaker on left panel: if current dialogue is Nadia, show Nadia; else Vance */
    SpeakerId leftSpeaker = (cd->speakerId == SPEAKER_NADIA) ? SPEAKER_NADIA :
                            (cd->speakerId == SPEAKER_ARCHON) ? SPEAKER_ARCHON : SPEAKER_VANCE;
    const char *leftName  = (cd->speakerId == SPEAKER_NADIA) ? "NADIA" :
                            (cd->speakerId == SPEAKER_ARCHON) ? "THE IRON ARCHON" : "COMMANDER VANCE";
    const char *leftRole  = (cd->speakerId == SPEAKER_NADIA) ? "CIVILIAN OPS // EARTH" :
                            (cd->speakerId == SPEAKER_ARCHON) ? "SUPREME RULER // SYNOD" : "FLEET HQ // SECTOR 1";
    int leftActive        = (cd->onRight == 0);

    DrawSpeakerPortrait(ms, leftSpeaker, leftName, leftRole,
                        leftX, panY, panW, panH, leftActive, ms->comms.freqAnim);

    /* Right Panel: Harper Kollins */
    int rightActive = (cd->onRight == 1);
    DrawSpeakerPortrait(ms, SPEAKER_HARPER, "HARPER KOLLINS", "PILOT // VANGUARD-1",
                        rightX, panY, panW, panH, rightActive, ms->comms.freqAnim);

    /* 3. Center Dialogue Box */
    int boxW = (int)((float)screenW * 0.76f);
    int boxH = 185;
    int boxX = screenW / 2 - boxW / 2;
    int boxY = (int)((float)screenH * 0.53f);

    DrawRectangleRounded((Rectangle){ (float)boxX, (float)boxY, (float)boxW, (float)boxH }, 0.06f, 6,
                         (Color){ 4, 8, 22, 240 });
    DrawRectangleRoundedLines((Rectangle){ (float)boxX, (float)boxY, (float)boxW, (float)boxH }, 0.06f, 6,
                              cd->onRight ? (Color){ 255, 180, 30, 200 } : (Color){ 0, 180, 255, 200 });

    /* Speaker Badge */
    DrawText(cd->speaker,  boxX + 24, boxY + 12, 16,
             cd->onRight ? (Color){ 255, 215, 0, 255 } : (Color){ 0, 220, 255, 255 });
    DrawText(cd->callsign, boxX + 24, boxY + 32, 11, (Color){ 140, 160, 190, 180 });
    DrawLine(boxX + 20, boxY + 50, boxX + boxW - 20, boxY + 50, (Color){ 30, 60, 100, 180 });

    /* Typewriter Substring — word-wrapped across up to 3 lines */
    char subBuf[512];
    int len = ms->comms.charsRevealed;
    if (len > (int)sizeof(subBuf) - 1) len = (int)sizeof(subBuf) - 1;
    strncpy(subBuf, cd->text, (size_t)len);
    subBuf[len] = '\0';

    /* Manual word-wrap: split into lines of at most ~maxCharsPerLine chars */
    int textFs   = 17;
    int maxWidth = boxW - 52; /* usable text width */
    int textX    = boxX + 26;
    int textY    = boxY + 60;
    int lineH    = textFs + 6;

    /* Simple greedy word-wrap */
    char wrapped[512];
    int wlen = 0;
    int lineStart = 0;
    for (int i = 0; i <= len; i++) {
        if (i == len || subBuf[i] == ' ' || subBuf[i] == '\0') {
            /* Check if this word fits */
            int segLen = i - lineStart;
            char testBuf[512];
            if (wlen + segLen < (int)sizeof(testBuf)) {
                strncpy(testBuf, wrapped, wlen);
                strncpy(testBuf + wlen, subBuf + lineStart, segLen);
                testBuf[wlen + segLen] = '\0';
                if (MeasureText(testBuf, textFs) > maxWidth) {
                    /* Wrap: replace last space with newline */
                    if (wlen > 0 && wrapped[wlen - 1] == ' ') wrapped[wlen - 1] = '\n';
                }
            }
            /* Copy word + space */
            for (int k = lineStart; k < i && wlen < 510; k++) wrapped[wlen++] = subBuf[k];
            if (i < len && wlen < 510) wrapped[wlen++] = subBuf[i]; /* space */
            lineStart = i + 1;
        }
    }
    wrapped[wlen] = '\0';

    /* Draw each line */
    const char *p = wrapped;
    int lineIdx = 0;
    while (*p && lineIdx < 5) {
        char lineBuf[256];
        int li = 0;
        while (*p && *p != '\n' && li < (int)sizeof(lineBuf) - 1) {
            lineBuf[li++] = *p++;
        }
        lineBuf[li] = '\0';
        if (*p == '\n') p++;
        DrawText(lineBuf, textX, textY + lineIdx * lineH, textFs, (Color){ 235, 245, 255, 255 });
        lineIdx++;
    }

    /* Blinking Terminal Cursor after last char */
    if ((int)(GetTime() * 4.0f) % 2 == 0 && lineIdx > 0) {
        /* Place cursor at end of last line drawn */
        DrawRectangle(textX + 4, textY + (lineIdx - 1) * lineH + textFs - 4, 8, 16,
                      (Color){ 0, 240, 200, 240 });
    }

    /* 4. Navigation Prompt */
    float pulse = 0.5f + 0.5f * sinf(GetTime() * 5.0f);
    const char *prompt = "[ SPACE / ENTER / CLICK ] ADVANCE   |   [ ESC ] LAUNCH TO BATTLE";
    int pw = MeasureText(prompt, 13);
    DrawText(prompt, screenW / 2 - pw / 2, screenH - 36, 13,
             (Color){ 0, 220, 255, (unsigned char)(140 + pulse * 115) });
}

/* ══════════════════════════════════════════════════════
   MILITARY SERVICE RECORD & MEDALS SHOWCASE
══════════════════════════════════════════════════════ */

void Menu_UpdateMedals(MenuState *ms, GameContext *ctx, float dt) {
    (void)dt;
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
        Game_PlaySFX(SFX_MENU_SELECT);
        Game_ChangeState(ctx, STATE_MAIN_MENU);
        return;
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        ms->selectedMedal = (ms->selectedMedal - 1 + MEDAL_COUNT) % MEDAL_COUNT;
        Game_PlaySFX(SFX_MENU_SELECT);
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        ms->selectedMedal = (ms->selectedMedal + 1) % MEDAL_COUNT;
        Game_PlaySFX(SFX_MENU_SELECT);
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        ms->selectedMedal = (ms->selectedMedal - 4 + MEDAL_COUNT) % MEDAL_COUNT;
        Game_PlaySFX(SFX_MENU_SELECT);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        ms->selectedMedal = (ms->selectedMedal + 4) % MEDAL_COUNT;
        Game_PlaySFX(SFX_MENU_SELECT);
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        Game_ChangeState(ctx, STATE_MAIN_MENU);
    }
}

void Menu_DrawMedals(const MenuState *ms, const PlayerProgress *prog, int screenW, int screenH) {
    DrawRectangle(0, 0, screenW, screenH, (Color){ 2, 4, 14, 225 });

    /* Title */
    const char *title = "MILITARY SERVICE RECORD & CAMPAIGN HONORS";
    int tw = MeasureText(title, 28);
    DrawText(title, screenW / 2 - tw / 2, (int)((float)screenH * 0.06f), 28, (Color){ 0, 220, 255, 255 });

    /* Pilot Rank Callout */
    const char *rank = Progress_GetRankName(prog);
    char rankBuf[128];
    snprintf(rankBuf, sizeof(rankBuf), "PILOT: HARPER KOLLINS  //  RANK: %s", rank);
    int rw = MeasureText(rankBuf, 16);
    DrawText(rankBuf, screenW / 2 - rw / 2, (int)((float)screenH * 0.06f) + 38, 16, (Color){ 255, 215, 0, 255 });

    /* Lifetime Stats Bar */
    int statBoxW = (int)((float)screenW * 0.82f);
    int statBoxH = 50;
    int statBoxX = screenW / 2 - statBoxW / 2;
    int statBoxY = (int)((float)screenH * 0.16f);
    DrawRectangleRounded((Rectangle){ (float)statBoxX, (float)statBoxY, (float)statBoxW, (float)statBoxH },
                         0.1f, 6, (Color){ 8, 14, 30, 220 });
    DrawRectangleRoundedLines((Rectangle){ (float)statBoxX, (float)statBoxY, (float)statBoxW, (float)statBoxH },
                              0.1f, 6, (Color){ 0, 140, 220, 160 });

    char s1[64], s2[64], s3[64], s4[64];
    snprintf(s1, sizeof(s1), "SYNOD KILLS: %d", prog->totalKills);
    snprintf(s2, sizeof(s2), "SHUTTLES SAVED: %d", prog->totalShuttlesSaved);
    snprintf(s3, sizeof(s3), "BOSSES CRUSHED: %d", prog->totalBossesDefeated);
    snprintf(s4, sizeof(s4), "BEST SCORE: %d", prog->highestScore);

    int colW = statBoxW / 4;
    DrawText(s1, statBoxX + colW * 0 + 16, statBoxY + 18, 13, (Color){ 0, 240, 200, 240 });
    DrawText(s2, statBoxX + colW * 1 + 16, statBoxY + 18, 13, (Color){ 0, 240, 200, 240 });
    DrawText(s3, statBoxX + colW * 2 + 16, statBoxY + 18, 13, (Color){ 0, 240, 200, 240 });
    DrawText(s4, statBoxX + colW * 3 + 16, statBoxY + 18, 13, (Color){ 255, 215, 0, 255 });

    /* Medals Grid (2 rows of 4 medals) */
    int gridStartY = (int)((float)screenH * 0.28f);
    int mCardW = (statBoxW - 3 * 16) / 4;
    int mCardH = 110;

    for (int i = 0; i < MEDAL_COUNT; i++) {
        int row = i / 4;
        int col = i % 4;
        int mx = statBoxX + col * (mCardW + 16);
        int my = gridStartY + row * (mCardH + 18);

        int unlocked = Progress_HasMedal(prog, (MedalId)i);
        int isSelected = (ms->selectedMedal == i);

        Color cardBg = unlocked ? (Color){ 12, 22, 44, 230 } : (Color){ 6, 8, 16, 210 };
        Color cardLine = isSelected ? (Color){ 0, 240, 255, 255 } :
                         unlocked   ? (Color){ 255, 200, 30, 180 } : (Color){ 40, 50, 70, 140 };

        DrawRectangleRounded((Rectangle){ (float)mx, (float)my, (float)mCardW, (float)mCardH }, 0.08f, 6, cardBg);
        DrawRectangleRoundedLines((Rectangle){ (float)mx, (float)my, (float)mCardW, (float)mCardH }, 0.08f, 6, cardLine);

        /* Medal Badge Icon */
        const char *icon = Progress_GetMedalIcon((MedalId)i);
        DrawText(icon, mx + 14, my + 14, 18, unlocked ? (Color){ 255, 215, 0, 255 } : (Color){ 80, 90, 110, 180 });

        /* Medal Title */
        const char *mTitle = Progress_GetMedalTitle((MedalId)i);
        DrawText(mTitle, mx + 14, my + 42, 12, unlocked ? (Color){ 0, 220, 255, 255 } : (Color){ 120, 130, 150, 180 });

        /* Status label */
        const char *status = unlocked ? "[ AWARDED ]" : "[ LOCKED ]";
        DrawText(status, mx + 14, my + 78, 11, unlocked ? (Color){ 0, 255, 160, 240 } : (Color){ 160, 60, 60, 200 });
    }

    /* Selected Medal Detail Box at bottom */
    int detailY = gridStartY + 2 * (mCardH + 18) + 10;
    int detailH = 65;
    DrawRectangleRounded((Rectangle){ (float)statBoxX, (float)detailY, (float)statBoxW, (float)detailH },
                         0.08f, 6, (Color){ 8, 14, 28, 230 });
    DrawRectangleRoundedLines((Rectangle){ (float)statBoxX, (float)detailY, (float)statBoxW, (float)detailH },
                              0.08f, 6, (Color){ 0, 180, 255, 140 });

    const char *selTitle = Progress_GetMedalTitle((MedalId)ms->selectedMedal);
    const char *selDesc  = Progress_GetMedalDesc((MedalId)ms->selectedMedal);
    DrawText(selTitle, statBoxX + 18, detailY + 12, 15, (Color){ 255, 215, 0, 255 });
    DrawText(selDesc,  statBoxX + 18, detailY + 36, 13, (Color){ 200, 220, 245, 220 });

    /* Back Prompt */
    const char *bPrompt = "[ ARROWS / WASD ] SELECT MEDAL   |   [ ESC / ENTER / CLICK ] BACK TO MAIN MENU";
    int bpw = MeasureText(bPrompt, 13);
    DrawText(bPrompt, screenW / 2 - bpw / 2, screenH - 28, 13, (Color){ 140, 160, 200, 190 });
}
