pkg_check_modules(LuaJIT REQUIRED IMPORTED_TARGET luajit)

# pkg_check_modules(cglm REQUIRED IMPORTED_TARGET cglm)
# pkg_check_modules(tinyobjloader REQUIRED IMPORTED_TARGET tinyobjloader)

add_library(KisoShared STATIC
        src/glfw/glfw_callbacks.c
        include/kiso/glfw/glfw_callbacks.h
        include/kiso/common.h
        include/kiso/graphics_api.h
        include/kiso/version.h
        src/error.c
        include/kiso/error.h
        src/math.c
        include/kiso/math.h
)

target_include_directories(KisoShared PUBLIC
        include
)

target_link_libraries(KisoShared PUBLIC
        PkgConfig::LuaJIT
        glfw
        logger-c
)

target_compile_options(KisoShared PUBLIC
        "-fmacro-prefix-map=${PROJECT_SOURCE_DIR}/kiso=kiso"
)

target_compile_definitions(KisoShared PUBLIC
        PROJECT_GITHUB_URL="${CMAKE_PROJECT_HOMEPAGE_URL}"
        PROJECT_VERSION_STRING="${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
        PROJECT_VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
        PROJECT_VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
        PROJECT_VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
)

if (GIT_COMMIT_HASH)
    message(STATUS "Git commit: ${GIT_COMMIT_HASH}")
    target_compile_definitions(KisoShared PUBLIC
            PROJECT_GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
    )
else ()
    message(WARNING "Could not get git commit hash")
endif ()

if (GIT_BRANCH)
    message(STATUS "Git branch: ${GIT_BRANCH}")
    target_compile_definitions(KisoShared PUBLIC
            PROJECT_GIT_BRANCH="${GIT_BRANCH}"
    )
else ()
    message(WARNING "Could not get git branch")
endif ()