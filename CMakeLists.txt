cmake_minimum_required(VERSION 3.16)

project(ReACDoor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ReACDoor main.cpp)

include(GNUInstallDirs)
include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG b2d95f260f1f4472e90fb69f66e362ba41c83946)
FetchContent_MakeAvailable(cpr)
target_link_libraries(ReACDoor PRIVATE cpr::cpr)

install(TARGETS ReACDoor
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
