/**
 * main.c - SPACE JAM entry point
 *
 * Responsibilities:
 *   - Initialize the window
 *   - Run the main loop
 *   - Delegate all game logic to game.c
 */
#include "raylib.h"
#include "game.h"
#include "config.h"
#include "settings.h"
#include <math.h>

int main(void) {
    /* Always ensure working directory matches the executable location for assets & save data */
    ChangeDirectory(GetApplicationDirectory());

    /* Load settings before window creation so we can set resolution */
    GameSettings preSettings;
    Settings_LoadDefaults(&preSettings);
    Settings_Load(&preSettings);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(preSettings.screenWidth, preSettings.screenHeight, GAME_TITLE);
    Font_Init();

    if (preSettings.fullscreen) ToggleFullscreen();

    SetExitKey(KEY_NULL); /* We handle ESC ourselves */
    SetTargetFPS(TARGET_FPS);

    /* Seed the random number generator */
    SetRandomSeed((unsigned int)GetTime());

    GameContext ctx;
    Game_Init(&ctx);

    /* -- Main Loop ------------------------------------ */
    while (!WindowShouldClose() && ctx.running) {
        float dt = GetFrameTime();

        /* Clamp to prevent physics explosion on window stalls */
        if (dt > MAX_FRAME_TIME) dt = MAX_FRAME_TIME;

        Game_Update(&ctx, dt);
        Game_Draw(&ctx);
    }

    Game_Shutdown(&ctx);
    Font_Shutdown();
    CloseWindow();
    return 0;
}

