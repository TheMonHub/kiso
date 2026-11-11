cmake_minimum_required(VERSION 4.4)
project(Kiso
        VERSION 0.1.0
        DESCRIPTION "A desktop framework you can use to make all sorts of software in embed, from games to tools."
        HOMEPAGE_URL https://github.com/TheMonHub/kiso
        LANGUAGES C
)

find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if (CLANG_TIDY_EXE)
    option(RUN_CLANG_TIDY "Run clang-tidy linter" ON)
else ()
    option(RUN_CLANG_TIDY "Run clang-tidy linter" OFF)
endif ()

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if (RUN_CLANG_TIDY)
    string(JOIN ","
            CLANG_TIDY_CHECKS
            bugprone-*
            performance-*
            clang-analyzer-*
            cert-*
            modernize-use-nodiscard
            -clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling
            -clang-diagnostic-unknown-attributes
            -bugprone-easily-swappable-parameters
            -bugprone-signed-bitwise
    )

    set(CMAKE_C_CLANG_TIDY
            clang-tidy
            --format-style=file
            "--header-filter=${PROJECT_SOURCE_DIR}/kiso/.*"
            "--checks=${CLANG_TIDY_CHECKS}"
            --warnings-as-errors=*
    )
else ()
    set(CMAKE_C_CLANG_TIDY "")
endif ()

find_package(PkgConfig REQUIRED)

# --------------------------------------------------
# Get git commit hash and branch
# --------------------------------------------------

find_package(Git QUIET)

if (GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )
endif ()

# --------------------------------------------------
# KISO Dependencies
# --------------------------------------------------

add_subdirectory(logger)

# --------------------------------------------------
# KISO
# --------------------------------------------------

add_subdirectory(kiso/shared)
add_subdirectory(kiso/vulkan)
add_subdirectory(kiso/core)