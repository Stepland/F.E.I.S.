cmake_minimum_required(VERSION 3.7)
project(FEIS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(OpenGL_GL_PREFERENCE GLVND)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3")

if (WIN32)
    message("Compiling for windows !")
    SET(GCC_COVERAGE_COMPILE_FLAGS "${GCC_COVERAGE_COMPILE_FLAGS} -mwindows")
    SET(GCC_COVERAGE_LINK_FLAGS    "${GCC_COVERAGE_LINK_FLAGS} -mwindows")
endif(WIN32)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

set(imgui include/imgui/imgui.cpp
          include/imgui/imgui_draw.cpp
          include/imgui/imgui_widgets.cpp
          include/imgui/imgui-SFML.cpp
          include/imgui/imgui_stdlib.cpp
          include/imgui/imgui_demo.cpp)

set(SOURCE_FILES
        main.cpp
        Widgets.cpp
        Marker.cpp
        Fumen.cpp
        Note.cpp
        Chart.cpp
        EditorState.cpp
        ${imgui}
        include/tinyfiledialogs.c
        Toolbox.cpp
        Toolbox.h
        LNMarker.cpp
        LNMarker.h
        History.h
        HistoryActions.cpp
        HistoryActions.h
        NotificationsQueue.cpp
        NotificationsQueue.h
        Notification.cpp
        Notification.h
        SoundEffect.cpp
        SoundEffect.h
        TimeSelection.h
        NotesClipboard.cpp
        NotesClipboard.h ChartWithHistory.cpp ChartWithHistory.h Preferences.cpp Preferences.h EditorStateActions.cpp EditorStateActions.h)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${FEIS_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})

find_library (WinMM libwinmm.a)
find_package (SFML 2.5.1 COMPONENTS system window graphics network audio REQUIRED)
find_package (OpenGL REQUIRED)

include_directories(${OPENGL_INCLUDE_DIR} ${SFML_INCLUDE_DIR} include/ include/imgui/)

add_executable(FEIS ${SOURCE_FILES})

set(TARGET_LINK_LIBS ${OPENGL_LIBRARIES} stdc++ stdc++fs sfml-system sfml-window sfml-graphics sfml-network sfml-audio)

if(WIN32)
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} WinMM)
endif(WIN32)

target_link_libraries(FEIS ${TARGET_LINK_LIBS})

