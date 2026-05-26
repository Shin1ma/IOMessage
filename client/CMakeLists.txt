cmake_minimum_required(VERSION 3.5)

project(IOMessage_client LANGUAGES C)

if(PLATFORM STREQUAL "WIN")
    add_library(NetworkManager STATIC src/win/NetworkManager_win_imp.c ./NetworkManager.h)
elseif(PLATFORM STREQUAL "LINUX")
    message(FATAL_ERROR "LINUX still unsupported")
else()
    message(FATAL_ERROR "platform unsupported (WIN or LINUX)")
endif()

add_library(utils STATIC src/utils.c ./utils.h)

add_executable(IOMessage_client src/main.c)
target_link_libraries(IOMessage_client PRIVATE NetworkManager utils)
if(PLATFORM STREQUAL "WIN")
    target_link_libraries(IOMessage_client PRIVATE ws2_32)
endif()
