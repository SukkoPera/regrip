pkg_check_modules (SNDFILE REQUIRED sndfile)

if (SNDFILE_FOUND)
	add_library (
		enc_sndfile

		MODULE

		enc_sndfile.c
	)

	include_directories (
		${PROJECT_BINARY_DIR}		# For config.h
		${HEADER_DIR}				# For encoder.h
		${GLIB_INCLUDE_DIRS}
		${GMODULE_INCLUDE_DIRS}
		${SNDFILE_INCLUDE_DIRS}
	)

	target_link_libraries (
		enc_sndfile

		${GLIB_LIBRARIES}
		${GMODULE_LIBRARIES}
		${SNDFILE_LIBRARIES}
	)

	set_property (
		TARGET enc_sndfile
		PROPERTY LIBRARY_OUTPUT_DIRECTORY ${MODULE_DIR}
	)
else (SNDFILE_FOUND)
	message (WARNING "libsndfile was not found on the system, WAV/OGG/FLAC encoder module will not be built")
endif (SNDFILE_FOUND)

