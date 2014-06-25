cmake_minimum_required(VERSION 2.8.11)

project(Evernus)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(PROJECT_NAME "evernus")

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5XmlPatterns REQUIRED)
find_package(Boost REQUIRED)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++1y -stdlib=libc++ -g")
elseif(WIN32)
    set(ADDITIONAL_SRC "evernus.rc")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
endif()

qt5_wrap_cpp(
    MOC_FILES
    ActiveTasksDialog.h
    APIInterface.h
    APIManager.h
    CharacterManagerDialog.h
    CharacterModel.h
    CharacterImportPreferencesWidget.h
    EvernusApplication.h
    MainWindow.h
    NetworkPreferencesWidget.h
    PreferencesDialog.h
)

qt5_add_resources(
    RCC_FILES
    evernus.qrc
)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(
    ${PROJECT_NAME}
    WIN32
    ActiveTasksDialog.cpp
    ActiveTasksDialog.h
    APIDomParser.h
    APIInterface.cpp
    APIInterface.h
    APIManager.cpp
    APIManager.h
    APIResponseCache.cpp
    APIResponseCache.h
    APIXmlReceiver.h
    CachedCharacter.cpp
    CachedCharacter.h
    CachedCharacterList.cpp
    CachedCharacterList.h
    CachedCharacterListRepository.cpp
    CachedCharacterListRepository.h
    CachedCharacterRepository.cpp
    CachedCharacterRepository.h
    CachedEntity.cpp
    CachedEntity.h
    CacheRepository.h
    Character.cpp
    Character.h
    CharacterData.h
    CharacterDomParser.h
    CharacterImportPreferencesWidget.cpp
    CharacterImportPreferencesWidget.h
    CharacterListXmlReceiver.h
    CharacterManagerDialog.cpp
    CharacterManagerDialog.h
    CharacterModel.cpp
    CharacterModel.h
    CharacterRepository.cpp
    CharacterRepository.h
    DatabaseUtils.cpp
    DatabaseUtils.h
    Entity.h
    EvernusApplication.cpp
    EvernusApplication.h
    ImportSettings.h
    Key.cpp
    Key.h
    KeyEditDialog.cpp
    KeyEditDialog.h
    KeyRepository.cpp
    KeyRepository.h
    main.cpp
    MainWindow.cpp
    MainWindow.h
    NetworkPreferencesWidget.cpp
    NetworkPreferencesWidget.h
    NetworkSettings.h
    PreferencesDialog.cpp
    PreferencesDialog.h
    Repository.h
    SimpleCrypt.cpp
    SimpleCrypt.h
    UISettings.h
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
    ${Boost_LIBRARIES}
    Qt5::Widgets
    Qt5::Sql
    Qt5::Network
    Qt5::Xml
    Qt5::XmlPatterns
)
