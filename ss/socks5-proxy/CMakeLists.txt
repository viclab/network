project(socks5-proxy)

add_executable(server server.cpp session.cpp encrypt.cpp config.cpp)
add_executable(local local.cpp session.cpp encrypt.cpp config.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(server PRIVATE ${ASIO_INCLUDE_DIR})
target_include_directories(local PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(server pthread)
target_link_libraries(local pthread)