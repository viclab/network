cmake_minimum_required(VERSION 2.8)
project(cpp_network_example)

set(CMAKE_CXX_FLAGS "-g -O2 -std=c++11 -DASIO_STANDALONE -Werror")

set(ASIO_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/asio/asio/include)

add_subdirectory(socks5-proxy)
