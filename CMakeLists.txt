cmake_minimum_required(VERSION 3.18)

project(issue_118)
set(CMAKE_CXX_STANDARD 17)
set(SOURCE_FILES 
    src/main.cpp
    ice/src/ice_library_manager.cpp
    ice/src/ice_library.cpp
    )

include_directories(
    include
    ice/include/ice
    )

add_subdirectory(ice)


add_executable(issue_118 ${SOURCE_FILES})