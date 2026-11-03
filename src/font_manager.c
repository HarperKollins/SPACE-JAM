/**
 * font_manager.c - High-definition TrueType font management implementation
 */
#include "font_manager.h"
#include <stdio.h>
#include <stdlib.h>

#undef DrawText
#undef MeasureText

Font g_gameFont = { 0 };
Font g_gameFontBold = { 0 };
int  g_gameFontLoaded = 0;

static const char *FindFontPath(const char *relPath) {
    static char buf[512];
    const char *appDir = GetApplicationDirectory();
    if (appDir && appDir[0] != '\0') {
        snprintf(buf, sizeof(buf), "%s%s", appDir, relPath);
        if (FileExists(buf)) return buf;
        snprintf(buf, sizeof(buf), "%s../%s", appDir, relPath);
        if (FileExists(buf)) return buf;
    }
    snprintf(buf, sizeof(buf), "SPACE_JAM/%s", relPath);
    if (FileExists(buf)) return buf;
    snprintf(buf, sizeof(buf), "../%s", relPath);
    if (FileExists(buf)) return buf;
    if (FileExists(relPath)) return relPath;
    if (FileExists("C:/Windows/Fonts/segoeui.ttf")) return "C:/Windows/Fonts/segoeui.ttf";
    if (FileExists("C:/Windows/Fonts/arial.ttf")) return "C:/Windows/Fonts/arial.ttf";
    return NULL;
}

void Font_Init(void) {
    /* Extended codepoints: Latin-1, punctuation, brackets, UI icons */
    int codepoints[320];
    int count = 0;
    for (int i = 32; i <= 126; i++) codepoints[count++] = i;
    for (int i = 160; i <= 255; i++) codepoints[count++] = i;
    codepoints[count++] = 0x2014; /* em dash — */
    codepoints[count++] = 0x2013; /* en dash – */
    codepoints[count++] = 0x2018; /* ‘ */
    codepoints[count++] = 0x2019; /* ’ */
    codepoints[count++] = 0x201C; /* “ */
    codepoints[count++] = 0x201D; /* ” */
    codepoints[count++] = 0x25A0; /* black square ■ */
    codepoints[count++] = 0x25A1; /* white square □ */
    codepoints[count++] = 0x25B6; /* right triangle ▶ */
    codepoints[count++] = 0x25C0; /* left triangle ◀ */

    const char *path = FindFontPath("assets/fonts/font.ttf");
    if (path) {
        g_gameFont = LoadFontEx(path, 64, codepoints, count);
        if (g_gameFont.texture.id > 0) {
            SetTextureFilter(g_gameFont.texture, TEXTURE_FILTER_BILINEAR);
            g_gameFontLoaded = 1;
            TraceLog(LOG_INFO, "FONT: Loaded ultra-smooth font from %s", path);
        }
    }

    const char *boldPath = FindFontPath("assets/fonts/font_bold.ttf");
    if (boldPath) {
        g_gameFontBold = LoadFontEx(boldPath, 64, codepoints, count);
        if (g_gameFontBold.texture.id > 0) {
            SetTextureFilter(g_gameFontBold.texture, TEXTURE_FILTER_BILINEAR);
        }
    } else if (g_gameFontLoaded) {
        g_gameFontBold = g_gameFont;
    }
}

void Font_Shutdown(void) {
    if (g_gameFontLoaded && g_gameFont.texture.id > 0) {
        UnloadFont(g_gameFont);
        g_gameFont.texture.id = 0;
    }
    if (g_gameFontBold.texture.id > 0 && g_gameFontBold.texture.id != g_gameFont.texture.id) {
        UnloadFont(g_gameFontBold);
        g_gameFontBold.texture.id = 0;
    }
    g_gameFontLoaded = 0;
}

void DrawGameText(const char *text, int posX, int posY, int fontSize, Color color) {
    if (g_gameFontLoaded && g_gameFont.texture.id > 0 && text) {
        Vector2 pos = { (float)posX, (float)posY };
        DrawTextEx(g_gameFont, text, pos, (float)fontSize, 1.0f, color);
    } else if (text) {
        DrawText(text, posX, posY, fontSize, color);
    }
}

void DrawGameTextBold(const char *text, int posX, int posY, int fontSize, Color color) {
    if (g_gameFontBold.texture.id > 0 && text) {
        Vector2 pos = { (float)posX, (float)posY };
        DrawTextEx(g_gameFontBold, text, pos, (float)fontSize, 1.0f, color);
    } else {
        DrawGameText(text, posX, posY, fontSize, color);
    }
}

int MeasureGameText(const char *text, int fontSize) {
    if (g_gameFontLoaded && g_gameFont.texture.id > 0 && text) {
        Vector2 dim = MeasureTextEx(g_gameFont, text, (float)fontSize, 1.0f);
        return (int)dim.x;
    } else if (text) {
        return MeasureText(text, fontSize);
    }
    return 0;
}

int MeasureGameTextBold(const char *text, int fontSize) {
    if (g_gameFontBold.texture.id > 0 && text) {
        Vector2 dim = MeasureTextEx(g_gameFontBold, text, (float)fontSize, 1.0f);
        return (int)dim.x;
    }
    return MeasureGameText(text, fontSize);
}
