/**
 * menu.h - Menu rendering and interaction
 */
#ifndef SPACEJAM_MENU_H
#define SPACEJAM_MENU_H

#include "game.h"
#include "progress.h"
#include "raylib.h"

typedef enum {
    COMMS_SEQ_PROLOGUE = 0,
    COMMS_SEQ_VAREK_INCURSION,
    COMMS_SEQ_MALAKOR_INCURSION,
    COMMS_SEQ_ZEPHYR_INCURSION,
    COMMS_SEQ_ARCHON_CONFRONTATION,
    COMMS_SEQ_COUNT
} CommsSequenceId;

typedef struct {
    CommsSequenceId sequence;
    int   currentLine;
    int   totalLines;
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
    /* Portrait and ship textures (loaded once, freed on shutdown) */
    Texture2D  texHarper;
    Texture2D  texVance;
    Texture2D  texArchon;
    Texture2D  texNadia;
    Texture2D  texAmara;
    Texture2D  texMira;
    Texture2D  texVanguard;
    Texture2D  texOdyssey;
    int        portraitsLoaded;
    int        selectedEnding;
    int        endingChosen;   /* -1 if picking, 0=A, 1=B, 2=C */
    float      endingTimer;
} MenuState;

void Menu_Init(MenuState *ms);
void Menu_Shutdown(MenuState *ms);
void Menu_UpdateMain(MenuState *ms, GameContext *ctx, float dt);
void Menu_DrawMain(const MenuState *ms, const GameContext *ctx, int screenW, int screenH);

void Menu_StartPrologue(MenuState *ms);
void Menu_UpdatePrologue(MenuState *ms, GameContext *ctx, float dt);
void Menu_DrawPrologue(const MenuState *ms, int screenW, int screenH);

void Menu_StartComms(MenuState *ms);
void Menu_StartCommsSeq(MenuState *ms, CommsSequenceId seq);
void Menu_UpdateComms(MenuState *ms, GameContext *ctx, float dt);
void Menu_DrawComms(const MenuState *ms, int screenW, int screenH);

void Menu_StartGenesisChoice(MenuState *ms);
void Menu_UpdateGenesisChoice(MenuState *ms, GameContext *ctx, PlayerProgress *prog, float dt);
void Menu_DrawGenesisChoice(const MenuState *ms, const PlayerProgress *prog, int screenW, int screenH);

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

