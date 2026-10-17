cmake_minimum_required(VERSION 3.16)

project(YANP
    VERSION 1.0.0
    DESCRIPTION "Yet Another Notepad - Lightweight classic desktop text editor"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    PrintSupport
    Test
)

# Application Sources
set(YANP_SOURCES
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/EditorWidget.h
    src/EditorWidget.cpp
    src/FileManager.h
    src/FileManager.cpp
    src/SearchController.h
    src/SearchController.cpp
    src/PrintManager.h
    src/PrintManager.cpp
    src/AboutDialog.h
    src/AboutDialog.cpp
    src/themes/Theme.h
    src/themes/Theme.cpp
    src/themes/ThemeRegistry.h
    src/themes/ThemeRegistry.cpp
    src/themes/ThemeManager.h
    src/themes/ThemeManager.cpp
    src/themes/definitions/SystemDefaultTheme.h
    src/themes/definitions/SystemDefaultTheme.cpp
    src/themes/definitions/ClassicLightTheme.h
    src/themes/definitions/ClassicLightTheme.cpp
    src/themes/definitions/ClassicDarkTheme.h
    src/themes/definitions/ClassicDarkTheme.cpp
    src/themes/definitions/RetroGreenTheme.h
    src/themes/definitions/RetroGreenTheme.cpp
    src/themes/definitions/AmberCRTTheme.h
    src/themes/definitions/AmberCRTTheme.cpp
    src/themes/definitions/SolarizedDarkTheme.h
    src/themes/definitions/SolarizedDarkTheme.cpp
    src/themes/definitions/EnchantedLibraryTheme.h
    src/themes/definitions/EnchantedLibraryTheme.cpp
    src/themes/definitions/DreamscapeIslandsTheme.h
    src/themes/definitions/DreamscapeIslandsTheme.cpp
    src/themes/definitions/MidnightObservatoryTheme.h
    src/themes/definitions/MidnightObservatoryTheme.cpp
)

add_executable(YANP ${YANP_SOURCES})

target_include_directories(YANP PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(YANP PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
)

if(APPLE)
    set_target_properties(YANP PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "YANP"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.geektober.yanp"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()

# Installation rules
include(GNUInstallDirs)
install(TARGETS YANP
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Testing
enable_testing()
add_subdirectory(tests)
