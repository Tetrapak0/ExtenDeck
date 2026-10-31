cmake_minimum_required(VERSION 3.15)
project(ExtenDeck)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/external)
#if (WIN32)
include_directories(${CMAKE_CURRENT_LIST_DIR}/external/freetype)
#endif()

file(GLOB_RECURSE DxSrc ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
file(GLOB ImSrc ${CMAKE_CURRENT_LIST_DIR}/external/imgui/*.cpp)
file(GLOB TetraSrc ${CMAKE_CURRENT_LIST_DIR}/external/Tetrapak0/*.c ${CMAKE_CURRENT_LIST_DIR}/external/Tetrapak0/*.cpp)

if (WIN32)
    file(GLOB NFDExSrc ${CMAKE_CURRENT_LIST_DIR}/external/NativeFileDialogs-Extended/nfd_win.cpp)
elseif (UNIX AND NOT APPLE)
    file(GLOB NFDExSrc ${CMAKE_CURRENT_LIST_DIR}/external/NativeFileDialogs-Extended/nfd_portal.cpp)
endif()

file(GLOB_RECURSE NoWarnCpp ${CMAKE_CURRENT_LIST_DIR}/external/*.cpp)

set_source_files_properties(${NoWarnCpp} PROPERTIES COMPILE_FLAGS "-w")

add_executable(ExtenDeck ${DxSrc}
                          ${ImSrc}
                          ${NFDExSrc}
                          ${TetraSrc}
                          ${CMAKE_CURRENT_LIST_DIR}/external/stb/stbi.c
                          ${CMAKE_CURRENT_LIST_DIR}/resources/icon.c
                          ${CMAKE_CURRENT_LIST_DIR}/resources/EnterIcon.c
                          ${CMAKE_CURRENT_LIST_DIR}/resources/ExtenDeck.rc)
set_property(TARGET ExtenDeck PROPERTY CXX_STANDARD 23)

# must be ran with -DCMAKE_BUILD_TYPE=Debug
if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    target_compile_definitions(ExtenDeck PRIVATE _DEBUG)
endif()

if (WIN32)
    if (MSVC)
#        file(GLOB_RECURSE ICO_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.ico")
#        target_sources(ExtenDeck PRIVATE ${ICO_FILES})
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            add_compile_options(/fsanitize=address)
            add_link_options(/fsanitize=address)
        endif()
        target_link_options(ExtenDeck PRIVATE "/SUBSYSTEM:WINDOWS")
        target_compile_options(ExtenDeck PRIVATE /MP)
        target_link_libraries(ExtenDeck ${CMAKE_CURRENT_LIST_DIR}/external/GLFW/lib/glfw3.lib
                                         ${CMAKE_CURRENT_LIST_DIR}/external/FreeType/lib/freetype.lib
                                         opengl32 dwmapi ws2_32 iphlpapi)
    else()
        target_compile_options(ExtenDeck PRIVATE -mwindows -Wl,--subsystem,windows)
#        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--subsystem,windows")
        target_link_libraries(ExtenDeck glfw3 opengl32 freetype dwmapi ws2_32 iphlpapi)
    endif()
elseif (UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS REQUIRED dbus-1)
    target_include_directories(ExtenDeck PRIVATE ${DBUS_INCLUDE_DIRS})
    target_link_libraries(ExtenDeck glfw GL freetype ${DBUS_LINK_LIBRARIES})
endif()
