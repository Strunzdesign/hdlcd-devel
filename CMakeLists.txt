cmake_minimum_required(VERSION 2.6)

project(hdlcd-devel)

# Automatically set the version number
set(HDLCD_DEVEL_VERSION_MAJOR  \"1\")
set(HDLCD_DEVEL_VERSION_MINOR  \"0pre\")
include_directories("${PROJECT_BINARY_DIR}/libs/framing/src")
configure_file (
    "${PROJECT_SOURCE_DIR}/src/HdlcdConfig.h.in"
    "${PROJECT_SOURCE_DIR}/src/HdlcdConfig.h"
)

add_subdirectory(src)
