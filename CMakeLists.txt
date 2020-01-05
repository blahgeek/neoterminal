cmake_minimum_required(VERSION 3.1.0)

project(neoterminal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

set(QT_COMPONENTS Widgets Network)
foreach(I ${QT_COMPONENTS})
    set(QT_LIBRARIES ${QT_LIBRARIES} Qt5::${I})
endforeach()

find_package(Qt5 COMPONENTS ${QT_COMPONENTS} REQUIRED)

add_library(neoterminal-objs OBJECT
    ./src/nvim_controller.cc
    ./src/nvim_ui_state.cc
    ./src/nvim_ui_widget.cc
    ./src/msgpack_rpc.cc
    ./src/keycodes.cc
    ./src/application.cc)

target_link_libraries(neoterminal-objs ${QT_LIBRARIES})

if (APPLE)
    set(MACOSX_BUNDLE_ICON_FILE term.icns)
    add_executable(NeoTerminal MACOSX_BUNDLE
        ./src/main.cc
        $<TARGET_OBJECTS:neoterminal-objs>
        ./res/term.icns
        )
    set_source_files_properties(./res/term.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_link_libraries(NeoTerminal "-framework Cocoa" ${QT_LIBRARIES})
else ()
    add_executable(neoterminal
        ./src/main.cc
        $<TARGET_OBJECTS:neoterminal-objs>
        )
    target_link_libraries(neoterminal ${QT_LIBRARIES})
    install(TARGETS neoterminal RUNTIME DESTINATION bin)
endif ()
