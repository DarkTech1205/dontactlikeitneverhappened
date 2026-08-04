cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)

project(FontGlitch14 VERSION 1.0.0)

file(GLOB SOURCES src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Please define GEODE_SDK environment variable to point to your Geode SDK checkout/install")
endif()

include($ENV{GEODE_SDK}/loader/include/link.cmake)

# Pulls in cocos2d-x + GD bindings
setup_geode_mod(${PROJECT_NAME})
