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

/* Boss health bar */
void HUD_DrawBossBar(const Enemy *boss, int wave, int screenW, int screenH);

/* Medal award pop-up notification */
void HUD_DrawMedalPopup(int medalId, float timer, int screenW);

#endif /* SPACEJAM_HUD_H */

