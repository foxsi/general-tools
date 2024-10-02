cmake_minimum_required(VERSION 3.18)

# Project description
project(
    gsetui VERSION 0.1.0
    DESCRIPTION "Tiny formatterless power board manager"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
# set(ftxui_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../FTXUI/include)
# set(ftxui_BINARY_DIR ${CMAKE_CURENT_SOURCE_DIR}/../FTXUI/bin)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/log)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src)
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

add_executable(ptui ${CMAKE_CURRENT_SOURCE_DIR}/app/main.cpp)
add_executable(debug-server ${CMAKE_CURRENT_SOURCE_DIR}/app/debug_server.cpp)
add_executable(debug-client ${CMAKE_CURRENT_SOURCE_DIR}/app/debug_client.cpp)

add_library(ptui-lib
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parameters.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parameters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/listen.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/listen.cpp
)

# add ftxui
target_link_libraries(ptui-lib 
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
    target_link_libraries(ptui-lib PUBLIC Boost::filesystem Boost::program_options)
   
    # then link them all to the executables
    target_link_libraries(ptui PUBLIC Boost::filesystem ptui-lib)
    target_link_libraries(debug-server PUBLIC Boost::filesystem)
    target_link_libraries(debug-client PUBLIC Boost::filesystem ptui-lib)
elseif(NOT Boost_FOUND)
    error("Boost not found.")
endif()
