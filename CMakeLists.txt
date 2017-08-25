cmake_minimum_required(VERSION 3.7)
project(FEIS)

set(CMAKE_CXX_STANDARD 11)

SET(GCC_COVERAGE_COMPILE_FLAGS "-mwindows")
SET(GCC_COVERAGE_LINK_FLAGS    "-mwindows")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )

set(SOURCE_FILES main.cpp screen.cpp screen.h Marker.cpp Marker.h Fumen.cpp Fumen.h Note.cpp Note.h Chart.cpp Chart.h)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${FEIS_SOURCE_DIR}/cmake")

set(SFML_ROOT ${FEIS_SOURCE_DIR})

find_package(SFML REQUIRED system window graphics network audio)
include_directories(${SFML_INCLUDE_DIR})

add_executable(FEIS ${SOURCE_FILES})

target_link_libraries(FEIS ${SFML_LIBRARIES})