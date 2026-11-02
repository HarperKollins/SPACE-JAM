/**
 * progress.h - Persistent player progression, stats, medals, and military ranks
 */
#ifndef SPACEJAM_PROGRESS_H
#define SPACEJAM_PROGRESS_H

#include <stdint.h>

typedef enum {
    MEDAL_SAVIOR_OF_TERRA = 0, /* Escort evacuation fleet in Sector 1 */
    MEDAL_SOLAR_ACE,           /* Defeat Inquisitor Malakor in Sector 2 */
    MEDAL_ICE_MARKSMAN,        /* Conquer the Ice Rings of Xylos */
    MEDAL_CITADEL_SLAYER,      /* Crush the Synod Citadel in Sector 4 */
    MEDAL_HYPER_NITRO,         /* Flight Mastery: continuous afterburner */
    MEDAL_IRON_WILL,           /* Survive a boss fight at critical hull */
    MEDAL_DEADEYE,             /* Score over 10,000 combat points */
    MEDAL_VANGUARD_LEGEND,     /* Unlock all other campaign medals */
    MEDAL_COUNT
} MedalId;

typedef struct {
    int      totalKills;
    int      totalShuttlesSaved;
    int      totalBossesDefeated;
    int      maxSectorReached;
    int      maxWaveReached;
    int      highestScore;
    uint32_t medalsBitmask; /* 1 bit per MedalId */
    int      newlyUnlocked; /* MedalId that was just awarded, or -1 */
    float    awardPopupTimer;
} PlayerProgress;

void        Progress_Init(PlayerProgress *prog);
void        Progress_Load(PlayerProgress *prog);
void        Progress_Save(const PlayerProgress *prog);
int         Progress_HasMedal(const PlayerProgress *prog, MedalId medal);
int         Progress_UnlockMedal(PlayerProgress *prog, MedalId medal);
const char *Progress_GetRankName(const PlayerProgress *prog);
const char *Progress_GetMedalTitle(MedalId medal);
const char *Progress_GetMedalDesc(MedalId medal);
const char *Progress_GetMedalIcon(MedalId medal);

#endif /* SPACEJAM_PROGRESS_H */
