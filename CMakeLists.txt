cmake_minimum_required(VERSION 4.4)
project(Kiso
        VERSION 1.0.0
        HOMEPAGE_URL https://github.com/TheMonHub/kiso
        LANGUAGES C
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(VK_VALIDATE "Enable Vulkan validation layers" ON)
else ()
    option(VK_VALIDATE "Enable Vulkan validation layers" OFF)
endif ()

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
    set(CMAKE_C_CLANG_TIDY
            clang-tidy
            "--format-style=file"
            "--header-filter=${PROJECT_SOURCE_DIR}/src/.*"

            "
            --checks=
            bugprone-*,
            performance-*,
            clang-analyzer-*,
            cert-*,
            modernize-use-nodiscard,
            -clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling,
            -clang-diagnostic-unknown-attributes,
            -bugprone-easily-swappable-parameters,
            -bugprone-signed-bitwise
    "

            "--warnings-as-errors=*"
    )
else ()
    set(CMAKE_C_CLANG_TIDY "")
endif ()

find_package(Vulkan COMPONENTS REQUIRED)

find_package(glfw3 REQUIRED)
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

add_subdirectory(shared)
add_subdirectory(graphics/vulkan)
add_subdirectory(core)