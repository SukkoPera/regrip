cmake_minimum_required (VERSION 2.6)

project (regrip)
set (VERSION_MAJOR 4)
set (VERSION_MINOR 0)
set (VERSION_REVISION 1)

include (GNUInstallDirs)
#include (FindThreads)

# Define some directories that will be useful later, mainly for installation
set (DATADIR "${CMAKE_INSTALL_PREFIX}/share")
set (HELPDIR "${CMAKE_INSTALL_DATADIR}/help/grip/C")


find_package (PkgConfig)
pkg_check_modules (GTK2 REQUIRED gtk+-2.0)
pkg_check_modules (GTHREAD REQUIRED gthread-2.0)
pkg_check_modules (VTE REQUIRED vte)
#pkg_check_modules (CDDB REQUIRED libcddb)
pkg_check_modules (SNDFILE sndfile)

include (CheckIncludeFiles)
CHECK_INCLUDE_FILES (sys/cdio.h HAVE_SYS_CDIO_H)
CHECK_INCLUDE_FILES (io/cam/cdrom.h HAVE_IO_CAM_CDROM_H)
CHECK_INCLUDE_FILES (linux/cdrom.h HAVE_LINUX_CDROM_H)
CHECK_INCLUDE_FILES (sys/statvfs.h HAVE_SYS_STATVFS_H)
CHECK_INCLUDE_FILES (sys/vfs.h HAVE_SYS_VFS_H)

#CHECK_INCLUDE_FILES ("cddb/cddb.h" HAVE_CDDB_H)
#check_library_exists ("cddb" "cddb_disc_new" "" HAVE_CDDB_LIB)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package (CURL)
find_package (CDParanoia REQUIRED)
find_package (FAAC)

configure_file (
    config.h.in
    config.h
)

add_subdirectory (src)
add_subdirectory (doc)
add_subdirectory (pixmaps)


### INSTALLATION STUFF ###

# install our .desktop file so the Applications menu will see it
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/regrip.desktop DESTINATION ${DATADIR}/applications/)
