/**
 * settings.c - Settings persistence
 */
#include "settings.h"
#include "config.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

#define SETTINGS_FILE "spacejam_settings.cfg"
#define SETTINGS_MAGIC 0x534A5346u  /* "SJSF" */

typedef struct {
    unsigned int magic;
    float masterVolume;
    float musicVolume;
    float sfxVolume;
    int   fullscreen;
    int   screenShake;
    int   particleQuality;
    int   screenWidth;
    int   screenHeight;
} SettingsFile;

void Settings_LoadDefaults(GameSettings *s) {
    s->masterVolume   = DEFAULT_MASTER_VOL;
    s->musicVolume    = DEFAULT_MUSIC_VOL;
    s->sfxVolume      = DEFAULT_SFX_VOL;
    s->fullscreen     = 0;
    s->screenShake    = 1;
    s->particleQuality = 2;
    s->screenWidth    = SCREEN_WIDTH;
    s->screenHeight   = SCREEN_HEIGHT;
}

void Settings_Load(GameSettings *s) {
    Settings_LoadDefaults(s);

    FILE *f = fopen(SETTINGS_FILE, "rb");
    if (!f) return;

    SettingsFile sf;
    size_t r = fread(&sf, 1, sizeof(sf), f);
    fclose(f);

    if (r != sizeof(sf) || sf.magic != SETTINGS_MAGIC) return;

    s->masterVolume   = sf.masterVolume;
    s->musicVolume    = sf.musicVolume;
    s->sfxVolume      = sf.sfxVolume;
    s->fullscreen     = sf.fullscreen;
    s->screenShake    = sf.screenShake;
    s->particleQuality = sf.particleQuality;
    s->screenWidth    = sf.screenWidth;
    s->screenHeight   = sf.screenHeight;
}

void Settings_Save(const GameSettings *s) {
    SettingsFile sf;
    sf.magic          = SETTINGS_MAGIC;
    sf.masterVolume   = s->masterVolume;
    sf.musicVolume    = s->musicVolume;
    sf.sfxVolume      = s->sfxVolume;
    sf.fullscreen     = s->fullscreen;
    sf.screenShake    = s->screenShake;
    sf.particleQuality = s->particleQuality;
    sf.screenWidth    = s->screenWidth;
    sf.screenHeight   = s->screenHeight;

    FILE *f = fopen(SETTINGS_FILE, "wb");
    if (!f) return;
    fwrite(&sf, 1, sizeof(sf), f);
    fclose(f);
}

