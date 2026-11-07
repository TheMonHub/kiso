cmake_minimum_required(VERSION 4.4)
project(kiso
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
# pkg_check_modules(cglm REQUIRED IMPORTED_TARGET cglm)
# pkg_check_modules(tinyobjloader REQUIRED IMPORTED_TARGET tinyobjloader)

pkg_check_modules(LuaJIT REQUIRED IMPORTED_TARGET luajit)

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
# KISO
# --------------------------------------------------

add_executable(kiso
        src/main.c
        src/vulkan/vk_init.c
        src/vulkan/vk_init.h
        src/version.h
        src/error.c
        src/error.h
        src/util/enum_to_str.c
        src/util/enum_to_str.h
        src/vulkan/vk_convert.c
        src/vulkan/vk_convert.h
        src/util/math.c
        src/util/math.h
        src/vulkan/vk_shader.c
        src/vulkan/vk_shader.h
        src/vulkan/vk_cleanup.c
        src/vulkan/vk_cleanup.h
        src/vulkan/vk_render.c
        src/vulkan/vk_render.h
        src/vulkan/vk_helper.h
        src/glfw/glfw_callbacks.c
        src/glfw/glfw_callbacks.h
        src/vulkan/vk_api.c
        src/vulkan/vk_api.h
        src/graphics_api.h
        src/util/common.h
        src/vulkan/vk_list.h
)

target_link_libraries(kiso
        Vulkan::Vulkan
        glfw
        logger-c
        PkgConfig::LuaJIT
)

target_include_directories(kiso PRIVATE
        src
)

target_compile_options(kiso PRIVATE
        "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/="
        "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/src/="
)

target_compile_definitions(kiso PRIVATE
        PROJECT_GITHUB_URL="${CMAKE_PROJECT_HOMEPAGE_URL}"
        PROJECT_VERSION_STRING="${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
        PROJECT_VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
        PROJECT_VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
        PROJECT_VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
)

if (VK_VALIDATE)
    target_compile_definitions(kiso PRIVATE
            VK_VALIDATE
    )
endif ()

if (GIT_COMMIT_HASH)
    message(STATUS "Git commit: ${GIT_COMMIT_HASH}")
    target_compile_definitions(kiso PRIVATE
            PROJECT_GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
    )
else ()
    message(WARNING "Could not get git commit hash")
endif ()

if (GIT_BRANCH)
    message(STATUS "Git branch: ${GIT_BRANCH}")
    target_compile_definitions(kiso PRIVATE
            PROJECT_GIT_BRANCH="${GIT_BRANCH}"
    )
else ()
    message(WARNING "Could not get git branch")
endif ()

# --------------------------------------------------
# Compile Shaders
# --------------------------------------------------

file(GLOB_RECURSE GLSL_SOURCE_FILES
        "shader/*.frag"
        "shader/*.vert"
)

foreach (GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/shader/${FILE_NAME}.spv")
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shader/"
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach (GLSL)

add_custom_target(
        Shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(kiso Shaders)

add_custom_command(TARGET kiso POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:kiso>/shader/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shader"
        "$<TARGET_FILE_DIR:kiso>/shader"
)

target_compile_options(kiso PRIVATE
        --embed-dir=${PROJECT_BINARY_DIR}/shader
)

# --------------------------------------------------