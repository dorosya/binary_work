cmake_minimum_required(VERSION 3.16)
project(ps_lab1_qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_CONSOLE "Build console target (ps_console)" OFF)

# ---- Core library (business logic from PSConsole) ----
file(GLOB_RECURSE PS_CORE_SOURCES
    "${CMAKE_SOURCE_DIR}/PSConsole/src/core/*.cpp"
    "${CMAKE_SOURCE_DIR}/PSConsole/src/domain/*.cpp"
    "${CMAKE_SOURCE_DIR}/PSConsole/src/infra/*.cpp"
    "${CMAKE_SOURCE_DIR}/PSConsole/src/services/*.cpp"
)

add_library(ps_core STATIC ${PS_CORE_SOURCES})
target_include_directories(ps_core PUBLIC
    "${CMAKE_SOURCE_DIR}/PSConsole/src"
)

# ---- Qt GUI ----
find_package(Qt6 COMPONENTS Widgets REQUIRED)

qt_standard_project_setup()

add_executable(ps_gui
    PSQtGui/main.cpp
    PSQtGui/MainWindow.h
    PSQtGui/MainWindow.cpp
    PSQtGui/OpenDialog.h
    PSQtGui/OpenDialog.cpp
    PSQtGui/ComponentsDialog.h
    PSQtGui/ComponentsDialog.cpp
    PSQtGui/SpecificationDialog.h
    PSQtGui/SpecificationDialog.cpp
    PSQtGui/SpecItemDialog.h
    PSQtGui/SpecItemDialog.cpp
)

target_link_libraries(ps_gui PRIVATE Qt6::Widgets ps_core)

# For Linux: make sure we compile with PIC where needed when linking static libs into PIE executables
set_target_properties(ps_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---- Optional: console target (for comparison/testing) ----
if(BUILD_CONSOLE)
    add_executable(ps_console PSConsole/src/main.cpp)
    target_link_libraries(ps_console PRIVATE ps_core)
    target_include_directories(ps_console PRIVATE "${CMAKE_SOURCE_DIR}/PSConsole/src")
endif()
