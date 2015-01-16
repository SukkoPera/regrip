find_package (FAAC REQUIRED)

if (FAAC_FOUND)
	add_library (
		enc_faac

		MODULE

		enc_faac.c
	)

	include_directories (
		${PROJECT_BINARY_DIR}		# For config.h
		${HEADER_DIR}				# For encoder.h
		${GLIB_INCLUDE_DIRS}
		${GMODULE_INCLUDE_DIRS}
		${FAAC_INCLUDE_DIR}
	)

	target_link_libraries (
		enc_faac

		${GLIB_LIBRARIES}
		${GMODULE_LIBRARIES}
		${FAAC_LIBRARIES}
	)

	set_property (
		TARGET enc_faac
		PROPERTY LIBRARY_OUTPUT_DIRECTORY ${MODULE_DIR}
	)
else (FAAC_FOUND)
	message (WARNING "FAAC was not found on the system, AAC encoder module will not be built")
endif (FAAC_FOUND)

