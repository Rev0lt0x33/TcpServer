cmake_minimum_required(VERSION 3.28)
project(TcpServer)
set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_CXX_STANDARD 20)
link_libraries(ws2_32 wsock32)
add_executable(TcpServer main.cpp
        include/TcpServer.h
        src/TcpServer.cpp
)
