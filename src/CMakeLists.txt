project(myquake)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_EXE_LINKER_FLAGS "-static")

include_directories(. ./ren_soft)

aux_source_directory(./ren_soft REN_SOFT_SRC_LIST)
add_library(ren_soft STATIC ${REN_SOFT_SRC_LIST})

aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} WIN32 ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} ren_soft)
