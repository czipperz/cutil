cmake_minimum_required(VERSION 2.6)
project(CUTIL)

# allow loading of packages based on scripts in `cmake` directory
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                      ${CUTIL_SOURCE_DIR}/cmake)
find_package(GLib REQUIRED)

add_library(cutil STATIC ${CUTIL_SOURCE_DIR}/src/str.c
                         ${CUTIL_SOURCE_DIR}/src/vec.c
                         ${CUTIL_SOURCE_DIR}/src/rpmalloc.c)
target_link_libraries(cutil ${GLib_LIBRARY})
install(TARGETS cutil DESTINATION lib)

include_directories(${CUTIL_SOURCE_DIR})
include_directories(${GLib_INCLUDE_DIRS})
