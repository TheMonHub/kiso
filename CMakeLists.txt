cmake_minimum_required(VERSION 4.4)
project(kiso
        VERSION 1.0.0
        LANGUAGES C
)

option(VULKAN_HEADERS_ONLY "Use only Vulkan headers")
option(GLFW_HEADERS_ONLY "Use only GLFW headers")

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_C_CLANG_TIDY
        clang-tidy;
        -format-style='file';
        -header-filter='${PROJECT_SOURCE_DIR}/src/.*';
        -checks=bugprone-*,performance-*,clang-analyzer-*,cert-*,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling;
        -warnings-as-errors=*;
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
# Logger Library https://github.com/JeepWay/logger-c
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
        src/vulkan/vk_instance.c
        src/vulkan/vk_instance.h
        src/version.h
        src/macro/array_size.h
        src/error.c
        src/error.h
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
)

target_compile_definitions(kiso PRIVATE
        PROJECT_VERSION_NUMBER=${CMAKE_PROJECT_VERSION_MAJOR}${CMAKE_PROJECT_VERSION_MINOR}${CMAKE_PROJECT_VERSION_PATCH}
        PROJECT_VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
        PROJECT_VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
        PROJECT_VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
)