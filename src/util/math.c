// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#include "math.h"

uint32_t UnsignedClamp(const uint32_t value, const uint32_t min, const uint32_t max) {
    return value < min ? min : value > max ? max : value;
}
