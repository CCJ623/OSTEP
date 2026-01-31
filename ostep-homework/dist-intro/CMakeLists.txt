# ----------------------------------------------------------------------
# CMakeLists.txt (Server & Client 混合项目)
# ----------------------------------------------------------------------
cmake_minimum_required(VERSION 3.28)

# 定义项目和语言
project(DistributedSystemIntro LANGUAGES C CXX) 

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -g")

# 指定最新的标准 (C23 和 C++23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)


add_executable(server
    server.cpp
)

add_executable(client
    client.cpp
)

add_executable(benchmark 
    benchmark.cpp
)

add_library(CommunicationModule)
target_sources(CommunicationModule
    PUBLIC
        FILE_SET CXX_MODULES FILES communication.cppm
    PRIVATE
        communication.cpp
        udp.c
)

target_link_libraries(server PRIVATE CommunicationModule)
target_link_libraries(client PRIVATE CommunicationModule)
target_link_libraries(benchmark PRIVATE CommunicationModule)

