project(myquake)
cmake_minimum_required(VERSION 3.0)

# for VS code cmake tools
include(CMakeToolsHelpers OPTIONAL)

# need c++ 14
set(CMAKE_CXX_STANDARD 14)

# dependencies
find_package(SDL2 REQUIRED)
find_library(SDL2_MIXER_LIB SDL2_mixer)
string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)

# global include path
include_directories(
  src
  src/ren_soft
  /usr/local/include)

# software renderer
aux_source_directory(src/ren_soft REN_SOFT_SRC_LIST)
add_library(ren_soft STATIC ${REN_SOFT_SRC_LIST})

# gl renderer
aux_source_directory(src/ren_gl REN_GL_SRC_LIST)
add_library(ren_gl STATIC ${REN_GL_SRC_LIST})

# main executable
aux_source_directory(src SRC_LIST)
add_executable(${PROJECT_NAME} WIN32 ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
  ren_soft
  ${SDL2_LIBRARIES}
  ${SDL2_MIXER_LIB})
