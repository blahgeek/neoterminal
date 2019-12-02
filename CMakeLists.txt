cmake_minimum_required(VERSION 3.1.0)

project(neoterminal)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets Network REQUIRED)

add_executable(main
    ./src/term_widget.cc
    ./src/msgpack_rpc.cc
    ./src/main.cc)

target_link_libraries(main Qt5::Widgets Qt5::Network)
