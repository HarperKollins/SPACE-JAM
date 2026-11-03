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

typedef enum {
    VECTOR_GENESIS = 0,   /* Architect Consciousness: Piercing & weapon overcharge */
    VECTOR_GRAVITY,       /* Gravitational Singularity (Key Q) */
    VECTOR_TEMPORAL,      /* Chrono-Stasis & Hull Rewind (Key E) */
    VECTOR_QUANTUM,       /* Micro-Warp Blink (Key SHIFT) */
    VECTOR_VOID,          /* Phasing Dematerialization (Key F) */
    VECTOR_NEXUS,         /* Chain Lightning Resonance */
    VECTOR_ASCENDANT,     /* Transcendent Key / Unlocks Secret Ending C */
    VECTOR_COUNT
} VectorId;

typedef enum {
    UPGRADE_HULL = 0,      /* Max HP increase & hull armor reduction */
    UPGRADE_SHIELD,        /* Deflector energy shield capacity & recharge rate */
    UPGRADE_AFTERBURNER,   /* Hyper-Nitro boost thrust, top speed & heat capacity */
    UPGRADE_OVERCHARGE,    /* Primary plasma damage & muzzle velocity */
    UPGRADE_TORPEDOES,     /* Secondary Homing Photon Torpedo ordnance */
    UPGRADE_COUNT
} UpgradeType;

#define MAX_UPGRADE_LEVEL 5

typedef struct {
    int      totalKills;
    int      totalShuttlesSaved;
    int      totalBossesDefeated;
    int      maxSectorReached;
    int      maxWaveReached;
    int      highestScore;
    int      credits;               /* Salvage credits for Hangar upgrades */
    int      upgradeLevels[UPGRADE_COUNT]; /* 0 to MAX_UPGRADE_LEVEL */
    uint32_t medalsBitmask; /* 1 bit per MedalId */
    uint32_t relicsBitmask; /* 1 bit per VectorId */
    uint8_t  endingsSeen;   /* bit 0 = Ending A, bit 1 = Ending B, bit 2 = Ending C */
    int      newlyUnlocked; /* MedalId that was just awarded, or -1 */
    float    awardPopupTimer;
} PlayerProgress;

void        Progress_Init(PlayerProgress *prog);
void        Progress_Load(PlayerProgress *prog);
void        Progress_Save(const PlayerProgress *prog);
void        Progress_AddCredits(PlayerProgress *prog, int amount);
int         Progress_GetUpgradeCost(const PlayerProgress *prog, UpgradeType type);
int         Progress_CanBuyUpgrade(const PlayerProgress *prog, UpgradeType type);
int         Progress_BuyUpgrade(PlayerProgress *prog, UpgradeType type);
const char *Progress_GetUpgradeName(UpgradeType type);
const char *Progress_GetUpgradeDesc(UpgradeType type);
const char *Progress_GetUpgradeStatLabel(const PlayerProgress *prog, UpgradeType type);

int         Progress_HasMedal(const PlayerProgress *prog, MedalId medal);
int         Progress_UnlockMedal(PlayerProgress *prog, MedalId medal);
int         Progress_HasRelic(const PlayerProgress *prog, VectorId relic);
int         Progress_UnlockRelic(PlayerProgress *prog, VectorId relic);
int         Progress_HasEnding(const PlayerProgress *prog, int endingIdx);
int         Progress_UnlockEnding(PlayerProgress *prog, int endingIdx);
const char *Progress_GetRankName(const PlayerProgress *prog);
const char *Progress_GetMedalTitle(MedalId medal);
const char *Progress_GetMedalDesc(MedalId medal);
const char *Progress_GetMedalIcon(MedalId medal);
const char *Progress_GetRelicName(VectorId relic);
const char *Progress_GetRelicDesc(VectorId relic);
const char *Progress_GetRelicKeyHint(VectorId relic);

#endif /* SPACEJAM_PROGRESS_H */
