// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#ifndef INIT_H
#define INIT_H

#include "lualib.h"
#include "lua.h"

void HandleLuaError(lua_State *state);
void handlePossibleExitRequest(lua_State *state);
void runUpdate(lua_State *state, double deltaTime);

bool ShouldExit();
int GetCurrentExitCode();
double GetLimitUpdateRate();
int RunAppFolder(const char *path, lua_State **out, int argc, char *argv[]);

#endif // INIT_H
