#############################################
##  BUILD FILE FOR PHONOMETRICA'S RUNTIME  ##
#############################################

cmake_minimum_required(VERSION 3.16)
project(phon-runtime)
set(CMAKE_CXX_STANDARD 17)


###########################
##  VERSION INFORMATION  ##
###########################

# Major release: incremented when major changes are introduced
add_definitions(-DPHON_VERSION_MAJOR=0)
# Minor release: incremented when new features and/or bug fixes are introduced
add_definitions(-DPHON_VERSION_MINOR=0)
# Revision release: incremented when bugs are fixed are small features are added
add_definitions(-DPHON_VERSION_MICRO=1)
# Build: this is only used internally for development (set to 0 on public release)
add_definitions(-DPHON_VERSION_NANO=1)

# Release date
add_definitions(-DPHON_RELEASE_DATE_YEAR=2020)
add_definitions(-DPHON_RELEASE_DATE_MONTH=5)
add_definitions(-DPHON_RELEASE_DATE_DAY=22)

# Operating system
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-DPHON_WINDOWS=1 -DPHON_MACOS=0 -DPHON_LINUX=0 -DPHON_UNIX=0)
    if (MINGW)
        add_definitions(-DPHON_POSIX=1 -DPHON_MINGW=1)
    else()
        add_definitions(-DPHON_POSIX=0 -DPHON_MINGW=0)
    endif()
else()
    add_definitions(-DPHON_POSIX=1 -DPHON_UNIX=1 -DPHON_WINDOWS=0)

    if(APPLE)
        add_definitions(-DPHON_MACOS=1 -DPHON_LINUX=0)
    else()
        # Assume linux
        add_definitions(-DPHON_MACOS=0 -DPHON_LINUX=1)
    endif()
endif()

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()


#####################
##  BUILD RUNTIME  ##
#####################

# Uncomment this line to use std::unordered_map instead of Phonometrica's default hashmap.
#add_definitions(-DPHON_STD_UNORDERED_MAP)

include_directories("..")

if(WIN32)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DUTF8PROC_STATIC)

    if(MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)
    endif()
endif()

add_definitions(-DEIGEN_MPL2_ONLY)

file(GLOB SOURCE_FILES
        *.cpp *.hpp
        runtime/*.cpp runtime/*.hpp
        runtime/compiler/*.cpp runtime/compiler/*.hpp
        utils/*.cpp utils/*.hpp
        third_party/utf8proc/utf8proc.c
        third_party/utf8proc/utf8proc.h
)

add_library(phon-runtime STATIC ${SOURCE_FILES})
set_property(TARGET phon-runtime PROPERTY POSITION_INDEPENDENT_CODE ON)

# Use UTF-8 in PCRE2.
add_definitions(-DPCRE2_CODE_UNIT_WIDTH=8 -DPCRE2_STATIC=1)
add_subdirectory(third_party/pcre2)
include_directories(${PROJECT_BINARY_DIR}/third_party/pcre2)
link_directories(${PROJECT_BINARY_DIR}/third_party/pcre2)

if (WIN32)
    target_link_libraries(phon-runtime shlwapi pcre2-8)
else()
    target_link_libraries(phon-runtime m pcre2-8)
endif(WIN32)
