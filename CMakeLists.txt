cmake_minimum_required(VERSION 3.6)
project (cr2db C)
add_subdirectory (crpat)

find_package(PkgConfig REQUIRED)
pkg_search_module(SQLITE3 sqlite3 REQUIRED)

add_executable (cr2db cr2db.c parser.c)
if(SQLITE3_FOUND)
target_include_directories (cr2db PRIVATE ${SQLITE3_INCLUDE_DIR})
target_link_libraries(cr2db ${SQLITE3_LIBRARIES} crpat)
endif(SQLITE3_FOUND)

