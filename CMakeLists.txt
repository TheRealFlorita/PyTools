cmake_minimum_required(VERSION 3.10)

project(PyTools LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 COMPONENTS Core Gui Network Widgets REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Network Widgets REQUIRED)

set(QWindowKit_DIR "${CMAKE_CURRENT_SOURCE_DIR}/QWindowKit/Lib/cmake/QWindowKit/")
find_package(QWindowKit COMPONENTS Core Widgets REQUIRED)

set(INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install/")

file(GLOB SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "Source/*.cpp" "PugiXml/Source/*.cpp" "Source/XmlClasses/*.cpp" )
file(GLOB HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.h" "Include/*.h" "Include/HiDpiClasses/*.h" "Include/Frameless/*.h" "PugiXml/Include/*.hpp" "Include/XmlClasses/*.h")

include_directories(
    "${Qt6Core_PRIVATE_INCLUDE_DIRS}"
    "${Qt6Gui_PRIVATE_INCLUDE_DIRS}"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/Frameless"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/HiDpiClasses"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/XmlClasses"
    "${CMAKE_CURRENT_SOURCE_DIR}/PugiXml/Include")

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/icon.rc")
qt_add_resources(SOURCES  "${CMAKE_SOURCE_DIR}/resources.qrc")

qt_add_executable(PyTools WIN32 MANUAL_FINALIZATION "${HEADERS}" "${SOURCES}" "${APP_ICON_RESOURCE_WINDOWS}")

add_definitions(-DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS -DUNICODE -D_UNICODE)

target_link_libraries(PyTools
    PRIVATE Qt6::Core
    PRIVATE Qt6::Gui
    PRIVATE Qt6::Network
    PRIVATE Qt6::Widgets
    PUBLIC QWindowKit::Widgets)

if(MSVC)
    set_target_properties(PyTools PROPERTIES COMPILE_FLAGS "/O2")
    set(CMAKE_C_FLAGS "/O2")
    set(CMAKE_CXX_FLAGS "/O2")
    install(TARGETS PyTools DESTINATION ${INSTALL_PREFIX})
elseif(MINGW)
    set_target_properties(PyTools PROPERTIES COMPILE_FLAGS "-O3")
    set(CMAKE_C_FLAGS "-O3")
    set(CMAKE_CXX_FLAGS "-O3")
    install(TARGETS PyTools DESTINATION ${INSTALL_PREFIX})
endif()

qt_finalize_executable(PyTools)
