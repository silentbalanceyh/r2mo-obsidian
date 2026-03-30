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

find_package(Qt6 REQUIRED COMPONENTS Core Widgets LinguistTools)

# Source files - Modular structure
set(SOURCES
    # Main
    src/main.cpp
    
    # UI
    src/mainwindow.cpp
    src/mainwindow.h
    
    # Theme
    src/theme/thememanager.cpp
    src/theme/thememanager.h
    
    # i18n
    src/i18n/translationmanager.cpp
    src/i18n/translationmanager.h
    
    # Models
    src/models/vaultmodel.cpp
    src/models/vaultmodel.h
    
    # Utils
    src/utils/settingsmanager.cpp
    src/utils/settingsmanager.h
    src/utils/vaultvalidator.cpp
    src/utils/vaultvalidator.h
    src/utils/obsidianconfigreader.cpp
    src/utils/obsidianconfigreader.h
    src/utils/r2moscanner.cpp
    src/utils/r2moscanner.h
)

# Font resources
set(FONT_RESOURCES
    src/fonts/fonts.qrc
)

# Translation files
set(TS_FILES
    translations/obsidianmanager_zh_CN.ts
    translations/obsidianmanager_en_US.ts
)

# Generate .qm files from .ts files
qt_add_translations(ObsidianManager TS_FILES ${TS_FILES})

qt_add_executable(ObsidianManager ${SOURCES} ${FONT_RESOURCES})

target_link_libraries(ObsidianManager PRIVATE
    Qt6::Core
    Qt6::Widgets
)

# Include directories
target_include_directories(ObsidianManager PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/theme
    ${CMAKE_CURRENT_SOURCE_DIR}/src/i18n
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
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