/**
 * menu.c - Menu screens (Main, Pause, Game Over, Settings)
 */
#include "menu.h"
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
    const char *sub = "ARCADE SPACE SURVIVAL";
    int sw = MeasureText(sub, 18);
    DrawText(sub, screenW / 2 - sw / 2, ty + fontSize + 8, 18,
             (Color){100, 200, 255, (unsigned char)(150 + pulse * 100)});

    /* Decorative lines */
    DrawLine(screenW / 2 - tw / 2 - 20, ty + fontSize + 34,
             screenW / 2 + tw / 2 + 20, ty + fontSize + 34,
             (Color){0, 150, 255, 180});
}

/* ── Menu init ───────────────────────────────────────── */

void Menu_Init(MenuState *ms) {
    memset(ms, 0, sizeof(*ms));
}

/* ══════════════════════════════════════════════════════
   MAIN MENU
══════════════════════════════════════════════════════ */

static const char *kMainItems[] = { "START GAME", "CONTROLS", "SETTINGS", "EXIT" };
static const int   kMainCount   = 4;

void Menu_UpdateMain(MenuState *ms, GameContext *ctx, float dt) {
    ms->titleAnim += dt;
    for (int i = 0; i < kMainCount; i++) ms->itemAnim[i] += dt;

    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) {
        ms->selectedItem = (ms->selectedItem - 1 + kMainCount) % kMainCount;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        ms->selectedItem = (ms->selectedItem + 1) % kMainCount;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        switch (ms->selectedItem) {
            case 0: Game_StartNewGame(ctx); Game_ChangeState(ctx, STATE_PLAYING); break;
            case 1: Game_ChangeState(ctx, STATE_CONTROLS); break;
            case 2: Game_ChangeState(ctx, STATE_SETTINGS); break;
            case 3: ctx->running = 0; break;
        }
    }
    /* Mouse */
    int screenW = GetScreenWidth(), screenH = GetScreenHeight();
    int menuY = (int)((float)screenH * 0.50f);
    for (int i = 0; i < kMainCount; i++) {
        int tw = MeasureText(kMainItems[i], 28);
        Rectangle r = { (float)(screenW / 2 - tw / 2 - 30), (float)(menuY + i * 52 - 4),
                        (float)(tw + 60), 40.0f };
        if (CheckCollisionPointRec(GetMousePosition(), r)) {
            ms->selectedItem = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                switch (i) {
                    case 0: Game_StartNewGame(ctx); Game_ChangeState(ctx, STATE_PLAYING); break;
                    case 1: Game_ChangeState(ctx, STATE_CONTROLS); break;
                    case 2: Game_ChangeState(ctx, STATE_SETTINGS); break;
                    case 3: ctx->running = 0; break;
                }
            }
        }
    }
}

void Menu_DrawMain(const MenuState *ms, const GameContext *ctx,
                   int screenW, int screenH) {
    DrawTitle(screenW, screenH, ms->titleAnim);

    int menuY = (int)((float)screenH * 0.50f);
    for (int i = 0; i < kMainCount; i++) {
        DrawMenuItem(kMainItems[i], screenW / 2, menuY + i * 52,
                     28, ms->selectedItem == i, ms->itemAnim[i]);
    }

    /* High score display */
    if (ctx->highScore > 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "BEST: %d", ctx->highScore);
        int tw = MeasureText(buf, 16);
        DrawText(buf, screenW / 2 - tw / 2,
                 menuY + kMainCount * 52 + 20, 16, (Color){255, 200, 80, 200});
    }

    /* Controls hint */
    const char *hint = "W/S or UP/DOWN  |  ENTER to select";
    int hw = MeasureText(hint, 13);
    DrawText(hint, screenW / 2 - hw / 2, screenH - 28, 13,
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

    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W))
        ms->selectedItem = (ms->selectedItem - 1 + kPauseCount) % kPauseCount;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        ms->selectedItem = (ms->selectedItem + 1) % kPauseCount;
    if (IsKeyPressed(KEY_ESCAPE)) {
        Game_ChangeState(ctx, STATE_PLAYING);
        return;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
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
            ms->selectedItem = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
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

    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W))
        ms->selectedItem = (ms->selectedItem - 1 + kGoCount) % kGoCount;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        ms->selectedItem = (ms->selectedItem + 1) % kGoCount;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        switch (ms->selectedItem) {
            case 0: Game_StartNewGame(ctx); Game_ChangeState(ctx, STATE_PLAYING);   break;
            case 1: Game_ChangeState(ctx, STATE_MAIN_MENU); break;
            case 2: ctx->running = 0; break;
        }
    }

    /* Mouse support */
    int screenW = GetScreenWidth(), screenH = GetScreenHeight();
    int ty = (int)((float)screenH * 0.22f);
    int statY = ty + 64 + 30;
    int menuY = statY + 110;
    for (int i = 0; i < kGoCount; i++) {
        int tw = MeasureText(kGoItems[i], 28);
        Rectangle r = { (float)(screenW / 2 - tw / 2 - 20), (float)(menuY + i * 52 - 4),
                        (float)(tw + 40), 40.0f };
        if (CheckCollisionPointRec(GetMousePosition(), r)) {
            ms->selectedItem = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
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
    int panW = 560, panH = 380;
    int panX = screenW / 2 - panW / 2;
    int panY = (int)((float)screenH * 0.18f);
    DrawRectangleRounded((Rectangle){(float)panX, (float)panY, (float)panW, (float)panH},
                         0.08f, 8, (Color){8, 8, 28, 200});
    DrawRectangleRoundedLines((Rectangle){(float)panX, (float)panY, (float)panW, (float)panH},
                               0.08f, 8, (Color){0, 100, 200, 140});

    /* Column layout: KEY on left, ACTION on right */
    int col1 = panX + 30;          /* key label */
    int col2 = panX + 210;         /* action label */
    int rowH  = 36;
    int startY = panY + 24;
    int fs    = 18;

    Color keyCol    = (Color){0, 220, 255, 255};
    Color actCol    = (Color){200, 200, 220, 230};
    Color headCol   = (Color){255, 200, 0, 255};
    Color divCol    = (Color){0, 80, 160, 120};

    /* ── Movement section ── */
    DrawText("MOVEMENT", col1, startY, 14, headCol);
    DrawLine(col1, startY + 18, panX + panW - 30, startY + 18, divCol);
    startY += 26;

    struct { const char *key; const char *action; } rows[] = {
        { "W  /  UP ARROW",    "Thrust forward"        },
        { "S  /  DOWN ARROW",  "Brake / reverse"       },
        { "A  /  LEFT ARROW",  "Rotate left"           },
        { "D  /  RIGHT ARROW", "Rotate right"          },
    };
    for (int i = 0; i < 4; i++) {
        DrawText(rows[i].key,    col1, startY + i * rowH, fs, keyCol);
        DrawText(rows[i].action, col2, startY + i * rowH, fs, actCol);
    }
    startY += 4 * rowH + 10;

    /* ── Combat section ── */
    DrawText("COMBAT", col1, startY, 14, headCol);
    DrawLine(col1, startY + 18, panX + panW - 30, startY + 18, divCol);
    startY += 26;

    struct { const char *key; const char *action; } combat[] = {
        { "SPACE",  "Fire weapon"  },
        { "ESC",    "Pause game"   },
        { "F1",     "Toggle controls hint" },
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
