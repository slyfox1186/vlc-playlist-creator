cmake_minimum_required(VERSION 3.10)
project(VLCPlaylistCreator VERSION 1.0.0 DESCRIPTION "VLC Playlist Creator")

# C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Installation prefix
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix" FORCE)
endif()

# Options
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(BUILD_TESTING "Build the testing tree" OFF)
option(WITH_COVERAGE "Enable coverage reporting" OFF)
option(WITH_ASAN "Enable AddressSanitizer" OFF)
option(WITH_TSAN "Enable ThreadSanitizer" OFF)

# Find Qt package
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# Add executable
add_executable(vlc-playlist-creator
    main.cpp
    videoprocessor.cpp
    vlcplaylistcreator.cpp
)

# Link libraries
target_link_libraries(vlc-playlist-creator PRIVATE Qt5::Widgets)

# Include directories
target_include_directories(vlc-playlist-creator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Compiler warnings
if(MSVC)
    target_compile_options(vlc-playlist-creator PRIVATE /W4)
else()
    target_compile_options(vlc-playlist-creator PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Coverage
if(WITH_COVERAGE)
    target_compile_options(vlc-playlist-creator PRIVATE --coverage)
    target_link_libraries(vlc-playlist-creator PRIVATE --coverage)
endif()

# Sanitizers
if(WITH_ASAN)
    target_compile_options(vlc-playlist-creator PRIVATE -fsanitize=address)
    target_link_libraries(vlc-playlist-creator PRIVATE -fsanitize=address)
elseif(WITH_TSAN)
    target_compile_options(vlc-playlist-creator PRIVATE -fsanitize=thread)
    target_link_libraries(vlc-playlist-creator PRIVATE -fsanitize=thread)
endif()

# Install target
include(GNUInstallDirs)
install(TARGETS vlc-playlist-creator
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Testing
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Generate config file
configure_file(config.h.in config.h)
