cmake_minimum_required(VERSION 3.28)
project(server)

set(CMAKE_CXX_STANDARD 17)

add_executable(server main.cpp)

target_link_libraries(server PRIVATE Ws2_32 Mswsock Advapi32)