/**
 * font_manager.h - High-definition TrueType font management
 *
 * Loads smooth anti-aliased Segoe UI / TrueType fonts with bilinear filtering
 * to replace the default pixelated 10x10 bitmap font.
 */
#ifndef SPACEJAM_FONT_MANAGER_H
#define SPACEJAM_FONT_MANAGER_H

#include "raylib.h"

extern Font g_gameFont;
extern Font g_gameFontBold;
extern int  g_gameFontLoaded;

void Font_Init(void);
void Font_Shutdown(void);

void DrawGameText(const char *text, int posX, int posY, int fontSize, Color color);
void DrawGameTextBold(const char *text, int posX, int posY, int fontSize, Color color);
int  MeasureGameText(const char *text, int fontSize);
int  MeasureGameTextBold(const char *text, int fontSize);

/* Redirect default text drawing to the smooth anti-aliased font */
#undef DrawText
#undef MeasureText
#define DrawText(text, x, y, fontSize, ...) DrawGameText(text, x, y, fontSize, __VA_ARGS__)
#define MeasureText(text, fontSize) MeasureGameText(text, fontSize)

#endif /* SPACEJAM_FONT_MANAGER_H */
