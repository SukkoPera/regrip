cmake_minimum_required (VERSION 2.8.11)

project (regrip)
#set (VERSION_MAJOR 4)
#set (VERSION_MINOR 0)
#set (VERSION_REVISION 1)
string(TIMESTAMP VERSION_MAJOR "%Y")
string(TIMESTAMP VERSION_MINOR "%m")
string(TIMESTAMP VERSION_REVISION "%d-svn")

include (GNUInstallDirs)

# Define some directories that will be useful later, mainly for installation
set (DATADIR "${CMAKE_INSTALL_PREFIX}/share")
set (HELPDIR "${CMAKE_INSTALL_DATADIR}/help/grip/C")


find_package (PkgConfig)
pkg_check_modules (GTHREAD REQUIRED gthread)
pkg_check_modules (GTK2 REQUIRED gtk+-2.0)
pkg_check_modules (VTE REQUIRED vte)
pkg_check_modules (TAGLIB taglib_c)
pkg_check_modules (CDDB libcddb)
pkg_check_modules (SNDFILE sndfile)
pkg_check_modules (OPUS opus)
pkg_check_modules (OGG ogg)
pkg_check_modules (SAMPLERATE samplerate)

include (CheckIncludeFiles)
CHECK_INCLUDE_FILES (sys/cdio.h HAVE_SYS_CDIO_H)
CHECK_INCLUDE_FILES (io/cam/cdrom.h HAVE_IO_CAM_CDROM_H)
CHECK_INCLUDE_FILES (linux/cdrom.h HAVE_LINUX_CDROM_H)
CHECK_INCLUDE_FILES (sys/statvfs.h HAVE_SYS_STATVFS_H)
CHECK_INCLUDE_FILES (sys/vfs.h HAVE_SYS_VFS_H)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package (CURL REQUIRED)
find_package (CDParanoia REQUIRED)
find_package (FAAC)

configure_file (
    config.h.in
    config.h
)

add_subdirectory (src)
add_subdirectory (doc)
add_subdirectory (pixmaps)
add_subdirectory (gschema)


### INSTALLATION STUFF ###

# install our .desktop file so the Applications menu will see it
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/regrip.desktop DESTINATION ${DATADIR}/applications/)
