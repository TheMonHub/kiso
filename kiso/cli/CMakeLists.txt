add_executable(KisoCli
        src/main.c
        src/verify.c
        src/verify.h
        src/new_cmd_callback.h
        src/run_cmd_callback.h
        src/pack_cmd_callback.h
)

set_target_properties(KisoCli PROPERTIES OUTPUT_NAME "kiso")

include(GNUInstallDirs)
install(TARGETS KisoCli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_link_libraries(KisoCli
        KisoCore
        args
)

target_include_directories(KisoCli PRIVATE
        src
)