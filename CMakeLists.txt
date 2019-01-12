cmake_minimum_required(VERSION 3.7)
project(FEIS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(GCC_COVERAGE_COMPILE_FLAGS "-mwindows")
SET(GCC_COVERAGE_LINK_FLAGS    "-mwindows")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )

set(SOURCE_FILES
        main.cpp
        screen.cpp
        screen.h
        Marker.cpp
        Marker.h
        Fumen.cpp
        Fumen.h
        Note.cpp
        Note.h
        Chart.cpp
        Chart.h
        EditorState.cpp
        EditorState.h
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui-SFML.cpp
        imgui/imgui_stdlib.cpp
        tinyfiledialogs.c
        tinyfiledialogs.h)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${FEIS_SOURCE_DIR}/cmake")

find_package(SFML 2 COMPONENTS system window graphics network audio REQUIRED)
find_package (OpenGL REQUIRED)

include_directories(${OPENGL_INCLUDE_DIR} ${SFML_INCLUDE_DIR} imgui/)

add_executable(FEIS ${SOURCE_FILES})

target_link_libraries(FEIS stdc++fs ${OPENGL_LIBRARIES} sfml-system sfml-window sfml-graphics sfml-network sfml-audio)