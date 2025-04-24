cmake_minimum_required(VERSION 3.18)

# Project description
project(
    gsetui VERSION 0.0.1
    DESCRIPTION "Tiny formatter commander"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
# set(ftxui_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../FTXUI/include)
# set(ftxui_BINARY_DIR ${CMAKE_CURENT_SOURCE_DIR}/../FTXUI/bin)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/log)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
include(FetchContent)

FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG v5.0.0
    GIT_PROGRESS TRUE
    GIT_SHALLOW FALSE
)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_executable(foxsicmd ${CMAKE_CURRENT_SOURCE_DIR}/app/main.cpp)

add_library(foxsicmd-lib
    ${CMAKE_CURRENT_SOURCE_DIR}/src/line.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/line.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/uart.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/uart.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/json.hpp
)

# add ftxui
target_link_libraries(foxsicmd-lib
    PUBLIC ftxui::screen
    PUBLIC ftxui::dom
    PUBLIC ftxui::component
)

# Boost library
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.70 REQUIRED COMPONENTS filesystem program_options)
if(Boost_FOUND)
    message(STATUS "Boost version: ${Boost_VERSION}")
    
    # Check if Boost version newer than CMake, or if CMake is too old:
    if(NOT TARGET Boost::filesystem)
        add_library(Boost::filesystem IMPORTED INTERFACE)
        set_property(TARGET Boost::filesystem PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})
        set_property(TARGET Boost::filesystem PROPERTY
            INTERFACE_LINK_LIBRARIES ${Boost_LIBRARIES})
    endif()

    # link the user libraries to Boost
    target_link_libraries(foxsicmd-lib PUBLIC Boost::filesystem Boost::program_options)
   
    # then link them all to the executables
    target_link_libraries(foxsicmd PUBLIC Boost::filesystem foxsicmd-lib)
elseif(NOT Boost_FOUND)
    error("Boost not found.")
endif()

enable_testing()
# add_test(NAME hkp_test COMMAND $<TARGET_FILE:hkp_test>)