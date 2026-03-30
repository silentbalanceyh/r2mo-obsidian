cmake_minimum_required(VERSION 3.16)
project(ObsidianManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt 6.10.0 path (macOS)
if(NOT DEFINED CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "/Users/lang/Qt/6.10.0/macos")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt_add_executable(ObsidianManager
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
)

target_link_libraries(ObsidianManager PRIVATE
    Qt6::Core
    Qt6::Widgets
)

# macOS bundle settings
set_target_properties(ObsidianManager PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.r2mo.obsidianmanager"
    MACOSX_BUNDLE_BUNDLE_NAME "Obsidian Manager"
    MACOSX_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

install(TARGETS ObsidianManager BUNDLE DESTINATION .)