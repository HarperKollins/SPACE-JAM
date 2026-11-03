/**
 * highscore.c - High score save/load with validation checksum
 */
#include "highscore.h"
#include "config.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Simple additive checksum over entries */
static uint32_t ComputeChecksum(const HighScoreTable *table) {
    uint32_t sum = table->magic ^ table->count;
    for (uint32_t i = 0; i < table->count && i < HIGHSCORE_MAX_ENTRIES; i++) {
        sum ^= (uint32_t)table->entries[i].score;
        sum ^= (uint32_t)table->entries[i].wave;
        const char *n = table->entries[i].name;
        for (int c = 0; n[c]; c++) sum += (uint32_t)n[c];
        sum = (sum << 1) | (sum >> 31);
    }
    return sum;
}

void HighScore_Load(HighScoreTable *table) {
    memset(table, 0, sizeof(*table));
    table->magic = HIGHSCORE_MAGIC;

    FILE *f = fopen(HIGHSCORE_FILE, "rb");
    if (!f) return; /* no file yet - start fresh */

    HighScoreTable tmp;
    size_t read = fread(&tmp, 1, sizeof(tmp), f);
    fclose(f);

    if (read != sizeof(tmp)) return; /* truncated */
    if (tmp.magic != HIGHSCORE_MAGIC) return; /* wrong format */
    if (tmp.count > HIGHSCORE_MAX_ENTRIES) return; /* corrupted */

    /* Validate checksum */
    uint32_t expected = ComputeChecksum(&tmp);
    if (tmp.checksum != expected) {
        fprintf(stderr, "[highscore] Checksum mismatch - starting fresh.\n");
        return;
    }

    *table = tmp;
}

void HighScore_Save(const HighScoreTable *table) {
    HighScoreTable tmp = *table;
    tmp.checksum = ComputeChecksum(&tmp);

    FILE *f = fopen(HIGHSCORE_FILE, "wb");
    if (!f) {
        fprintf(stderr, "[highscore] Failed to open save file for writing.\n");
        return;
    }
    fwrite(&tmp, 1, sizeof(tmp), f);
    fclose(f);
}

int HighScore_GetTop(const HighScoreTable *table) {
    if (table->count == 0) return 0;
    return table->entries[0].score;
}

int HighScore_IsNew(const HighScoreTable *table, int score) {
    if (table->count < HIGHSCORE_MAX_ENTRIES) return 1;
    return score > table->entries[table->count - 1].score;
}

void HighScore_Insert(HighScoreTable *table, int score, int wave, const char *name) {
    if (!HighScore_IsNew(table, score)) return;

    uint32_t n = table->count < HIGHSCORE_MAX_ENTRIES ?
                 table->count : HIGHSCORE_MAX_ENTRIES - 1;
    table->count = n + 1;
    if (table->count > HIGHSCORE_MAX_ENTRIES) table->count = HIGHSCORE_MAX_ENTRIES;

    /* Find insertion point */
    int ins = (int)n;
    for (int i = 0; i < (int)n; i++) {
        if (score > table->entries[i].score) { ins = i; break; }
    }

    /* Shift down */
    for (int i = (int)n - 1; i >= ins; i--) {
        table->entries[i + 1] = table->entries[i];
    }

    /* Insert */
    table->entries[ins].score = (int32_t)score;
    table->entries[ins].wave  = (int32_t)wave;
    strncpy(table->entries[ins].name, name ? name : "ACE", HIGHSCORE_NAME_MAX - 1);
    table->entries[ins].name[HIGHSCORE_NAME_MAX - 1] = '\0';
}

