cmake_minimum_required(VERSION 2.6)
project(CUTIL)

# allow loading of packages based on scripts in `cmake` directory
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                      ${CUTIL_SOURCE_DIR}/cmake)
find_package(GLib REQUIRED)

set(files ${CUTIL_SOURCE_DIR}/src/str.c
          ${CUTIL_SOURCE_DIR}/src/vec.c
          ${CUTIL_SOURCE_DIR}/src/dll.c
          ${CUTIL_SOURCE_DIR}/src/stack_trace.c
          ${CUTIL_SOURCE_DIR}/src/log.c
          ${CUTIL_SOURCE_DIR}/src/rpmalloc.c)

add_definitions("-Wincompatible-pointer-types" "-Wall"
  "-Wextra" "-Wpedantic" "-Wno-error=unused-parameter"
  "-Wno-error=format-extra-args" "-Wno-unused-function"
  "-Wno-unused-variable")

add_library(cutil STATIC ${files})
target_link_libraries(cutil ${GLib_LIBRARY})
target_link_libraries(cutil ${CMAKE_DL_LIBS})
install(TARGETS cutil DESTINATION lib)

add_executable(test_cutil ${files} ${CUTIL_SOURCE_DIR}/src/test_main.c)
target_link_libraries(test_cutil ${GLib_LIBRARY})
target_link_libraries(test_cutil ${CMAKE_DL_LIBS})
target_compile_definitions(test_cutil PRIVATE "TEST_MODE")

set(CUTIL_INCLUDE_DIRS ${CUTIL_SOURCE_DIR} PARENT_SCOPE)

include_directories(${CUTIL_SOURCE_DIR})
include_directories(${GLib_INCLUDE_DIRS})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
