pkg_check_modules (OPUS opus)
pkg_check_modules (OGG ogg)
pkg_check_modules (SAMPLERATE samplerate)

if (OPUS_FOUND AND OGG_FOUND AND SAMPLERATE_FOUND)
	add_library (
		enc_opus

		MODULE

        enc_opus.c
        opus_header.c
        opus_header.h
	)

	include_directories (
		${PROJECT_BINARY_DIR}		# For config.h
		${HEADER_DIR}				# For encoder.h
		${GLIB_INCLUDE_DIRS}
		${GMODULE_INCLUDE_DIRS}
        ${OPUS_INCLUDE_DIRS}
        ${OGG_INCLUDE_DIRS}
        ${SAMPLERATE_INCLUDE_DIRS}
	)

	target_link_libraries (
		enc_opus

		${GLIB_LIBRARIES}
		${GMODULE_LIBRARIES}
        ${OPUS_LIBRARIES}
        ${OGG_LIBRARIES}
        ${SAMPLERATE_LIBRARIES}
	)

	set_property (
		TARGET enc_opus
		PROPERTY LIBRARY_OUTPUT_DIRECTORY ${MODULE_DIR}
	)
else (OPUS_FOUND AND OGG_FOUND AND SAMPLERATE_FOUND)
	message (WARNING "opus, libogg and/or libsamplerate was/were not found on the system, Opus encoder module will not be built")
endif (OPUS_FOUND AND OGG_FOUND AND SAMPLERATE_FOUND)

