project(myquake)
cmake_minimum_required(VERSION 3.0)

if (MINGW)
  set(CMAKE_SH CMAKE_SH-NOTFOUND)
endif()

# for VS code cmake tools
include(CMakeToolsHelpers OPTIONAL)

# need c++ 14
set(CMAKE_CXX_STANDARD 14)

# dependencies
find_package(SDL2 REQUIRED)
find_library(SDL2_MIXER_LIB SDL2_mixer)
find_library(GLBINDING_LIB glbinding)
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
target_link_libraries(ren_gl PUBLIC ${GLBINDING_LIB})

# main executable
aux_source_directory(src SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
  ren_gl
  ${SDL2_LIBRARIES}
  ${SDL2_MIXER_LIB})

# software render executable
aux_source_directory(src SRC_LIST)
add_executable(${PROJECT_NAME}_soft ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}_soft
  ren_soft
  ${SDL2_LIBRARIES}
  ${SDL2_MIXER_LIB})
