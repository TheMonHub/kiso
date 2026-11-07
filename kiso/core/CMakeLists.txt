add_executable(KisoCore
        src/main.c
)

target_link_libraries(KisoCore
        KisoShared
        KisoVulkan
)

target_include_directories(KisoCore PRIVATE
        src
)