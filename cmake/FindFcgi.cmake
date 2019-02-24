# - Try to find the SQLite3 library
# Once done this will define
#
#  FCGI_FOUND - System has SQLite3
#  FCGI_INCLUDE_DIR - The SQLite3 include directory
#  FCGI_LIBRARIES - The libraries needed to use SQLite3
#  FCGI_DEFINITIONS - Compiler switches required for using SQLite3
#  FCGI_EXECUTABLE - The SQLite3 command line shell
#  FCGI_VERSION_STRING - the version of SQLite3 found (since CMake 2.8.8)

#=============================================================================
# Copyright 2006-2009 Kitware, Inc.
# Copyright 2006 Alexander Neundorf <neundorf@kde.org>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
find_package(PkgConfig QUIET)
PKG_CHECK_MODULES(PC_FCGI QUIET fcgi)
set(FCGI_DEFINITIONS ${PC_FCGI_CFLAGS_OTHER})

find_path(FCGI_INCLUDE_DIR NAMES fastcgi.h
   HINTS
   ${PC_FCGI_INCLUDEDIR}
   ${PC_FCGI_INCLUDE_DIRS}
   )

find_library(FCGI_LIBRARIES NAMES fcgi
   HINTS
   ${PC_FCGI_LIBDIR}
   ${PC_FCGI_LIBRARY_DIRS}
   )

find_program(FCGI_EXECUTABLE sqlite3)

if(PC_FCGI_VERSION)
    set(FCGI_VERSION_STRING ${PC_FCGI_VERSION})
elseif(FCGI_INCLUDE_DIR AND EXISTS "${FCGI_INCLUDE_DIR}/fastcgi.h")
    file(STRINGS "${FCGI_INCLUDE_DIR}/fastcgi.h" fastcgi_version_str
         REGEX "^#define[\t ]+FCGI_VERSION_1[\t ]+\".*\"")

    string(REGEX REPLACE "^#define[\t ]+FCGI_VERSION[\t ]+\"([^\"]*)\".*" "\\1"
           FCGI_VERSION_STRING "${fastcgi_version_str}")
    unset(sqlite3_version_str)
endif()

# handle the QUIETLY and REQUIRED arguments and set FCGI_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FCgi
                                  REQUIRED_VARS FCGI_LIBRARIES FCGI_INCLUDE_DIR
                                  VERSION_VAR FCGI_VERSION_STRING)

mark_as_advanced(FCGI_INCLUDE_DIR FCGI_LIBRARIES FCGI_EXECUTABLE)
