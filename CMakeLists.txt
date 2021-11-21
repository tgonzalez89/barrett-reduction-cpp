cmake_minimum_required(VERSION 3.19)

project(libbr VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message("Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic" CACHE STRING "Set C++ Compiler Flags" FORCE)

add_library(br
    INTERFACE
        libbr/br.hpp
)

target_include_directories(br
    INTERFACE
        ${PROJECT_SOURCE_DIR}
)

add_executable(br-test
    libbr/br-test.cpp
)

target_link_libraries(br-test
    PRIVATE
        br
)
