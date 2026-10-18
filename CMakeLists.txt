cmake_minimum_required(VERSION 3.16)

project(ReACDoor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ReACDoor main.cpp
    lodepng.h
    lodepng.cpp
    base64.h
    base64.cpp
    HTTPRequest.hpp)

include(GNUInstallDirs)
include(FetchContent)
target_link_libraries(ReACDoor PRIVATE
    ws2_32)

install(TARGETS ReACDoor
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
