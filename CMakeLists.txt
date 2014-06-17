cmake_minimum_required(VERSION 2.8.11)

project(Evernus)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(PROJECT_NAME "evernus")

find_package(Qt5Widgets REQUIRED)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -g")
endif()

add_executable(
    ${PROJECT_NAME}
    WIN32
    EvernusApplication.cpp
    EvernusApplication.h
    main.cpp
    MainWindow.cpp
    MainWindow.h
)

if(WIN32)
    # cmake bug
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${LINK_FLAGS} /machine:x64")
endif()

target_link_libraries(
    ${PROJECT_NAME}
    Qt5::Widgets
)
