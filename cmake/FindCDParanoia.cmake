# - Find cdparanoia
# Find the native CDPARANOIA headers and libraries.
#
#  CDPARANOIA_INCLUDE_DIRS - where to find cdda_interface.h/cdda_paranoia.h.
#  CDPARANOIA_LIBRARIES    - List of libraries when using CDPARANOIA.
#  CDPARANOIA_FOUND        - True if CDPARANOIA found.

# Look for the cdda_interface.h header file.
FIND_PATH(CDDAINTF_INCLUDE_DIR cdda_interface.h
  $ENV{INCLUDE}
  "$ENV{LIB_DIR}/include"
  "$ENV{LIB_DIR}/include/cdda"
  /usr/local/include
  /usr/local/include/cdda
  /usr/include
  /usr/include/cdda
  #mingw
  c:/msys/local/include
  c:/msys/local/include/cdda
  NO_DEFAULT_PATH
  )

MARK_AS_ADVANCED(CDDAINTF_INCLUDE_DIR)


# Look for the cdda_paranoia.h header file.
FIND_PATH(CDDAPARA_INCLUDE_DIR cdda_paranoia.h
  $ENV{INCLUDE}
  "$ENV{LIB_DIR}/include"
  "$ENV{LIB_DIR}/include/cdda"
  /usr/local/include
  /usr/local/include/cdda
  /usr/include
  /usr/include/cdda
  #mingw
  c:/msys/local/include
  c:/msys/local/include/cdda
  NO_DEFAULT_PATH
  )

MARK_AS_ADVANCED(CDDAPARA_INCLUDE_DIR)


# Look for the cdda_interface library.
FIND_LIBRARY(CDDAINTF_LIBRARY NAMES cdda_interface PATHS
  $ENV{LIB}
  "$ENV{LIB_DIR}/lib"
  /usr/local/lib
  /usr/lib
  c:/msys/local/lib
  NO_DEFAULT_PATH
  )

MARK_AS_ADVANCED(CDDAINTF_LIBRARY)


# Look for the cdda_paranoia library.
FIND_LIBRARY(CDDAPARA_LIBRARY NAMES cdda_paranoia PATHS
  $ENV{LIB}
  "$ENV{LIB_DIR}/lib"
  /usr/local/lib
  /usr/lib
  c:/msys/local/lib
  NO_DEFAULT_PATH
  )

MARK_AS_ADVANCED(CDDAPARA_LIBRARY)


IF(CDDAINTF_INCLUDE_DIR)
  MESSAGE(STATUS "cdda_interface include was found")
ENDIF(CDDAINTF_INCLUDE_DIR)
IF(CDDAINTF_LIBRARY)
  MESSAGE(STATUS "cdda_interface lib was found")
ENDIF(CDDAINTF_LIBRARY)
IF(CDDAPARA_INCLUDE_DIR)
  MESSAGE(STATUS "cdda_paranoia include was found")
ENDIF(CDDAPARA_INCLUDE_DIR)
IF(CDDAPARA_LIBRARY)
  MESSAGE(STATUS "cdda_paranoia lib was found")
ENDIF(CDDAPARA_LIBRARY)

# Copy the results to the output variables.
IF(CDDAINTF_INCLUDE_DIR AND CDDAINTF_LIBRARY AND CDDAPARA_INCLUDE_DIR AND CDDAPARA_LIBRARY)
  SET(CDPARANOIA_FOUND 1)
  SET(CDPARANOIA_LIBRARIES ${CDDAINTF_LIBRARY} ${CDDAPARA_LIBRARY})
  SET(CDPARANOIA_INCLUDE_DIRS ${CDDAINTF_INCLUDE_DIR} ${CDDAPARA_INCLUDE_DIR})
ELSE(CDDAINTF_INCLUDE_DIR AND CDDAINTF_LIBRARY AND CDDAPARA_INCLUDE_DIR AND CDDAPARA_LIBRARY)
  SET(CDPARANOIA_FOUND 0)
  SET(CDPARANOIA_LIBRARIES)
  SET(CDPARANOIA_INCLUDE_DIRS)
ENDIF(CDDAINTF_INCLUDE_DIR AND CDDAINTF_LIBRARY AND CDDAPARA_INCLUDE_DIR AND CDDAPARA_LIBRARY)

# Report the results.
IF(CDPARANOIA_FOUND)
   IF (NOT CDPARANOIA_FIND_QUIETLY)
      MESSAGE(STATUS "Found CDPARANOIA: ${CDPARANOIA_LIBRARIES}")
   ENDIF (NOT CDPARANOIA_FIND_QUIETLY)
ELSE(CDPARANOIA_FOUND)
  SET(CDPARANOIA_DIR_MESSAGE "CDPARANOIA was not found.")

  IF(CDPARANOIA_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "${CDPARANOIA_DIR_MESSAGE}")
  ELSE(CDPARANOIA_FIND_REQUIRED)
    IF(NOT CDPARANOIA_FIND_QUIETLY)
      MESSAGE(STATUS "${CDPARANOIA_DIR_MESSAGE}")
    ENDIF(NOT CDPARANOIA_FIND_QUIETLY)
    # Avoid cmake complaints if CDPARANOIA is not found
    SET(CDDAINTF_INCLUDE_DIR "")
    SET(CDDAPARA_INCLUDE_DIR "")
	SET(CDDAINTF_LIBRARY "")
    SET(CDDAPARA_LIBRARY "")
  ENDIF(CDPARANOIA_FIND_REQUIRED)

ENDIF(CDPARANOIA_FOUND)
