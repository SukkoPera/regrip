cmake_minimum_required (VERSION 2.6)

project (grip4)
set (VERSION_MAJOR 4)
set (VERSION_MINOR 0)
set (VERSION_REVISION 1)

include (GNUInstallDirs)
include (FindThreads)

find_package (PkgConfig)
pkg_check_modules (GTK2 REQUIRED gtk+-2.0)
pkg_check_modules (GTHREAD REQUIRED gthread-2.0)
pkg_check_modules (VTE REQUIRED vte)
pkg_check_modules (CDDB REQUIRED libcddb)

include (CheckIncludeFiles)
CHECK_INCLUDE_FILES (sys/cdio.h HAVE_SYS_CDIO_H)
CHECK_INCLUDE_FILES (io/cam/cdrom.h HAVE_IO_CAM_CDROM_H)
CHECK_INCLUDE_FILES (linux/cdrom.h HAVE_LINUX_CDROM_H)
CHECK_INCLUDE_FILES (sys/statvfs.h HAVE_SYS_STATVFS_H)
CHECK_INCLUDE_FILES (sys/vfs.h HAVE_SYS_VFS_H)


include (CheckLibraryExists)
CHECK_INCLUDE_FILES ("cdda/cdda_interface.h" HAVE_CDDA_CDDA_INTERFACE_H)
CHECK_INCLUDE_FILES ("cdda/cdda_interface.h;cdda/cdda_paranoia.h" HAVE_CDDA_CDDA_PARANOIA_H)
CHECK_INCLUDE_FILES ("cdda_interface.h" HAVE_CDDA_INTERFACE_H)
CHECK_INCLUDE_FILES ("cdda_interface.h;cdda_paranoia.h" HAVE_CDDA_PARANOIA_H)
check_library_exists ("cdda_interface" "cdda_open" "" HAVE_CDDA_INTERFACE_LIB)
check_library_exists ("cdda_paranoia" "paranoia_init" "" HAVE_CDDA_PARANOIA_LIB)

#CHECK_INCLUDE_FILES ("cddb/cddb.h" HAVE_CDDB_H)
#check_library_exists ("cddb" "cddb_disc_new" "" HAVE_CDDB_LIB)


find_package (CURL)

configure_file (
    config.h.in
    config.h
)

add_subdirectory (src)
