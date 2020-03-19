cmake_minimum_required(VERSION 2.8)

project( videodiff ) # PROJECT NAME

# set output dir of "make" (binary executable)
# set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build)

# set output dir of library files (.so??)
# set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

add_definitions(-std=c++14)

find_package( OpenCV REQUIRED )
FIND_PACKAGE( Boost COMPONENTS system REQUIRED )

include_directories("lib" "lib/VQMT" "src")

# set( HEADERS src/main.hpp )
set( SOURCES src/main.cpp )


# set as static build, so the client doesn't need to have any lib installed
SET(BUILD_SHARED_LIBRARIES OFF)

add_executable( videodiff ${SOURCES} ${HEADERS} )
target_link_libraries(
  videodiff
  ${OpenCV_LIBS}
  ${Boost_SYSTEM_LIBRARY}
  stdc++fs
  )

add_custom_target(framesdiff_)
add_executable(framesdiff src/main_dir.cpp)
target_link_libraries(
  framesdiff
  ${OpenCV_LIBS}
  ${Boost_SYSTEM_LIBRARY}
  stdc++fs
  )
# add_dependencies(framesdiff freamesdiff_exec)
