/**
 * menu.h - Menu rendering and interaction
 */
#ifndef SPACEJAM_MENU_H
#define SPACEJAM_MENU_H

#include "game.h"
#include "progress.h"

typedef struct {
    int   currentLine;
    float charTimer;
    int   charsRevealed;
    float beepTimer;
    float freqAnim;
} CommsState;

typedef struct {
    int        selectedItem;
    float      itemAnim[8]; /* per-item highlight pulse timers */
    float      titleAnim;
    float      bgTimer;
    float      prologueScroll;
    CommsState comms;
    int        selectedMedal;
} MenuState;

void Menu_Init(MenuState *ms);
void Menu_UpdateMain(MenuState *ms, GameContext *ctx, float dt);
void Menu_DrawMain(const MenuState *ms, const GameContext *ctx, int screenW, int screenH);

void Menu_StartPrologue(MenuState *ms);
void Menu_UpdatePrologue(MenuState *ms, GameContext *ctx, float dt);
void Menu_DrawPrologue(const MenuState *ms, int screenW, int screenH);

void Menu_StartComms(MenuState *ms);
void Menu_UpdateComms(MenuState *ms, GameContext *ctx, float dt);
void Menu_DrawComms(const MenuState *ms, int screenW, int screenH);

void Menu_UpdateMedals(MenuState *ms, GameContext *ctx, float dt);
void Menu_DrawMedals(const MenuState *ms, const PlayerProgress *prog, int screenW, int screenH);

void Menu_UpdatePause(MenuState *ms, GameContext *ctx, float dt);
void Menu_DrawPause(const MenuState *ms, int screenW, int screenH);

void Menu_UpdateGameOver(MenuState *ms, GameContext *ctx, float dt);
void Menu_DrawGameOver(const MenuState *ms, const GameContext *ctx, int screenW, int screenH);

void Menu_UpdateSettings(MenuState *ms, GameContext *ctx, float dt);
void Menu_DrawSettings(const MenuState *ms, const GameContext *ctx, int screenW, int screenH);

void Menu_UpdateControls(MenuState *ms, GameContext *ctx, float dt);
void Menu_DrawControls(const MenuState *ms, int screenW, int screenH);

#endif /* SPACEJAM_MENU_H */

