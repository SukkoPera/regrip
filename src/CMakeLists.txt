set (
    regrip_src

    common.h
    cddev.c
    cddev.h
    cdpar.c
    cdpar.h
    cdplay.c
    cdplay.h
    circbuf.c
    circbuf.h
    dialog.c
    dialog.h
    discdb.c
    discdb.h
    discedit.c
    discedit.h
    eggtrayicon.c
    eggtrayicon.h
    encoder.c
    encoder.h
    gain_analysis.c
    gain_analysis.h
    grip.c
    grip.h
    gripcfg.c
    gripcfg.h
    launch.c
    launch.h
    main.c
    rip.c
    rip.h
    status_window.c
    status_window.h
    tag.c
    tag.h
    tray.c
    tray.h
    uihelper.c
    uihelper.h
    xpm.h
)

set (
    includes

    ${PROJECT_BINARY_DIR}     # For config.h
    ${GTK2_INCLUDE_DIRS}
    ${GTHREAD_INCLUDE_DIRS}
    ${VTE_INCLUDE_DIRS}
    ${CDPARANOIA_INCLUDE_DIRS}
    ${CDIO_INCLUDE_DIRS}
#    ${CURL_INCLUDE_DIRS}
)

set (
    libraries

    -lm			# For gain analysis
    ${GTK2_LIBRARIES}
    ${GTHREAD_LIBRARIES}
    ${GTHREAD_LDFLAGS_OTHER}
    ${VTE_LIBRARIES}
#    ${CURL_LIBRARIES}
    ${CDPARANOIA_LIBRARIES}
    ${CDIO_LIBRARIES}
)

if (CDDB_FOUND)
    set (
        includes
        ${includes}
        ${CDDB_INCLUDE_DIRS}
    )

    set (
        libraries
        ${libraries}
        ${CDDB_LIBRARIES}
    )
endif (CDDB_FOUND)


if (TAGLIB_FOUND)
    set (
        includes
        ${includes}
        ${TAGLIB_INCLUDE_DIRS}
    )

    set (
        libraries
        ${libraries}
        ${TAGLIB_LIBRARIES}
    )
endif (TAGLIB_FOUND)

if (SNDFILE_FOUND)
    set (
        regrip_src
        ${regrip_src}

        encoder_sndfile.c
        encoder_sndfile.h
    )

    set (
        includes
        ${includes}
        ${SNDFILE_INCLUDE_DIRS}
    )

    set (
        libraries
        ${libraries}
        ${SNDFILE_LIBRARIES}
    )
endif (SNDFILE_FOUND)

if (FAAC_FOUND)
    set (
        encoder_faac_src
        encoder_faac.c
        encoder_faac.h
    )

    set (
        regrip_src
        ${regrip_src}
        ${encoder_faac_src}
    )

    set (
        includes
        ${includes}
        ${FAAC_INCLUDE_DIR}
    )

    set (
        libraries
        ${libraries}
        ${FAAC_LIBRARIES}
    )

    add_executable (
        faactest
        EXCLUDE_FROM_ALL

        ${encoder_faac_src}
#        circbuf.c
#        circbuf.h
        codectest.c
    )

    target_link_libraries (
        faactest
        ${SNDFILE_LIBRARIES}
        ${FAAC_LIBRARIES}
        ${GTK2_LIBRARIES}
    )

    target_compile_definitions (
        faactest
        PUBLIC -DTESTENC=faac
    )
endif (FAAC_FOUND)

if (OPUS_FOUND AND OGG_FOUND AND SAMPLERATE_FOUND)
    set (
        encoder_opus_src

        encoder_opus.c
        encoder_opus.h
        opus_header.c
        opus_header.h
    )

    set (
        regrip_src
        ${regrip_src}
        ${encoder_opus_src}
    )

    set (
        includes
        ${includes}
        ${OPUS_INCLUDE_DIRS}
        ${OGG_INCLUDE_DIRS}
        ${SAMPLERATE_INCLUDE_DIRS}
    )

    set (
        libraries
        ${libraries}
        ${OPUS_LIBRARIES}
        ${OGG_LIBRARIES}
        ${SAMPLERATE_LIBRARIES}
    )

    add_executable (
        opustest
        EXCLUDE_FROM_ALL

        ${encoder_opus_src}
        circbuf.c
        circbuf.h
        codectest.c
    )

    target_link_libraries (
        opustest
        ${SNDFILE_LIBRARIES}
        ${OPUS_LIBRARIES}
        ${OGG_LIBRARIES}
        ${SAMPLERATE_LIBRARIES}
        ${GTK2_LIBRARIES}
    )

    target_compile_definitions (
        opustest
        PUBLIC -DTESTENC=opus
    )
endif (OPUS_FOUND AND OGG_FOUND AND SAMPLERATE_FOUND)

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


### INSTALLATION STUFF ###

# install the binaries we just made
install (TARGETS regrip RUNTIME DESTINATION bin)
