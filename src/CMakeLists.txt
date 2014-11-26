set (
    regrip_src

    cddev.c
    cddev.h
    cdpar.c
    cdpar.h
    cdplay.c
    cdplay.h
    common.h
    dialog.c
    dialog.h
    discdb.c
    discdb.h
    discedit.c
    discedit.h
    eggtrayicon.c
    eggtrayicon.h
    gain_analysis.c
    gain_analysis.h
    grip.c
    grip.h
    grip_id3.h
    gripcfg.c
    gripcfg.h
    id3.c
    launch.c
    launch.h
    main.c
    parsecfg.c
    parsecfg.h
    rip.c
    rip.h
    status_window.c
    status_window.h
    tray.c
    tray.h
    uihelper.c
    xpm.h

    cppmain.cc
)

set (
    includes

    ${PROJECT_BINARY_DIR}     # For config.h
    ${GTK2_INCLUDE_DIRS}
    ${VTE_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

set (
    libraries

    ${GTK2_LIBRARIES}
    ${VTE_LIBRARIES}
    ${CURL_LIBRARIES}
    -lcdda_interface
    -lcdda_paranoia
    -lpthread
    -lm
)

if (${CDDB_FOUND})
    set (includes ${includes} ${CDDB_INCLUDES})
    set (libraries ${libraries} ${CDDB_LIBRARIES})
endif (${CDDB_FOUND})

include_directories (
    ${includes}
)

add_executable (
    regrip
    ${regrip_src}
)

target_link_libraries (
    regrip
    ${libraries}
)
