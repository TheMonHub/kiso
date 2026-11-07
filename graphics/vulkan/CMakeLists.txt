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
        "../shader/*.frag"
        "../shader/*.vert"
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
        KisoSPRIVShader
        DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(KisoVulkan KisoSPRIVShader)

target_compile_options(KisoVulkan PRIVATE
        --embed-dir=${PROJECT_BINARY_DIR}/shader
)