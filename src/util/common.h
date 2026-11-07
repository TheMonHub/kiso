#ifndef COMMON_H
#define COMMON_H

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define TO_STR(x) #x
#define TO_STR_EXPAND(x) TO_STR(x)

#define LINE_SEPARATOR "==========================="

constexpr int kMaxFramesInFlight = 2;

#endif // COMMON_H
