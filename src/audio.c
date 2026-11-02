/**
 * audio.c - Audio management (graceful degradation if assets missing)
 */
#include "audio.h"
#include "config.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>

/* Internal sound handle storage */
static Sound  g_sfx[SFX_COUNT];
static int    g_sfxLoaded[SFX_COUNT];
static Music  g_music;
static int    g_musicLoaded = 0;

/* Asset paths relative to executable */
static const char *kSfxPaths[SFX_COUNT] = {
    "assets/audio/shoot.wav",
    "assets/audio/explosion_small.wav",
    "assets/audio/explosion_large.wav",
    "assets/audio/player_hit.wav",
    "assets/audio/powerup.wav",
    "assets/audio/boss_appear.wav",
    "assets/audio/boss_explode.wav",
    "assets/audio/menu_select.wav",
    "assets/audio/menu_confirm.wav",
    "assets/audio/game_over.wav",
};

void Audio_Init(AudioManager *am, float masterVol, float musicVol, float sfxVol) {
    memset(am, 0, sizeof(*am));
    am->masterVol = masterVol;
    am->musicVol  = musicVol;
    am->sfxVol    = sfxVol;

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        fprintf(stderr, "[audio] Audio device failed to initialize — continuing without audio.\n");
        am->initialized = 0;
        return;
    }
    am->initialized = 1;

    /* Load sound effects — missing files are silently skipped */
    for (int i = 0; i < SFX_COUNT; i++) {
        g_sfxLoaded[i] = 0;
        if (FileExists(kSfxPaths[i])) {
            g_sfx[i] = LoadSound(kSfxPaths[i]);
            if (IsSoundValid(g_sfx[i])) {
                SetSoundVolume(g_sfx[i], masterVol * sfxVol);
                g_sfxLoaded[i] = 1;
            }
        }
    }

    /* Background music */
    const char *musicPath = "assets/audio/music.ogg";
    g_musicLoaded = 0;
    if (FileExists(musicPath)) {
        g_music = LoadMusicStream(musicPath);
        if (IsMusicValid(g_music)) {
            SetMusicVolume(g_music, masterVol * musicVol);
            PlayMusicStream(g_music);
            g_musicLoaded = 1;
        }
    }
}

void Audio_Shutdown(AudioManager *am) {
    if (!am->initialized) return;
    if (g_musicLoaded) {
        StopMusicStream(g_music);
        UnloadMusicStream(g_music);
        g_musicLoaded = 0;
    }
    for (int i = 0; i < SFX_COUNT; i++) {
        if (g_sfxLoaded[i]) {
            UnloadSound(g_sfx[i]);
            g_sfxLoaded[i] = 0;
        }
    }
    CloseAudioDevice();
    am->initialized = 0;
}

void Audio_PlaySFX(AudioManager *am, SfxId id) {
    if (!am->initialized) return;
    if (id < 0 || id >= SFX_COUNT) return;
    if (!g_sfxLoaded[id]) return;
    PlaySound(g_sfx[id]);
}

void Audio_SetVolumes(AudioManager *am, float master, float music, float sfx) {
    am->masterVol = master;
    am->musicVol  = music;
    am->sfxVol    = sfx;
    if (!am->initialized) return;
    for (int i = 0; i < SFX_COUNT; i++) {
        if (g_sfxLoaded[i]) SetSoundVolume(g_sfx[i], master * sfx);
    }
    if (g_musicLoaded) SetMusicVolume(g_music, master * music);
}

void Audio_UpdateMusic(AudioManager *am, float dt) {
    (void)dt;
    if (!am->initialized || !g_musicLoaded) return;
    UpdateMusicStream(g_music);
}

