# - Try to find the cJSON library
# Once done this will define
#
#  CJSON_FOUND - System has cJSON
#  CJSON_INCLUDE_DIR - The cJSON include directory
#  CJSON_LIBRARIES - The libraries needed to use cJSON
#  CJSON_DEFINITIONS - Compiler switches required for using cJSON
#  CJSON_VERSION_STRING - the version of cJSON found

# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
find_package(PkgConfig QUIET)
PKG_CHECK_MODULES(PC_CJSON QUIET cjson)
set(CJSON_DEFINITIONS ${PC_CJSON_CFLAGS_OTHER})

find_path(CJSON_INCLUDE_DIR NAMES cJSON.h
   HINTS
   cjson
   ${PC_CJSON_INCLUDEDIR}
   ${PC_CJSON_INCLUDE_DIRS}
   )

find_library(CJSON_LIBRARIES NAMES cjson
   HINTS
   ${PC_CJSON_LIBDIR}
   ${PC_CJSON_LIBRARY_DIRS}
   )

if (PC_CJSON_VERSION)
    set(CJSON_VERSION_STRING ${PC_CJSON_VERSION})
elseif (CJSON_INCLUDE_DIR AND EXISTS "${CJSON_INCLUDE_DIR}/cJSON.h")
    file(STRINGS "${CJSON_INCLUDE_DIR}/cJSON.h" version_major
         REGEX "^#define[\t ]+CJSON_VERSION_MAJOR[\t ]+.*")
    string(REGEX REPLACE "^#define[\t ]+CJSON_VERSION_MAJOR[\t ]+([^\"]*).*" "\\1"
           CJSON_VERSION_MAJOR "${version_major}")
    unset(version_major)
    file(STRINGS "${CJSON_INCLUDE_DIR}/cJSON.h" version_minor
         REGEX "^#define[\t ]+CJSON_VERSION_MINOR[\t ]+.*")
    string(REGEX REPLACE "^#define[\t ]+CJSON_VERSION_MINOR[\t ]+([^\"]*).*" "\\1"
           CJSON_VERSION_MINOR "${version_minor}")
    unset(version_minor)
    file(STRINGS "${CJSON_INCLUDE_DIR}/cJSON.h" version_patch
         REGEX "^#define[\t ]+CJSON_VERSION_PATCH[\t ]+.*")
    string(REGEX REPLACE "^#define[\t ]+CJSON_VERSION_PATCH[\t ]+([^\"]*).*" "\\1"
           CJSON_VERSION_PATCH "${version_patch}")
    unset(version_patch)

    set(CJSON_VERSION_STRING "${CJSON_VERSION_MAJOR}.${CJSON_VERSION_MINOR}.${CJSON_VERSION_PATCH}")
    unset(CJSON_VERSION_MAJOR)
    unset(CJSON_VERSION_MINOR)
    unset(CJSON_VERSION_PATCH)
endif()

# handle the QUIETLY and REQUIRED arguments and set CJSON_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(cJSON
                                  REQUIRED_VARS CJSON_LIBRARIES CJSON_INCLUDE_DIR
                                  VERSION_VAR CJSON_VERSION_STRING)

mark_as_advanced(CJSON_INCLUDE_DIR CJSON_LIBRARIES)
