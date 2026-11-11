if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(VK_VALIDATE "Enable Vulkan validation layers" ON)
else ()
    option(VK_VALIDATE "Enable Vulkan validation layers" OFF)
endif ()

find_package(Vulkan COMPONENTS REQUIRED)

add_library(KisoVulkan STATIC
        src/vk_init.c
        src/vk_init.h
        src/vk_convert.c
        src/vk_convert.h
        src/vk_shader.c
        src/vk_shader.h
        src/vk_cleanup.c
        src/vk_cleanup.h
        src/vk_render.c
        src/vk_render.h
        src/vk_helper.h
        src/vk_api.c
        include/kiso/vk_api.h
        src/enum_to_str.c
        src/enum_to_str.h
        src/vk_list.h
)

target_link_libraries(KisoVulkan
        Vulkan::Vulkan
        KisoShared
)

target_include_directories(KisoVulkan PRIVATE
        src
)

target_include_directories(KisoVulkan PUBLIC
        include
)

if (VK_VALIDATE)
    target_compile_definitions(KisoVulkan PRIVATE
            VK_VALIDATE
    )
endif ()

# --------------------------------------------------
# Compile Shaders
# --------------------------------------------------

file(GLOB_RECURSE GLSL_SOURCE_FILES
        "../shader/*"
        "../shader/*"
)

foreach (GLSL IN LISTS GLSL_SOURCE_FILES)
    file(RELATIVE_PATH RELATIVE_PATH
            "${CMAKE_CURRENT_SOURCE_DIR}/../shader"
            "${GLSL}"
    )

    set(SPIRV "${PROJECT_BINARY_DIR}/shader/${RELATIVE_PATH}.spv")

    get_filename_component(OUTPUT_DIR "${SPIRV}" DIRECTORY)

    add_custom_command(
            OUTPUT "${SPIRV}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V "${GLSL}" -o "${SPIRV}"
            DEPENDS "${GLSL}"
    )

    list(APPEND SPIRV_BINARY_FILES "${SPIRV}")
endforeach()

add_custom_target(
        KisoSPRIVShader
        DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(KisoVulkan KisoSPRIVShader)

target_compile_options(KisoVulkan PRIVATE
        --embed-dir=${PROJECT_BINARY_DIR}/shader
)