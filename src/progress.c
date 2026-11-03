/**
 * progress.c - Persistent profile tracking with medals, upgrades, credits, and military career rank
 */
#include "progress.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

static const char *kSaveFileName = "spacejam_save.dat";

static const int kUpgradeCosts[MAX_UPGRADE_LEVEL] = { 150, 320, 580, 950, 1500 };

void Progress_Init(PlayerProgress *prog) {
    memset(prog, 0, sizeof(*prog));
    prog->newlyUnlocked = -1;
    prog->awardPopupTimer = 0.0f;
    prog->credits = 250; /* Starting recruit salvage allowance */
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
    if (fread(&magic, sizeof(magic), 1, f) == 1 &&
        (magic == 0x53504A4D || magic == 0x53504A32 || magic == 0x53504A33 || magic == 0x53504A34)) {
        if (fread(&prog->totalKills, sizeof(int), 1, f) != 1) { fclose(f); return; }
        if (fread(&prog->totalShuttlesSaved, sizeof(int), 1, f) != 1) { fclose(f); return; }
        if (fread(&prog->totalBossesDefeated, sizeof(int), 1, f) != 1) { fclose(f); return; }
        if (fread(&prog->maxSectorReached, sizeof(int), 1, f) != 1) { fclose(f); return; }
        if (fread(&prog->maxWaveReached, sizeof(int), 1, f) != 1) { fclose(f); return; }
        if (fread(&prog->highestScore, sizeof(int), 1, f) != 1) { fclose(f); return; }
        if (fread(&prog->medalsBitmask, sizeof(uint32_t), 1, f) != 1) { fclose(f); return; }
        if (magic == 0x53504A32 || magic == 0x53504A33 || magic == 0x53504A34) {
            fread(&prog->relicsBitmask, sizeof(uint32_t), 1, f);
        }
        if (magic == 0x53504A33 || magic == 0x53504A34) {
            fread(&prog->endingsSeen, sizeof(uint8_t), 1, f);
        }
        if (magic == 0x53504A34) {
            fread(&prog->credits, sizeof(int), 1, f);
            fread(prog->upgradeLevels, sizeof(int), UPGRADE_COUNT, f);
        }
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

    uint32_t magic = 0x53504A34; /* 'SPJ4' with upgrades and salvage */
    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&prog->totalKills, sizeof(int), 1, f);
    fwrite(&prog->totalShuttlesSaved, sizeof(int), 1, f);
    fwrite(&prog->totalBossesDefeated, sizeof(int), 1, f);
    fwrite(&prog->maxSectorReached, sizeof(int), 1, f);
    fwrite(&prog->maxWaveReached, sizeof(int), 1, f);
    fwrite(&prog->highestScore, sizeof(int), 1, f);
    fwrite(&prog->medalsBitmask, sizeof(uint32_t), 1, f);
    fwrite(&prog->relicsBitmask, sizeof(uint32_t), 1, f);
    fwrite(&prog->endingsSeen, sizeof(uint8_t), 1, f);
    fwrite(&prog->credits, sizeof(int), 1, f);
    fwrite(prog->upgradeLevels, sizeof(int), UPGRADE_COUNT, f);
    fclose(f);
}

void Progress_AddCredits(PlayerProgress *prog, int amount) {
    if (!prog) return;
    prog->credits += amount;
    if (prog->credits < 0) prog->credits = 0;
    if (prog->credits > 999999) prog->credits = 999999;
}

int Progress_GetUpgradeCost(const PlayerProgress *prog, UpgradeType type) {
    if (!prog || type < 0 || type >= UPGRADE_COUNT) return 0;
    int curLvl = prog->upgradeLevels[type];
    if (curLvl >= MAX_UPGRADE_LEVEL) return 0;
    return kUpgradeCosts[curLvl];
}

int Progress_CanBuyUpgrade(const PlayerProgress *prog, UpgradeType type) {
    if (!prog || type < 0 || type >= UPGRADE_COUNT) return 0;
    int curLvl = prog->upgradeLevels[type];
    if (curLvl >= MAX_UPGRADE_LEVEL) return 0;
    return prog->credits >= kUpgradeCosts[curLvl];
}

int Progress_BuyUpgrade(PlayerProgress *prog, UpgradeType type) {
    if (!Progress_CanBuyUpgrade(prog, type)) return 0;
    int cost = Progress_GetUpgradeCost(prog, type);
    prog->credits -= cost;
    prog->upgradeLevels[type]++;
    Progress_Save(prog);
    return 1;
}

const char *Progress_GetUpgradeName(UpgradeType type) {
    switch (type) {
        case UPGRADE_HULL:        return "HULL REINFORCEMENT";
        case UPGRADE_SHIELD:      return "DEFLECTOR SHIELD MATRIX";
        case UPGRADE_AFTERBURNER: return "OVERCHARGED NITRO THRUST";
        case UPGRADE_OVERCHARGE:  return "PLASMA ACCELERATOR GRID";
        case UPGRADE_TORPEDOES:   return "SEEKING PHOTON TORPEDOES";
        default: return "UNKNOWN";
    }
}

const char *Progress_GetUpgradeDesc(UpgradeType type) {
    switch (type) {
        case UPGRADE_HULL:
            return "Multi-layered titanium-alloy plating. Increases max hull integrity and reduces impact damage.";
        case UPGRADE_SHIELD:
            return "Regenerative energy bubble. Absorbs incoming fire and auto-recharges when out of combat.";
        case UPGRADE_AFTERBURNER:
            return "High-efficiency vector fuel injection. Boosts top speed, burst thrust, and cooling rate.";
        case UPGRADE_OVERCHARGE:
            return "Superheated plasma coils. Increases primary laser damage, projectile size, and muzzle velocity.";
        case UPGRADE_TORPEDOES:
            return "Secondary ordnance: smart-homing micro-torpedoes that track nearest hostile dreadnoughts.";
        default: return "";
    }
}

const char *Progress_GetUpgradeStatLabel(const PlayerProgress *prog, UpgradeType type) {
    static char buf[64];
    int lvl = (prog && type >= 0 && type < UPGRADE_COUNT) ? prog->upgradeLevels[type] : 0;
    switch (type) {
        case UPGRADE_HULL:
            snprintf(buf, sizeof(buf), "Max Hull: %d HP (+%d%% Armor)", 100 + lvl * 25, lvl * 5);
            break;
        case UPGRADE_SHIELD:
            if (lvl == 0) snprintf(buf, sizeof(buf), "Deflector: Offline (Buy Tier 1 to activate)");
            else snprintf(buf, sizeof(buf), "Shield: %d SP (+%d/s Recharge)", lvl * 20, 10 + lvl * 4);
            break;
        case UPGRADE_AFTERBURNER:
            snprintf(buf, sizeof(buf), "Top Speed: %d px/s (+%d%% Boost)", 780 + lvl * 45, lvl * 15);
            break;
        case UPGRADE_OVERCHARGE:
            snprintf(buf, sizeof(buf), "Pulse Dmg: %d (+%d%% Velocity)", 12 + lvl * 3, lvl * 10);
            break;
        case UPGRADE_TORPEDOES:
            if (lvl == 0) snprintf(buf, sizeof(buf), "Payload: Locked (Secondary weapon on Key C / RB)");
            else snprintf(buf, sizeof(buf), "Payload: %s (%d Dmg, %.1fs Cd)",
                          lvl >= 5 ? "Cluster Volley" : lvl >= 3 ? "Dual Torpedo" : "Homing Torpedo",
                          50 + lvl * 20, 3.8f - lvl * 0.4f);
            break;
        default:
            snprintf(buf, sizeof(buf), "--");
            break;
    }
    return buf;
}

int Progress_HasMedal(const PlayerProgress *prog, MedalId medal) {
    if (!prog || medal < 0 || medal >= MEDAL_COUNT) return 0;
    return (prog->medalsBitmask & (1u << (uint32_t)medal)) != 0;
}

int Progress_UnlockMedal(PlayerProgress *prog, MedalId medal) {
    if (!prog || medal < 0 || medal >= MEDAL_COUNT) return 0;
    if (Progress_HasMedal(prog, medal)) return 0;

    prog->medalsBitmask |= (1u << (uint32_t)medal);
    prog->newlyUnlocked = (int)medal;
    prog->awardPopupTimer = 4.0f;

    /* Check Vanguard Legend: unlocked if all other 7 medals achieved */
    if (medal != MEDAL_VANGUARD_LEGEND) {
        uint32_t required = 0;
        for (int i = 0; i < MEDAL_VANGUARD_LEGEND; i++) required |= (1u << i);
        if ((prog->medalsBitmask & required) == required) {
            Progress_UnlockMedal(prog, MEDAL_VANGUARD_LEGEND);
        }
    }
    Progress_Save(prog);
    return 1;
}

int Progress_HasRelic(const PlayerProgress *prog, VectorId relic) {
    if (!prog || relic < 0 || relic >= VECTOR_COUNT) return 0;
    return (prog->relicsBitmask & (1u << (uint32_t)relic)) != 0;
}

int Progress_UnlockRelic(PlayerProgress *prog, VectorId relic) {
    if (!prog || relic < 0 || relic >= VECTOR_COUNT) return 0;
    if (Progress_HasRelic(prog, relic)) return 0;

    prog->relicsBitmask |= (1u << (uint32_t)relic);
    Progress_Save(prog);
    return 1;
}

int Progress_HasEnding(const PlayerProgress *prog, int endingIdx) {
    if (!prog || endingIdx < 0 || endingIdx > 2) return 0;
    return (prog->endingsSeen & (1u << endingIdx)) != 0;
}

int Progress_UnlockEnding(PlayerProgress *prog, int endingIdx) {
    if (!prog || endingIdx < 0 || endingIdx > 2) return 0;
    prog->endingsSeen |= (1u << endingIdx);
    Progress_Save(prog);
    return 1;
}

const char *Progress_GetRankName(const PlayerProgress *prog) {
    if (!prog) return "RECRUIT PILOT";
    int medals = 0;
    for (int i = 0; i < MEDAL_COUNT; i++) {
        if (Progress_HasMedal(prog, (MedalId)i)) medals++;
    }
    if (medals >= 8) return "VANGUARD FLEET ADMIRAL";
    if (medals >= 6) return "SOLAR COMMODORE";
    if (medals >= 4) return "WING COMMANDER";
    if (medals >= 2) return "INTERCEPTOR ACE";
    if (medals >= 1) return "STARFIGHTER LIEUTENANT";
    return "RECRUIT PILOT";
}

const char *Progress_GetMedalTitle(MedalId medal) {
    switch (medal) {
        case MEDAL_SAVIOR_OF_TERRA: return "SAVIOR OF TERRA";
        case MEDAL_SOLAR_ACE:       return "SOLAR CORONA ACE";
        case MEDAL_ICE_MARKSMAN:    return "ICE RINGS MARKSMAN";
        case MEDAL_CITADEL_SLAYER:  return "CITADEL CONQUEROR";
        case MEDAL_HYPER_NITRO:     return "AFTERBURNER OVERDRIVE";
        case MEDAL_IRON_WILL:       return "UNBROKEN WILL";
        case MEDAL_DEADEYE:         return "APEX HUNTER";
        case MEDAL_VANGUARD_LEGEND: return "VANGUARD LEGEND";
        default: return "UNKNOWN MEDAL";
    }
}

const char *Progress_GetMedalDesc(MedalId medal) {
    switch (medal) {
        case MEDAL_SAVIOR_OF_TERRA: return "Escort humanity's civilian evacuation convoys out of Sector 1 Terra Novus.";
        case MEDAL_SOLAR_ACE:       return "Conquer Inquisitor Malakor's Solar Dreadnought in the searing flares of Sector 2.";
        case MEDAL_ICE_MARKSMAN:    return "Navigate cryogenic asteroid hazards and shatter the Xylos Cryo-Colossus.";
        case MEDAL_CITADEL_SLAYER:  return "Assault the Iron Synod's orbital mechanical citadel and reach the Genesis Engine.";
        case MEDAL_HYPER_NITRO:     return "Sustain afterburner boost continuously for 3.5 seconds in intense dogfighting.";
        case MEDAL_IRON_WILL:       return "Survive and conquer a sector boss dreadnought with hull integrity below 25%.";
        case MEDAL_DEADEYE:         return "Amass over 10,000 tactical combat score in a single operational sortie.";
        case MEDAL_VANGUARD_LEGEND: return "Unlock all campaign medals and master the Seven Vectors of the Architects.";
        default: return "";
    }
}

const char *Progress_GetMedalIcon(MedalId medal) {
    switch (medal) {
        case MEDAL_SAVIOR_OF_TERRA: return "[TERRA]";
        case MEDAL_SOLAR_ACE:       return "[SOLAR]";
        case MEDAL_ICE_MARKSMAN:    return "[ICE]";
        case MEDAL_CITADEL_SLAYER:  return "[CITADEL]";
        case MEDAL_HYPER_NITRO:     return "[NITRO]";
        case MEDAL_IRON_WILL:       return "[WILL]";
        case MEDAL_DEADEYE:         return "[APEX]";
        case MEDAL_VANGUARD_LEGEND: return "[LEGEND]";
        default: return "[?]";
    }
}

const char *Progress_GetRelicName(VectorId relic) {
    switch (relic) {
        case VECTOR_GENESIS:   return "VECTOR I: THE GENESIS CORE";
        case VECTOR_GRAVITY:   return "VECTOR II: GRAVITATIONAL SINGULARITY";
        case VECTOR_TEMPORAL:  return "VECTOR III: TEMPORAL CHRONO-STASIS";
        case VECTOR_QUANTUM:   return "VECTOR IV: QUANTUM SLIPSTREAM";
        case VECTOR_VOID:      return "VECTOR V: VOID PHASE DEMATERIALIZATION";
        case VECTOR_NEXUS:     return "VECTOR VI: SYNOD NEXUS LINK";
        case VECTOR_ASCENDANT: return "VECTOR VII: THE ASCENDANT KEY";
        default: return "UNKNOWN RELIC";
    }
}

const char *Progress_GetRelicDesc(VectorId relic) {
    switch (relic) {
        case VECTOR_GENESIS:
            return "Passive: Architect energy infused into primary cannons. All lasers pierce asteroid clusters.";
        case VECTOR_GRAVITY:
            return "Active (Key Q / Gamepad X): Condenses a localized gravitational black hole that pulls and crushes hostile craft.";
        case VECTOR_TEMPORAL:
            return "Active (Key E / Gamepad Y): Distorts local spacetime, freezing enemies for 3.5s while rewinding ship hull integrity.";
        case VECTOR_QUANTUM:
            return "Passive: Micro-jump inertial dampening. Hyper-nitro afterburner consumes 30% less heat.";
        case VECTOR_VOID:
            return "Active (Key F / Gamepad B): Shifts ship out of normal matter phase, granting 4.5s total invulnerability.";
        case VECTOR_NEXUS:
            return "Passive: Tactical scan reveals Synod stealth craft and highlights boss weak points in amber HUD.";
        case VECTOR_ASCENDANT:
            return "Keystone: Unlocks the third path at the Genesis Engine -- True Ending: The Last Vector.";
        default: return "";
    }
}

const char *Progress_GetRelicKeyHint(VectorId relic) {
    switch (relic) {
        case VECTOR_GRAVITY:   return "[KEY Q / PAD X]";
        case VECTOR_TEMPORAL:  return "[KEY E / PAD Y]";
        case VECTOR_VOID:      return "[KEY F / PAD B]";
        default:               return "[PASSIVE]";
    }
}
