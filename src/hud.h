/**
 * hud.h - Heads-up display rendering
 */
#ifndef SPACEJAM_HUD_H
#define SPACEJAM_HUD_H

#include "raylib.h"
#include "game.h"
#include "player.h"
#include "enemy.h"

void HUD_Draw(const GameContext *ctx, const Player *player,
              const EnemyPool *enemies, int screenW, int screenH);

/* Boss health bar (drawn separately so it can slide in/out) */
void HUD_DrawBossBar(const Enemy *boss, int screenW, int screenH);

#endif /* SPACEJAM_HUD_H */

