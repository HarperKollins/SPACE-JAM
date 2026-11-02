/**
 * settings.h - Settings persistence
 */
#ifndef SPACEJAM_SETTINGS_H
#define SPACEJAM_SETTINGS_H

#include "game.h"

void Settings_LoadDefaults(GameSettings *s);
void Settings_Load(GameSettings *s);
void Settings_Save(const GameSettings *s);

#endif /* SPACEJAM_SETTINGS_H */

