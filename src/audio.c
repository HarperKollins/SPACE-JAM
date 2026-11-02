/**
 * audio.c - Audio management with multi-path resolution and procedural fallback
 */
#include "audio.h"
#include "config.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/* Internal sound handle storage */
static Sound  g_sfx[SFX_COUNT];
static int    g_sfxLoaded[SFX_COUNT];

static Music  g_music[MUSIC_TRACK_COUNT];
static int    g_musicLoaded[MUSIC_TRACK_COUNT];

/* Asset paths */
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
    "assets/audio/radio_chirp.wav",
    "assets/audio/radio_static.wav",
    "assets/audio/radio_beep.wav",
    "assets/audio/warp_jump.wav",
    "assets/audio/medal_award.wav",
};

static const char *kMusicPaths[MUSIC_TRACK_COUNT] = {
    "",
    "assets/audio/music_intro.wav",
    "assets/audio/music.wav",
};

/* Multi-path search: finds assets regardless of whether launched from
   SPACE_JAM, build/, workspace root, or Windows Explorer */
static const char *ResolveAssetPath(const char *path, char *buf, size_t bufSize) {
    if (FileExists(path)) return path;

    const char *appDir = GetApplicationDirectory();
    if (appDir && appDir[0] != '\0') {
        snprintf(buf, bufSize, "%s%s", appDir, path);
        if (FileExists(buf)) return buf;

        snprintf(buf, bufSize, "%s../%s", appDir, path);
        if (FileExists(buf)) return buf;
    }

    snprintf(buf, bufSize, "SPACE_JAM/%s", path);
    if (FileExists(buf)) return buf;

    snprintf(buf, bufSize, "SPACE_JAM/build/%s", path);
    if (FileExists(buf)) return buf;

    snprintf(buf, bufSize, "../%s", path);
    if (FileExists(buf)) return buf;

    return path;
}

/* ── Procedural In-Memory Fallback Synthesis ─────────── */
static Sound SynthesizeFallbackSFX(SfxId id) {
    unsigned int sampleRate = 44100;
    float dur = 0.15f;
    if (id == SFX_EXPLOSION_LARGE) dur = 0.65f;
    else if (id == SFX_BOSS_APPEAR || id == SFX_GAME_OVER) dur = 1.2f;
    else if (id == SFX_BOSS_EXPLODE) dur = 1.6f;
    else if (id == SFX_MENU_SELECT) dur = 0.05f;
    else if (id == SFX_RADIO_CHIRP) dur = 0.16f;
    else if (id == SFX_RADIO_STATIC) dur = 0.12f;
    else if (id == SFX_RADIO_BEEP) dur = 0.04f;
    else if (id == SFX_WARP_JUMP) dur = 1.10f;
    else if (id == SFX_MEDAL_AWARD) dur = 0.75f;

    unsigned int frames = (unsigned int)(sampleRate * dur);
    int16_t *buf = (int16_t *)malloc(frames * sizeof(int16_t));
    if (!buf) return (Sound){0};

    float phase = 0.0f;
    for (unsigned int i = 0; i < frames; i++) {
        float t = (float)i / (float)sampleRate;
        float p = t / dur;
        float sample = 0.0f;

        switch (id) {
            case SFX_SHOOT: {
                float freq = 220.0f + 1100.0f * expf(-p * 5.0f);
                phase += 2.0f * (float)M_PI * freq / sampleRate;
                sample = sinf(phase) * expf(-p * 4.0f) * 0.85f;
                break;
            }
            case SFX_ENEMY_SHOOT: {
                float freq = 130.0f + 500.0f * expf(-p * 4.5f);
                phase += 2.0f * (float)M_PI * freq / sampleRate;
                float saw = (fmodf(phase, 2.0f * (float)M_PI) / (float)M_PI) - 1.0f;
                sample = (saw * 0.6f + sinf(phase) * 0.4f) * expf(-p * 4.0f) * 0.8f;
                break;
            }
            case SFX_EXPLOSION_SMALL:
            case SFX_EXPLOSION_LARGE:
            case SFX_BOSS_EXPLODE: {
                float noise = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
                phase += 2.0f * (float)M_PI * (45.0f + 120.0f * expf(-p * 4.0f)) / sampleRate;
                sample = (noise * 0.65f + sinf(phase) * 0.45f) * expf(-p * 3.5f) * 0.85f;
                break;
            }
            case SFX_PLAYER_HIT: {
                phase += 2.0f * (float)M_PI * (50.0f + 180.0f * expf(-p * 6.0f)) / sampleRate;
                float noise = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
                sample = (sinf(phase) * 0.7f + noise * 0.3f) * expf(-p * 5.0f) * 0.8f;
                break;
            }
            case SFX_POWERUP: {
                float freqs[4] = { 523.25f, 659.25f, 783.99f, 1046.50f };
                int nIdx = (int)(p * 4.0f);
                if (nIdx > 3) nIdx = 3;
                phase += 2.0f * (float)M_PI * freqs[nIdx] / sampleRate;
                sample = sinf(phase) * expf(-fmodf(t, dur * 0.25f) * 12.0f) * 0.8f;
                break;
            }
            case SFX_MENU_SELECT: {
                phase += 2.0f * (float)M_PI * 750.0f / sampleRate;
                sample = sinf(phase) * expf(-p * 18.0f) * 0.6f;
                break;
            }
            case SFX_MENU_CONFIRM: {
                phase += 2.0f * (float)M_PI * (440.0f + 440.0f * p) / sampleRate;
                sample = sinf(phase) * expf(-p * 6.0f) * 0.75f;
                break;
            }
            case SFX_RADIO_CHIRP: {
                /* Dual radio frequency chirp: 1800 Hz then 2400 Hz */
                float freq = (p < 0.5f) ? 1800.0f : 2400.0f;
                phase += 2.0f * (float)M_PI * freq / sampleRate;
                float subT = (p < 0.5f) ? (p * 2.0f) : ((p - 0.5f) * 2.0f);
                sample = sinf(phase) * (1.0f - subT) * 0.85f;
                break;
            }
            case SFX_RADIO_STATIC: {
                /* Squelch noise burst with low-frequency rumble */
                float noise = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
                phase += 2.0f * (float)M_PI * 180.0f / sampleRate;
                sample = (noise * 0.75f + sinf(phase) * 0.25f) * (1.0f - p * 0.8f) * 0.70f;
                break;
            }
            case SFX_RADIO_BEEP: {
                /* Short typewriter character blip: 920 Hz */
                phase += 2.0f * (float)M_PI * 920.0f / sampleRate;
                sample = sinf(phase) * expf(-p * 14.0f) * 0.65f;
                break;
            }
            case SFX_WARP_JUMP: {
                /* Rising hyper-drive sweep from 140 Hz to 1600 Hz then deep sonic thunder */
                float freq = (p < 0.6f) ? (140.0f + 1460.0f * (p / 0.6f) * (p / 0.6f)) : 90.0f;
                phase += 2.0f * (float)M_PI * freq / sampleRate;
                float noise = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
                float env = (p < 0.6f) ? (p / 0.6f) : (1.0f - (p - 0.6f) / 0.4f);
                sample = (sinf(phase) * 0.65f + noise * 0.35f) * env * 0.90f;
                break;
            }
            case SFX_MEDAL_AWARD: {
                /* Heroic 4-note brass fanfare arpeggio (C5, E5, G5, C6) */
                float notes[4] = { 523.25f, 659.25f, 783.99f, 1046.50f };
                int n = (int)(p * 4.0f);
                if (n > 3) n = 3;
                phase += 2.0f * (float)M_PI * notes[n] / sampleRate;
                float subT = fmodf(p * 4.0f, 1.0f);
                sample = sinf(phase) * (1.0f - subT * 0.7f) * 0.85f;
                break;
            }
            default: {
                phase += 2.0f * (float)M_PI * (330.0f * (1.0f - p * 0.4f)) / sampleRate;
                sample = sinf(phase) * expf(-p * 3.0f) * 0.7f;
                break;
            }
        }

        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        buf[i] = (int16_t)(sample * 32767.0f);
    }

    Wave w = {
        .frameCount = frames,
        .sampleRate = sampleRate,
        .sampleSize = 16,
        .channels = 1,
        .data = buf
    };
    Sound s = LoadSoundFromWave(w);
    free(buf);
    return s;
}

void Audio_Init(AudioManager *am, float masterVol, float musicVol, float sfxVol) {
    memset(am, 0, sizeof(*am));
    am->masterVol = masterVol;
    am->musicVol  = musicVol;
    am->sfxVol    = sfxVol;
    am->currentTrack = MUSIC_TRACK_NONE;

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        fprintf(stderr, "[audio] Warning: Audio device failed to initialize.\n");
        am->initialized = 0;
        return;
    }
    am->initialized = 1;
    SetMasterVolume(masterVol);

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

        /* Fallback: if file loading failed or missing, synthesize in memory! */
        if (!g_sfxLoaded[i]) {
            g_sfx[i] = SynthesizeFallbackSFX((SfxId)i);
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

    SetMasterVolume(master);
    for (int i = 0; i < SFX_COUNT; i++) {
        if (g_sfxLoaded[i]) SetSoundVolume(g_sfx[i], master * sfx);
    }
    for (int m = 1; m < MUSIC_TRACK_COUNT; m++) {
        if (g_musicLoaded[m]) SetMusicVolume(g_music[m], master * music);
    }
}

void Audio_PlayMusicTrack(AudioManager *am, MusicTrackId track) {
    if (!am->initialized) return;

    /* If already requested track, ensure it is actively streaming */
    if (track == am->currentTrack) {
        if (track > MUSIC_TRACK_NONE && track < MUSIC_TRACK_COUNT && g_musicLoaded[track]) {
            if (!IsMusicStreamPlaying(g_music[track])) {
                PlayMusicStream(g_music[track]);
            }
        }
        return;
    }

    /* Stop previous track */
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
