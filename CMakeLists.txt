cmake_minimum_required(VERSION 3.1.0)

project(neoterminal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets Network REQUIRED)

add_executable(main
    ./src/term_controller.cc
    ./src/term_ui_state.cc
    ./src/term_ui_widget.cc
    ./src/msgpack_rpc.cc
    ./src/keycodes.cc
    ./src/main.cc
    )

target_link_libraries(main Qt5::Widgets Qt5::Network)
