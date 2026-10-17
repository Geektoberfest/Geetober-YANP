cmake_minimum_required(VERSION 3.16)

# Test target: test_filemanager
add_executable(test_filemanager
    test_filemanager.cpp
    ${CMAKE_SOURCE_DIR}/src/FileManager.cpp
)
target_include_directories(test_filemanager PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_filemanager PRIVATE Qt6::Core Qt6::Test)
add_test(NAME test_filemanager COMMAND test_filemanager)

# Test target: test_search
add_executable(test_search
    test_search.cpp
    ${CMAKE_SOURCE_DIR}/src/EditorWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/SearchController.cpp
)
target_include_directories(test_search PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_search PRIVATE Qt6::Core Qt6::Widgets Qt6::Test)
add_test(NAME test_search COMMAND test_search)

# Test target: test_themes
add_executable(test_themes
    test_themes.cpp
    ${CMAKE_SOURCE_DIR}/src/themes/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/themes/ThemeRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/themes/ThemeManager.cpp
    ${CMAKE_SOURCE_DIR}/src/themes/definitions/SystemDefaultTheme.cpp
    ${CMAKE_SOURCE_DIR}/src/themes/definitions/ClassicLightTheme.cpp
    ${CMAKE_SOURCE_DIR}/src/themes/definitions/ClassicDarkTheme.cpp
    ${CMAKE_SOURCE_DIR}/src/themes/definitions/RetroGreenTheme.cpp
    ${CMAKE_SOURCE_DIR}/src/themes/definitions/AmberCRTTheme.cpp
    ${CMAKE_SOURCE_DIR}/src/themes/definitions/SolarizedDarkTheme.cpp
    ${CMAKE_SOURCE_DIR}/src/themes/definitions/EnchantedLibraryTheme.cpp
    ${CMAKE_SOURCE_DIR}/src/themes/definitions/DreamscapeIslandsTheme.cpp
)
target_include_directories(test_themes PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_themes PRIVATE Qt6::Core Qt6::Widgets Qt6::Test)
add_test(NAME test_themes COMMAND test_themes)

set_tests_properties(
    test_filemanager
    test_search
    test_themes
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
