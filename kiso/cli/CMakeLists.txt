add_executable(KisoCli
        src/main.c
)

set_target_properties(KisoCli PROPERTIES OUTPUT_NAME "kiso")

include(GNUInstallDirs)
install(TARGETS KisoCli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_link_libraries(KisoCli
        KisoCore
)

target_include_directories(KisoCli PRIVATE
        src
)

file(GLOB_RECURSE EXAMPLE_FILES
        "../example/*"
)

foreach (EXAMPLE IN LISTS EXAMPLE_FILES)
    file(RELATIVE_PATH RELATIVE_PATH
            "${CMAKE_CURRENT_SOURCE_DIR}/../example"
            "${EXAMPLE}"
    )

    set(EXAMPLE_OUT "${PROJECT_BINARY_DIR}/example/${RELATIVE_PATH}")

    get_filename_component(OUTPUT_DIR "${EXAMPLE_OUT}" DIRECTORY)

    add_custom_command(
            OUTPUT "${EXAMPLE_OUT}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${EXAMPLE}" "${EXAMPLE_OUT}"
            DEPENDS "${EXAMPLE}"
    )

    list(APPEND EXAMPLE_OUT_FILES "${EXAMPLE_OUT}")
endforeach()

add_custom_target(
        KisoExample
        DEPENDS ${EXAMPLE_OUT_FILES}
)

add_dependencies(KisoCli KisoExample)