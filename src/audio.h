/**
 * audio.h - Audio management wrapper
 *
 * All sounds are loaded at startup. Missing files produce a warning but
 * do NOT crash the game — every function checks for valid handles before use.
 */
#ifndef SPACEJAM_AUDIO_H
#define SPACEJAM_AUDIO_H

typedef enum {
    SFX_SHOOT = 0,
    SFX_ENEMY_SHOOT,
    SFX_EXPLOSION_SMALL,
    SFX_EXPLOSION_LARGE,
    SFX_PLAYER_HIT,
    SFX_POWERUP,
    SFX_BOSS_APPEAR,
    SFX_BOSS_EXPLODE,
    SFX_MENU_SELECT,
    SFX_MENU_CONFIRM,
    SFX_GAME_OVER,
    SFX_RADIO_CHIRP,
    SFX_RADIO_STATIC,
    SFX_RADIO_BEEP,
    SFX_WARP_JUMP,
    SFX_MEDAL_AWARD,
    SFX_COUNT
} SfxId;

typedef enum {
    MUSIC_TRACK_NONE = 0,
    MUSIC_TRACK_INTRO,
    MUSIC_TRACK_BATTLE,
    MUSIC_TRACK_COUNT
} MusicTrackId;

typedef struct {
    int          initialized;
    float        masterVol;
    float        musicVol;
    float        sfxVol;
    MusicTrackId currentTrack;
} AudioManager;

void  Audio_Init(AudioManager *am, float masterVol, float musicVol, float sfxVol);
void  Audio_Shutdown(AudioManager *am);
void  Audio_PlaySFX(AudioManager *am, SfxId id);
void  Audio_SetVolumes(AudioManager *am, float master, float music, float sfx);
void  Audio_UpdateMusic(AudioManager *am, float dt);
void  Audio_PlayMusicTrack(AudioManager *am, MusicTrackId track);
void  Audio_StopMusic(AudioManager *am);

#endif /* SPACEJAM_AUDIO_H */

