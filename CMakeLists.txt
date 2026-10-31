cmake_minimum_required(VERSION 4.4)
project(kiso
        VERSION 1.0.0
        HOMEPAGE_URL https://github.com/TheMonHub/kiso
        LANGUAGES C
)

option(VULKAN_HEADERS_ONLY "Use only Vulkan headers")
option(GLFW_HEADERS_ONLY "Use only GLFW headers")

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

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
        -clang-diagnostic-unknown-attributes
"

        "--warnings-as-errors=*"
)

if(VULKAN_HEADERS_ONLY)
    find_package(VulkanHeaders CONFIG REQUIRED)
else()
    find_package(Vulkan COMPONENTS REQUIRED)
endif()

if(NOT GLFW_HEADERS_ONLY)
    set(GLFW_TARGET glfw)
    find_package(glfw3 REQUIRED)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(cglm REQUIRED IMPORTED_TARGET cglm)
pkg_check_modules(tinyobjloader REQUIRED IMPORTED_TARGET tinyobjloader)

# --------------------------------------------------
# Get git commit hash and branch
# --------------------------------------------------

find_package(Git QUIET)

if(GIT_FOUND)
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
endif()

# --------------------------------------------------
# Logger Library https://github.com/JeepWay/logger-c
# Slightly modified to work nicely with this project
# --------------------------------------------------

add_library(logger-c
        logger/logger.c
        logger/logger.h
)

set_target_properties(logger-c PROPERTIES
        SKIP_LINTING ON
)

target_include_directories(logger-c PUBLIC logger)

# --------------------------------------------------

add_executable(kiso
        src/main.c
        src/vulkan/vk_init.c
        src/vulkan/vk_init.h
        src/version.h
        src/util/common_macro.h
        src/error.c
        src/error.h
        src/util/enum_to_str.c
        src/util/enum_to_str.h
        src/vulkan/vk_convert.c
        src/vulkan/vk_convert.h
)

if(VULKAN_HEADERS_ONLY)
    target_link_libraries(kiso
            Vulkan::Headers
            ${GLFW_TARGET}
            PkgConfig::cglm
            PkgConfig::tinyobjloader
            logger-c
    )
else()
    target_link_libraries(kiso
            Vulkan::Vulkan
            ${GLFW_TARGET}
            PkgConfig::cglm
            PkgConfig::tinyobjloader
            logger-c
    )
endif()

target_include_directories(kiso PRIVATE
        src
        include
)

target_compile_options(kiso PRIVATE
        "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/="
        "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/src/="
        "-Wno-unknown-attributes"
)

target_compile_definitions(kiso PRIVATE
        PROJECT_GITHUB_URL="${CMAKE_PROJECT_HOMEPAGE_URL}"
        PROJECT_VERSION_STRING="${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
        PROJECT_VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
        PROJECT_VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
        PROJECT_VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
)

if(GIT_COMMIT_HASH)
    message(STATUS "Git commit: ${GIT_COMMIT_HASH}")
    target_compile_definitions(kiso PRIVATE
            PROJECT_GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
    )
else()
    message(WARNING "Could not get git commit hash")
endif()

if(GIT_BRANCH)
    message(STATUS "Git branch: ${GIT_BRANCH}")
    target_compile_definitions(kiso PRIVATE
            PROJECT_GIT_BRANCH="${GIT_BRANCH}"
    )
else()
    message(WARNING "Could not get git branch")
endif()
