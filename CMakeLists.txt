cmake_minimum_required(VERSION 3.6)
project (cr2db C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory (crpat)

find_package(SQLite3 REQUIRED)

add_executable (cr2db cr2db.c crdb.c)
if(SQLITE3_FOUND)
target_include_directories (cr2db PRIVATE ${SQLITE3_INCLUDE_DIR})
target_link_libraries(cr2db ${SQLITE3_LIBRARIES} crpat)
endif(SQLITE3_FOUND)

