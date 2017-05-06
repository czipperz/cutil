cmake_minimum_required(VERSION 2.6)
project(CUTIL)

add_library(cutil STATIC ${CUTIL_SOURCE_DIR}/src/str.c)
install(TARGETS cutil DESTINATION lib)

include_directories(${CUTIL_SOURCE_DIR})
