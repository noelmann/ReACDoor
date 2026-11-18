
cmake_minimum_required(VERSION 3.16)

project(ReACDoor LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static GCC runtime
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

include(GNUInstallDirs)
include(ExternalProject)

# --------------------------------------------------
# libjpeg-turbo
# --------------------------------------------------

set(JPEG_INSTALL_DIR "${CMAKE_BINARY_DIR}/libjpeg-turbo-install")

ExternalProject_Add(libjpeg_turbo_external
    GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git
    GIT_TAG 3.2.1
    GIT_SHALLOW TRUE

    INSTALL_DIR "${JPEG_INSTALL_DIR}"

    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${JPEG_INSTALL_DIR}
        -DCMAKE_INSTALL_LIBDIR=lib
        -DENABLE_SHARED=OFF
        -DENABLE_STATIC=ON
        -DWITH_SIMD=ON
        -DWITH_TURBOJPEG=OFF
        -DWITH_TOOLS=OFF
        -DWITH_TESTS=OFF
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}

    BUILD_BYPRODUCTS
        "${JPEG_INSTALL_DIR}/lib/libjpeg.a"
)

# Static JPEG library
add_library(jpeg-static STATIC IMPORTED GLOBAL)

set_target_properties(jpeg-static PROPERTIES
    IMPORTED_LOCATION
        "${JPEG_INSTALL_DIR}/lib/libjpeg.a"
)

# --------------------------------------------------
# Application
# --------------------------------------------------

add_executable(ReACDoor WIN32
    main.cpp

    base64.h
    base64.cpp

    HTTPRequest.hpp

    datatransmission.h
    datatransmission.cpp

    cipher.h
    cipher.cpp
)

# JPEG headers
target_include_directories(ReACDoor PRIVATE
    "${JPEG_INSTALL_DIR}/include"
)

# Ensure JPEG is built before application
add_dependencies(ReACDoor libjpeg_turbo_external)

# Link libraries
target_link_libraries(ReACDoor PRIVATE
    jpeg-static
    ws2_32
)

# --------------------------------------------------
# Installation
# --------------------------------------------------

install(TARGETS ReACDoor
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
