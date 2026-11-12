add_library(KisoCore STATIC
        src/app.c
        include/kiso/app.h
)

target_link_libraries(KisoCore
        KisoShared
        KisoVulkan
)

target_include_directories(KisoCore PRIVATE
        src
)

target_include_directories(KisoCore PUBLIC
        include
)