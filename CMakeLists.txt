cmake_minimum_required(VERSION 3.8)
project (crservice C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory (crpat)

find_package(SQLite3 REQUIRED)
find_library(FCGI NAMES fcgi)

add_subdirectory (cJSON)
add_subdirectory (clibs)

set (TARGETS hellocgi crservice cr2json)

# simple example of libfcgi use
if (FCGI_FOUND)
add_executable (echocgi echocgi.c)
target_link_libraries (echocgi ${FCGI_LIBRARIES})
target_include_directories (echocgi PRIVATE ${FCGI_INCLUDE_DIR})
set (TARGETS echocgi ${TARGETS})
endif (FCGI_FOUND)

if (SQLITE3_FOUND)
  add_executable (atl-cli atlcli.c log.c crfile.c gamedb.c jsondata.c)
  target_link_libraries (atl-cli
    ${CLIBS_LIBRARIES}
    ${CRPAT_LIBRARIES}
    ${CJSON_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    )
  target_include_directories (atl-cli PRIVATE 
    ${CLIBS_INCLUDE_DIR}
    ${CRPAT_INCLUDE_DIR}
    ${CJSON_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIR}
    )
  set (TARGETS atl-cli ${TARGETS})
endif(SQLITE3_FOUND)

# cr2json cli converter
add_executable (cr2json cr2json.c crfile.c log.c)
target_include_directories (cr2json PRIVATE
${CJSON_INCLUDE_DIR}
${CRPAT_INCLUDE_DIR}
)
target_link_libraries(cr2json
${CJSON_LIBRARIES}
${CRPAT_LIBRARIES}
)

# hello world with cgi.c
add_executable (hellocgi hellocgi.c cgi.c log.c)
if (FCGI_FOUND)
target_link_libraries (hellocgi ${FCGI_LIBRARIES})
target_include_directories (hellocgi PRIVATE ${FCGI_INCLUDE_DIR})
target_compile_definitions (hellocgi PRIVATE HAVE_FCGI=1)
endif (FCGI_FOUND)

# the eressea service
add_executable (crservice crservice.c cgi.c crdata.c crfile.c log.c)
target_include_directories (crservice PRIVATE
	${CJSON_INCLUDE_DIR}
	${CRPAT_INCLUDE_DIR}
)
target_link_libraries(crservice
	${CJSON_LIBRARIES}
	${CRPAT_LIBRARIES}
)
if (FCGI_FOUND)
target_link_libraries (crservice ${FCGI_LIBRARIES})
target_include_directories (crservice PRIVATE ${FCGI_INCLUDE_DIR})
target_compile_definitions (crservice PRIVATE HAVE_FCGI=1)
endif (FCGI_FOUND)

#find_package(SQLite3)
#if(SQLITE3_FOUND)
#target_include_directories (cr2json PRIVATE ${SQLITE3_INCLUDE_DIR})
#target_link_libraries(cr2json ${SQLITE3_LIBRARIES})
#endif(SQLITE3_FOUND)

if(MSVC)
#set_target_properties (${TARGETS} PROPERTIES COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
set_target_properties (${TARGETS} PROPERTIES COMPILE_FLAGS "/W3 /WX /MP")
elseif (APPLE)
  set_target_properties (${TARGETS} PROPERTIES COMPILE_FLAGS
    "-Wall -Wextra")
else (MSVC)
  set_target_properties (${TARGETS} PROPERTIES COMPILE_FLAGS 
    "-Wall -Wextra -Wno-return-local-addr")
endif(MSVC)

