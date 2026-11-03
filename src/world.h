/**
 * world.h - Cosmic environments: dynamic planetary sectors, parallax stars,
 *           celestial bodies (Terra Novus, Blazing Sun, Ringed Giant, Synod Citadel),
 *           and hyperspace warp transitions.
 */
#ifndef SPACEJAM_WORLD_H
#define SPACEJAM_WORLD_H

#include "raylib.h"
#include "config.h"

typedef enum {
    SECTOR_TERRA_NOVUS = 0, /* Waves 1-3: Shattered Earth cradle world */
    SECTOR_SOLAR_CORONA,    /* Waves 4-6: Blazing dying sun & solar flares */
    SECTOR_XYLOS_RINGS,     /* Waves 7-9: Emerald gas giant with ice rings */
    SECTOR_SYNOD_CITADEL,   /* Waves 10+: Ominous mechanical orbital fortress */
    SECTOR_COUNT
} WorldSector;

typedef struct {
    Vector2 pos;
    float   brightness;
    float   size;
    float   twinkleSpeed;
    float   twinklePhase;
    int     hasGlint;
} Star;

#define MAX_NEBULA_CLOUDS 12

typedef struct {
    Vector2 pos;
    float   radius;
    Color   color;
    float   speed;
} NebulaCloud;

typedef struct {
    Star        layers[STAR_LAYERS][STARS_PER_LAYER];
    float       speeds[STAR_LAYERS];
    NebulaCloud nebulae[MAX_NEBULA_CLOUDS];
    float       time;
    WorldSector sector;
    float       warpTimer;    /* > 0 during hyperspace warp jump */
    float       warpDuration;
    Texture2D   texSectors[SECTOR_COUNT];
    int         texturesLoaded;
} Starfield;

void        Starfield_Init(Starfield *sf, int screenW, int screenH);
void        Starfield_Shutdown(Starfield *sf);
void        Starfield_Update(Starfield *sf, float dt, int screenW, int screenH);
void        Starfield_Draw(const Starfield *sf);
void        Starfield_SetSector(Starfield *sf, WorldSector sector, int triggerWarp);
WorldSector Starfield_GetSectorForWave(int wave);
const char *Starfield_GetSectorName(WorldSector sector);
const char *Starfield_GetSectorSubtitle(WorldSector sector);

#endif /* SPACEJAM_WORLD_H */
