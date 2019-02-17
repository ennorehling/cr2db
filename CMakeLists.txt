cmake_minimum_required(VERSION 3.6)
project (cr2db C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory (crpat)
add_subdirectory (cJSON)

# find_package(SQLite3 REQUIRED)

add_executable (cr2db cr2db.c crdb.c)
target_include_directories (cr2db PRIVATE
${CJSON_INCLUDE_DIR}
${CRPAT_INCLUDE_DIR}
)
target_link_libraries(cr2db
${CJSON_LIBRARIES}
${CRPAT_LIBRARIES}
)

if(SQLITE3_FOUND)
target_include_directories (cr2db PRIVATE ${SQLITE3_INCLUDE_DIR})
target_link_libraries(cr2db ${SQLITE3_LIBRARIES})
endif(SQLITE3_FOUND)

if(MSVC)
set_target_properties (cr2db PROPERTIES COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
set_target_properties (cr2db PROPERTIES COMPILE_FLAGS /W3 /WX /MP)
endif(MSVC)
