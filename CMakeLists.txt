#######################################################
##  CMAKE FILE FOR THE STANDALONE CALAO INTERPRETER  ##
#######################################################

cmake_minimum_required(VERSION 3.12)
project(calao)
set(CMAKE_CXX_STANDARD 17)


#####################
##  CONFIGURATION  ##
#####################

set(BUILD_INTERPRETER ON)
set(BUILD_UNIT_TEST OFF)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
endif()


#########################
##  BUILD APPLICATION  ##
#########################

add_subdirectory(calao)
include_directories(${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
link_directories(${PROJECT_BINARY_DIR}/calao/)
link_directories(${PROJECT_BINARY_DIR}/calao/third_party/pcre2)

if(BUILD_INTERPRETER)
    add_executable(${PROJECT_NAME} interpreter.cpp)
    target_link_libraries(${PROJECT_NAME} calao-runtime)
endif(BUILD_INTERPRETER)

if(BUILD_UNIT_TEST)
    file(GLOB TEST_FILES ./unit_test/*.cpp)
    add_executable(test_calao ${TEST_FILES})
    target_link_libraries(test_calao calao-runtime)
endif(BUILD_UNIT_TEST)