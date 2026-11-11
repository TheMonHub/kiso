// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <threads.h>

#include "lua.h"
#include "kiso/time.h"

int main(const int argc, char *argv[]) {
    lua_State *state = nullptr;
    const int status = RunAppFolder("../../example", &state, argc - 1, argv + 1);
    if (status != 0) {
        lua_close(state);
        return status;
    }

    double previous_time = GetMonotonicTime();

    while (!ShouldExit()) {
        handlePossibleExitRequest(state);

        const double current_time = GetMonotonicTime();
        const double deltaTime = current_time - previous_time;
        const double limit_update_rate = GetLimitCallbacksRate();

        if (limit_update_rate == 0 || deltaTime >= 1.0 / limit_update_rate) {
            previous_time = current_time;
            runUpdate(state, deltaTime);
            continue;
        }

        const double remaining_time = 1.0 / limit_update_rate - deltaTime;

        struct timespec ts = {
            .tv_sec = 0,
            .tv_nsec = (long)(remaining_time * 1'000'000'000)
        };

        (void)thrd_sleep(&ts, NULL);
    }

    lua_close(state);
    return GetCurrentExitCode();
}
