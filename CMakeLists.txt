cmake_minimum_required(VERSION 2.8.11)

project(Evernus)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(PROJECT_NAME "evernus")

find_package(Qt5Widgets REQUIRED)

add_executable(
    ${PROJECT_NAME}
    WIN32
    main.cpp
)

if(WIN32)
    # cmake bug
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${LINK_FLAGS} /machine:x64")
endif()

target_link_libraries(
    ${PROJECT_NAME}
    Qt5::Widgets
)
