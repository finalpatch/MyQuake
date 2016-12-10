project(myquake)
cmake_minimum_required(VERSION 3.0)

include(CMakeToolsHelpers OPTIONAL)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

find_package(SDL2 REQUIRED)

include_directories(
  src
  src/ren_soft
  ${SDL2_INCLUDE_DIR})

aux_source_directory(src/ren_soft REN_SOFT_SRC_LIST)
add_library(ren_soft STATIC ${REN_SOFT_SRC_LIST})

aux_source_directory(src SRC_LIST)
add_executable(${PROJECT_NAME} WIN32 ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
  ren_soft
  ${SDL2_LIBRARIES})
