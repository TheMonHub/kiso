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
        src/time.c
        include/kiso/time.h
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

file(GLOB_RECURSE EMBED_FILES
        "../embed/*"
)

foreach (EMBED IN LISTS EMBED_FILES)
    file(RELATIVE_PATH RELATIVE_PATH
            "${CMAKE_CURRENT_SOURCE_DIR}/../embed"
            "${EMBED}"
    )

    set(EMBED_OUT "${PROJECT_BINARY_DIR}/embed/${RELATIVE_PATH}")

    get_filename_component(OUTPUT_DIR "${EMBED_OUT}" DIRECTORY)
    get_filename_component(EMBED_EXT "${EMBED}" EXT)

    if(EMBED_EXT STREQUAL ".lua")
        add_custom_command(
                OUTPUT "${EMBED_OUT}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
                COMMAND luajit -b "${EMBED}" "${EMBED_OUT}bc"
                DEPENDS "${EMBED}"
        )

    else()
        add_custom_command(
                OUTPUT "${EMBED_OUT}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${EMBED}" "${EMBED_OUT}"
                DEPENDS "${EMBED}"
        )
    endif()

    list(APPEND EMBED_OUT_FILES "${EMBED_OUT}")
endforeach()

add_custom_target(
        KisoEmbed
        DEPENDS ${EMBED_OUT_FILES}
)

add_dependencies(KisoShared KisoEmbed)

target_compile_options(KisoShared PUBLIC
        --embed-dir=${PROJECT_BINARY_DIR}/embed
)