project(myquake)
cmake_minimum_required(VERSION 3.0)

# for VS code cmake tools
include(CMakeToolsHelpers OPTIONAL)

# need c++ 14
set_property(GLOBAL PROPERTY CXX_STANDARD 14)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED ON)

# dependencies
find_package(SDL2 REQUIRED)

# global include path
include_directories(
  src
  src/ren_soft
  ${SDL2_INCLUDE_DIR})

# software renderer
aux_source_directory(src/ren_soft REN_SOFT_SRC_LIST)
add_library(ren_soft STATIC ${REN_SOFT_SRC_LIST})

# main executable
aux_source_directory(src SRC_LIST)
add_executable(${PROJECT_NAME} WIN32 ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
  ren_soft
  ${SDL2_LIBRARIES})
