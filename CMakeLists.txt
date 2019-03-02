cmake_minimum_required(VERSION 3.6)
project (crservice C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory (crpat)
add_subdirectory (cJSON)

find_package(Fcgi)
find_package(SQLite3)

if (FCGI_FOUND)
add_executable (hellocgi hellocgi.c)
target_link_libraries (hellocgi ${FCGI_LIBRARIES})
target_include_directories (hellocgi PRIVATE ${FCGI_INCLUDE_DIR})
set_target_properties (hellocgi PROPERTIES COMPILE_FLAGS "-Wall -Wextra")

endif (FCGI_FOUND)

add_executable (crservice crdata.c crfile.c log.c)

add_executable (cr2json cr2json.c crfile.c log.c)
target_include_directories (cr2json PRIVATE
${CJSON_INCLUDE_DIR}
${CRPAT_INCLUDE_DIR}
)
target_link_libraries(cr2json
${CJSON_LIBRARIES}
${CRPAT_LIBRARIES}
)

#if(SQLITE3_FOUND)
#target_include_directories (cr2json PRIVATE ${SQLITE3_INCLUDE_DIR})
#target_link_libraries(cr2json ${SQLITE3_LIBRARIES})
#endif(SQLITE3_FOUND)

if(MSVC)
set_target_properties (cr2json PROPERTIES COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
set_target_properties (cr2json PROPERTIES COMPILE_FLAGS "/W3 /WX /MP")
endif(MSVC)

