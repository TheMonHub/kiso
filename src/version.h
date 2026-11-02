#ifndef VERSION_H
#define VERSION_H

#include <stdint.h>

#define MAKE_VERSION(variant, major, minor, patch) \
    ((((uint32_t)(variant)) << 29U) | (((uint32_t)(major)) << 22U) | (((uint32_t)(minor)) << 12U) | ((uint32_t)(patch))); \
    static_assert((variant) < 8 - 1, "A variant must be less than 8"); \
    static_assert((major) < 128 - 1, "A major version must be less than 128"); \
    static_assert((minor) < 1024 - 1, "A minor version must be less than 1024"); \
    static_assert((patch) < 4096 - 1, "A patch version must be less than 4096");

#define VERSION_VARIANT(version) ((uint32_t)(version) >> 29U)
#define VERSION_MAJOR(version) (((uint32_t)(version) >> 22U) & 0x7FU)
#define VERSION_MINOR(version) (((uint32_t)(version) >> 12U) & 0x3FFU)
#define VERSION_PATCH(version) ((uint32_t)(version) & 0xFFFU)

constexpr uint32_t kProjectVersionNumber = MAKE_VERSION(0, PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH);
constexpr uint32_t kProjectVersionMajor = PROJECT_VERSION_MAJOR;
constexpr uint32_t kProjectVersionMinor = PROJECT_VERSION_MINOR;
constexpr uint32_t kProjectVersionPatch = PROJECT_VERSION_PATCH;

#endif // VERSION_H
