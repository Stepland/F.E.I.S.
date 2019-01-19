cmake_minimum_required(VERSION 3.7)
project(FEIS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3")

SET(GCC_COVERAGE_COMPILE_FLAGS "-mwindows")
SET(GCC_COVERAGE_LINK_FLAGS    "-mwindows")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS} -static-libgcc -static-libstdc++")

set(SOURCE_FILES
        main.cpp
        Widgets.cpp
        Marker.cpp
        Fumen.cpp
        Note.cpp
        Chart.cpp
        EditorState.cpp
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui-SFML.cpp
        imgui/imgui_stdlib.cpp
        imgui/imgui_demo.cpp
        tinyfiledialogs.c Toolbox.cpp Toolbox.h)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${FEIS_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})

find_library (WinMM libwinmm.a)
find_package (SFML 2.5.1 COMPONENTS system window graphics network audio REQUIRED)
find_package (OpenGL REQUIRED)

include_directories(${OPENGL_INCLUDE_DIR} imgui/)

add_executable(FEIS ${SOURCE_FILES})

target_link_libraries(FEIS stdc++fs ${OPENGL_LIBRARIES} sfml-system sfml-window sfml-graphics sfml-network sfml-audio WinMM)


