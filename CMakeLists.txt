cmake_minimum_required(VERSION 2.8.11)

project(Evernus)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(PROJECT_NAME "evernus")

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Sql REQUIRED)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -g")
endif()

if(WIN32)
    set(ADDITIONAL_SRC "evernus.rc")
endif()

qt5_wrap_cpp(
    MOC_FILES
    CharacterManagerDialog.h
    MainWindow.h
)

qt5_add_resources(
    RCC_FILES
    evernus.qrc
)

add_executable(
    ${PROJECT_NAME}
    WIN32
    Character.cpp
    Character.h
    CharacterManagerDialog.cpp
    CharacterManagerDialog.h
    CharacterRepository.cpp
    CharacterRepository.h
    Entity.h
    EvernusApplication.cpp
    EvernusApplication.h
    Key.cpp
    Key.h
    KeyEditDialog.cpp
    KeyEditDialog.h
    KeyRepository.cpp
    KeyRepository.h
    main.cpp
    MainWindow.cpp
    MainWindow.h
    Repository.h
    ${MOC_FILES}
    ${RCC_FILES}
    ${ADDITIONAL_SRC}
)

if(WIN32)
    # cmake bug
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${LINK_FLAGS} /machine:x64")
endif()

target_link_libraries(
    ${PROJECT_NAME}
    Qt5::Widgets
    Qt5::Sql
)
