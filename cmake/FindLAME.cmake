FIND_PATH(LAME_INCLUDE_DIR lame/lame.h /usr/include /usr/local/include)
FIND_LIBRARY(
    LAME_LIBRARIES
    NAMES mp3lame
    PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
    /lib/x86_64-linux-gnu
    /usr/lib/x86_64-linux-gnu
)

IF (LAME_INCLUDE_DIR AND LAME_LIBRARIES)
	SET(LAME_FOUND TRUE)
ENDIF (LAME_INCLUDE_DIR AND LAME_LIBRARIES)

IF (LAME_FOUND)
	IF (NOT LAME_FIND_QUIETLY)
		MESSAGE (STATUS "Found LAME: ${LAME_INCLUDE_DIR}/lame/lame.h ${LAME_LIBRARY}")
	ENDIF (NOT LAME_FIND_QUIETLY)
ELSE (LAME_FOUND)
	IF (LAME_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could not find LAME")
	ENDIF (lame_FIND_REQUIRED)
ENDIF (LAME_FOUND)

mark_as_advanced(
  LAME_LIBRARIES
  LAME_INCLUDE_DIR
)