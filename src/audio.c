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

static Music  g_music[MUSIC_TRACK_COUNT];
static int    g_musicLoaded[MUSIC_TRACK_COUNT];

/* Asset paths relative to executable */
static const char *kSfxPaths[SFX_COUNT] = {
    "assets/audio/shoot.wav",
    "assets/audio/enemy_shoot.wav",
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

static const char *kMusicPaths[MUSIC_TRACK_COUNT] = {
    "",
    "assets/audio/music_intro.wav",
    "assets/audio/music.wav",
};

static const char *ResolveAssetPath(const char *path, char *buf, size_t bufSize) {
    if (FileExists(path)) return path;
    snprintf(buf, bufSize, "../%s", path);
    if (FileExists(buf)) return buf;
    return path;
}

void Audio_Init(AudioManager *am, float masterVol, float musicVol, float sfxVol) {
    memset(am, 0, sizeof(*am));
    am->masterVol = masterVol;
    am->musicVol  = musicVol;
    am->sfxVol    = sfxVol;
    am->currentTrack = MUSIC_TRACK_NONE;

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        fprintf(stderr, "[audio] Audio device failed to initialize — continuing without audio.\n");
        am->initialized = 0;
        return;
    }
    am->initialized = 1;

    char resolved[256];

    /* Load sound effects */
    for (int i = 0; i < SFX_COUNT; i++) {
        g_sfxLoaded[i] = 0;
        const char *p = ResolveAssetPath(kSfxPaths[i], resolved, sizeof(resolved));
        if (FileExists(p)) {
            g_sfx[i] = LoadSound(p);
            if (IsSoundValid(g_sfx[i])) {
                SetSoundVolume(g_sfx[i], masterVol * sfxVol);
                g_sfxLoaded[i] = 1;
            }
        }
    }

    /* Load music tracks */
    for (int m = 1; m < MUSIC_TRACK_COUNT; m++) {
        g_musicLoaded[m] = 0;
        const char *p = ResolveAssetPath(kMusicPaths[m], resolved, sizeof(resolved));
        if (FileExists(p)) {
            g_music[m] = LoadMusicStream(p);
            if (IsMusicValid(g_music[m])) {
                g_music[m].looping = 1;
                SetMusicVolume(g_music[m], masterVol * musicVol);
                g_musicLoaded[m] = 1;
            }
        }
    }
}

void Audio_Shutdown(AudioManager *am) {
    if (!am->initialized) return;

    for (int m = 1; m < MUSIC_TRACK_COUNT; m++) {
        if (g_musicLoaded[m]) {
            StopMusicStream(g_music[m]);
            UnloadMusicStream(g_music[m]);
            g_musicLoaded[m] = 0;
        }
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
    for (int m = 1; m < MUSIC_TRACK_COUNT; m++) {
        if (g_musicLoaded[m]) SetMusicVolume(g_music[m], master * music);
    }
}

void Audio_PlayMusicTrack(AudioManager *am, MusicTrackId track) {
    if (!am->initialized) return;
    if (track == am->currentTrack) return;

    /* Stop currently playing track */
    if (am->currentTrack > MUSIC_TRACK_NONE && am->currentTrack < MUSIC_TRACK_COUNT) {
        if (g_musicLoaded[am->currentTrack]) {
            StopMusicStream(g_music[am->currentTrack]);
        }
    }

    am->currentTrack = track;
    if (track > MUSIC_TRACK_NONE && track < MUSIC_TRACK_COUNT) {
        if (g_musicLoaded[track]) {
            PlayMusicStream(g_music[track]);
        }
    }
}

void Audio_StopMusic(AudioManager *am) {
    if (!am->initialized) return;
    if (am->currentTrack > MUSIC_TRACK_NONE && am->currentTrack < MUSIC_TRACK_COUNT) {
        if (g_musicLoaded[am->currentTrack]) {
            StopMusicStream(g_music[am->currentTrack]);
        }
    }
    am->currentTrack = MUSIC_TRACK_NONE;
}

void Audio_UpdateMusic(AudioManager *am, float dt) {
    (void)dt;
    if (!am->initialized) return;
    if (am->currentTrack > MUSIC_TRACK_NONE && am->currentTrack < MUSIC_TRACK_COUNT) {
        if (g_musicLoaded[am->currentTrack]) {
            UpdateMusicStream(g_music[am->currentTrack]);
        }
    }
}
