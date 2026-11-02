/**
 * highscore.h - Persistent high-score storage
 */
#ifndef SPACEJAM_HIGHSCORE_H
#define SPACEJAM_HIGHSCORE_H

#include <stdint.h>
#include "config.h"

#define HIGHSCORE_NAME_MAX 16

typedef struct {
    char    name[HIGHSCORE_NAME_MAX];
    int32_t score;
    int32_t wave;
} HighScoreEntry;

typedef struct {
    uint32_t       magic;
    uint32_t       count;
    HighScoreEntry entries[HIGHSCORE_MAX_ENTRIES];
    uint32_t       checksum;
} HighScoreTable;

void HighScore_Load(HighScoreTable *table);
void HighScore_Save(const HighScoreTable *table);
void HighScore_Insert(HighScoreTable *table, int score, int wave, const char *name);
int  HighScore_IsNew(const HighScoreTable *table, int score);

/* Returns the top score, or 0 if table is empty */
int  HighScore_GetTop(const HighScoreTable *table);

#endif /* SPACEJAM_HIGHSCORE_H */

