find_package (LAME REQUIRED)

if (LAME_FOUND)
	add_library (
		enc_lame

		MODULE

		enc_lame.c
	)

	include_directories (
		${PROJECT_BINARY_DIR}		# For config.h
		${HEADER_DIR}				# For encoder.h
		${GLIB_INCLUDE_DIRS}
		${GMODULE_INCLUDE_DIRS}
		${LAME_INCLUDE_DIR}
	)

	target_link_libraries (
		enc_lame

		${GLIB_LIBRARIES}
		${GMODULE_LIBRARIES}
		${LAME_LIBRARIES}
	)

	set_property (
		TARGET enc_lame
		PROPERTY LIBRARY_OUTPUT_DIRECTORY ${MODULE_DIR}
	)
else (LAME_FOUND)
	message (WARNING "LAME was not found on the system, MP3 encoder module will not be built")
endif (LAME_FOUND)
