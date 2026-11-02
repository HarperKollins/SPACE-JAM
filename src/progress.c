/**
 * progress.c - Persistent profile tracking with medals and military career rank
 */
#include "progress.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

static const char *kSaveFileName = "spacejam_save.dat";

void Progress_Init(PlayerProgress *prog) {
    memset(prog, 0, sizeof(*prog));
    prog->newlyUnlocked = -1;
    prog->awardPopupTimer = 0.0f;
    Progress_Load(prog);
}

void Progress_Load(PlayerProgress *prog) {
    /* Multi-path check */
    char pathBuf[256];
    const char *path = kSaveFileName;
    if (!FileExists(path)) {
        const char *appDir = GetApplicationDirectory();
        if (appDir && appDir[0] != '\0') {
            snprintf(pathBuf, sizeof(pathBuf), "%s%s", appDir, kSaveFileName);
            if (FileExists(pathBuf)) path = pathBuf;
        }
    }

    FILE *f = fopen(path, "rb");
    if (!f) return;

    uint32_t magic = 0;
    if (fread(&magic, sizeof(magic), 1, f) == 1 && magic == 0x53504A4D) { /* "SPJM" */
        if (fread(&prog->totalKills, sizeof(int), 1, f) != 1) return;
        if (fread(&prog->totalShuttlesSaved, sizeof(int), 1, f) != 1) return;
        if (fread(&prog->totalBossesDefeated, sizeof(int), 1, f) != 1) return;
        if (fread(&prog->maxSectorReached, sizeof(int), 1, f) != 1) return;
        if (fread(&prog->maxWaveReached, sizeof(int), 1, f) != 1) return;
        if (fread(&prog->highestScore, sizeof(int), 1, f) != 1) return;
        if (fread(&prog->medalsBitmask, sizeof(uint32_t), 1, f) != 1) return;
    }
    fclose(f);
}

void Progress_Save(const PlayerProgress *prog) {
    FILE *f = fopen(kSaveFileName, "wb");
    if (!f) {
        const char *appDir = GetApplicationDirectory();
        if (appDir && appDir[0] != '\0') {
            char pathBuf[256];
            snprintf(pathBuf, sizeof(pathBuf), "%s%s", appDir, kSaveFileName);
            f = fopen(pathBuf, "wb");
        }
    }
    if (!f) return;

    uint32_t magic = 0x53504A4D; /* "SPJM" */
    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&prog->totalKills, sizeof(int), 1, f);
    fwrite(&prog->totalShuttlesSaved, sizeof(int), 1, f);
    fwrite(&prog->totalBossesDefeated, sizeof(int), 1, f);
    fwrite(&prog->maxSectorReached, sizeof(int), 1, f);
    fwrite(&prog->maxWaveReached, sizeof(int), 1, f);
    fwrite(&prog->highestScore, sizeof(int), 1, f);
    fwrite(&prog->medalsBitmask, sizeof(uint32_t), 1, f);
    fclose(f);
}

int Progress_HasMedal(const PlayerProgress *prog, MedalId medal) {
    if ((int)medal < 0 || (int)medal >= MEDAL_COUNT) return 0;
    return (prog->medalsBitmask & (1u << (uint32_t)medal)) != 0;
}

int Progress_UnlockMedal(PlayerProgress *prog, MedalId medal) {
    if (Progress_HasMedal(prog, medal)) return 0;
    prog->medalsBitmask |= (1u << (uint32_t)medal);
    prog->newlyUnlocked = (int)medal;
    prog->awardPopupTimer = 4.5f;

    /* Check if all prior medals unlocked, award Vanguard Legend */
    if (medal != MEDAL_VANGUARD_LEGEND) {
        int allUnlocked = 1;
        for (int i = 0; i < MEDAL_VANGUARD_LEGEND; i++) {
            if (!Progress_HasMedal(prog, (MedalId)i)) {
                allUnlocked = 0;
                break;
            }
        }
        if (allUnlocked) {
            prog->medalsBitmask |= (1u << (uint32_t)MEDAL_VANGUARD_LEGEND);
        }
    }

    Progress_Save(prog);
    return 1;
}

const char *Progress_GetRankName(const PlayerProgress *prog) {
    int score = prog->highestScore;
    int kills = prog->totalKills;
    int bosses = prog->totalBossesDefeated;

    if (bosses >= 4 || score >= 20000) return "VANGUARD LEGEND";
    if (bosses >= 3 || score >= 12000 || kills >= 100) return "FLEET COMMANDER";
    if (bosses >= 2 || score >= 6000  || kills >= 50)  return "ACE PILOT";
    if (bosses >= 1 || score >= 2500  || kills >= 20)  return "FLIGHT OFFICER";
    return "CADET";
}

const char *Progress_GetMedalTitle(MedalId medal) {
    switch (medal) {
        case MEDAL_SAVIOR_OF_TERRA: return "SAVIOR OF TERRA NOVUS";
        case MEDAL_SOLAR_ACE:       return "SOLAR CORONA ACE";
        case MEDAL_ICE_MARKSMAN:    return "ICE RING MARKSMAN";
        case MEDAL_CITADEL_SLAYER:  return "CITADEL DREADNOUGHT SLAYER";
        case MEDAL_HYPER_NITRO:     return "HYPER-NITRO MASTER";
        case MEDAL_IRON_WILL:       return "IRON WILL SURVIVOR";
        case MEDAL_DEADEYE:         return "DEADEYE SHARPSHOOTER";
        case MEDAL_VANGUARD_LEGEND: return "VANGUARD GRAND LEGEND";
        default: return "UNKNOWN HONOR";
    }
}

const char *Progress_GetMedalDesc(MedalId medal) {
    switch (medal) {
        case MEDAL_SAVIOR_OF_TERRA: return "Escort refugee shuttles past the Sector 1 blockade";
        case MEDAL_SOLAR_ACE:       return "Defeat Inquisitor Malakor in the blazing Solar Corona";
        case MEDAL_ICE_MARKSMAN:    return "Shatter the Synod Carrier in the Ice Rings of Xylos";
        case MEDAL_CITADEL_SLAYER:  return "Obliterate the Synod Citadel Flagship 'Apocalypse'";
        case MEDAL_HYPER_NITRO:     return "Maintain sustained afterburner boost in high-intensity combat";
        case MEDAL_IRON_WILL:       return "Triumph over an Imperial Boss with under 25% hull integrity";
        case MEDAL_DEADEYE:         return "Achieve over 10,000 combat points in a single tour of duty";
        case MEDAL_VANGUARD_LEGEND: return "Unlock all combat campaign decorations";
        default: return "";
    }
}

const char *Progress_GetMedalIcon(MedalId medal) {
    switch (medal) {
        case MEDAL_SAVIOR_OF_TERRA: return "[TERRA]";
        case MEDAL_SOLAR_ACE:       return "[SOLAR]";
        case MEDAL_ICE_MARKSMAN:    return "[RINGS]";
        case MEDAL_CITADEL_SLAYER:  return "[TITAN]";
        case MEDAL_HYPER_NITRO:     return "[NITRO]";
        case MEDAL_IRON_WILL:       return "[STEEL]";
        case MEDAL_DEADEYE:         return "[ELITE]";
        case MEDAL_VANGUARD_LEGEND: return "[VALOR]";
        default: return "[MEDAL]";
    }
}

