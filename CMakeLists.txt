cmake_minimum_required(VERSION 3.0)
project(web_server)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -pthread -Wall -Wno-unused-function")
#set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

include_directories(include)

set_target_properties(${TARGET} PROPERTIES
COMPILE_FLAGS "-pthread"
LINK_FLAGS "-pthread")

set(LIB_SRC
    src/http_server.cc
    src/protocal.cc
    src/tcp_server.cc
    src/thread_pool.cc
    src/util.cc
    src/log.cc
    )

add_library(libserver SHARED ${LIB_SRC})

add_executable(http_server src/main.cc)
add_dependencies(http_server libserver)
target_link_libraries(http_server libserver)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

