cmake_minimum_required(VERSION 3.5)

project(IOMessage)

set(PLATFORM CACHE STRING "")

if(NOT PLATFORM OR NOT PLATFORM STREQUAL "WIN" OR NOT PLATFORM STREQUAL "LINUX")
    message(FATAL_ERROR "platform specification required (must be WIN or LINUX)")
endif()

add_subdirectory(client)
add_subdirectory(server)