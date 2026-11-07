# --------------------------------------------------
# Logger Library https://github.com/JeepWay/logger-c
# Slightly modified to work nicely with this project
# --------------------------------------------------

add_library(logger-c STATIC
        logger.c
        include/logger.h
)

set_target_properties(logger-c PROPERTIES
        SKIP_LINTING ON
)

target_include_directories(logger-c PUBLIC include)