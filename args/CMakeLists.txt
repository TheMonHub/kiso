# --------------------------------------------------
# Arguments Library https://github.com/dmulholl/args
# Slightly modified to fix a problem in args library
# --------------------------------------------------

add_library(args STATIC
        args.c
        include/args.h
)

set_target_properties(args PROPERTIES
        SKIP_LINTING ON
)

target_compile_options(args PRIVATE -Wall -Wextra --std=c99 --pedantic -Wno-unused-parameter)

target_include_directories(args PUBLIC include)