cmake_minimum_required(VERSION 3.5)
project(yaqtlib CXX)

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt5 COMPONENTS Core Network DBus Multimedia Positioning Qml Gui Quick LinguistTools REQUIRED)

include(FindPkgConfig)
pkg_search_module(NEMONOTIFICATIONS nemonotifications-qt5 REQUIRED)
pkg_search_module(LIBPULSE libpulse REQUIRED)

set(CMAKE_INSTALL_MESSAGE NEVER)
set(CMAKE_AUTOMOC ON)


add_library(yaqtlib STATIC)

# tgcalls
option(USE_CALLS "Whether to enable calls support" ON)

if(USE_CALLS)
    target_include_directories(yaqtlib PUBLIC tg_owt/include)

    include(tgcalls.cmake)
    target_link_libraries(yaqtlib tgcalls)
    target_include_directories(yaqtlib PUBLIC tgcalls)

    target_compile_definitions(yaqtlib PUBLIC USE_CALLS)

    target_sources(yaqtlib PRIVATE
        src/callsmanager.cpp
        src/callaudio.cpp

        src/callsmanager.h
        src/callaudio.h
    )
endif()


target_include_directories(yaqtlib PUBLIC
    ${NEMONOTIFICATIONS_INCLUDE_DIRS}
    ${LIBPULSE_INCLUDE_DIRS}
    tdlib/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(yaqtlib
    Qt5::Quick
    Qt5::DBus
    Qt5::Multimedia
    Qt5::Positioning
    ${NEMONOTIFICATIONS_LIBRARIES}
    ${LIBPULSE_LIBRARIES}
    TDLib
)

target_sources(yaqtlib PRIVATE
    src/settings.cpp
    src/boolfiltermodel.cpp
    src/chat/jumpablemessagesmodel.cpp
    src/chatdata.cpp
    src/chatfoldersmodel.cpp
    src/chat/chatmanager.cpp
    src/chatpermissionfiltermodel.cpp
    src/chatlistmodel.cpp
    src/contactsmodel.cpp
    src/dbusadaptor.cpp
    src/folderchatlistmodel.cpp
    src/chat/forumtopicsmodel.cpp
    src/chat/mediamessagesmodel.cpp
    src/chat/messagedata.cpp
    src/chat/messagesmodel.cpp
    src/chat/readablemessagesmodel.cpp
    src/suggestedactionsmanager.cpp
    src/utilities.cpp
    src/knownusersmodel.cpp
    src/mceinterface.cpp
    src/notificationmanager.cpp
    src/stickermanager.cpp
    src/tdlib/tdlibfile.cpp
    src/tdlib/tdlibreceiver.cpp
    src/tdlib/tdlibwrapper.cpp
    src/tdlib/tdlibresponse.cpp
    src/textfiltermodel.cpp
    src/waveformmanager.cpp
    src/basemessagabledata.cpp
    src/chat/forumtopicmessagesmodel.cpp
    src/chat/forumtopic.cpp
    src/mainhelper.cpp
    src/chat/invertedmediamessagesmodel.cpp
    src/userprofilepicturesmodel.cpp
    src/chat/chatphotosmodel.cpp
    src/ngfinterface.cpp
    src/tdlib/tdlibdata.cpp
    src/usersmodel.cpp

    src/settings.h
    src/boolfiltermodel.h
    src/chat/jumpablemessagesmodel.h
    src/chatdata.h
    src/chatfoldersmodel.h
    src/chat/chatmanager.h
    src/chatpermissionfiltermodel.h
    src/chatlistmodel.h
    src/contactsmodel.h
    src/dbusadaptor.h
    src/debuglog.h
    src/debuglogjs.h
    src/folderchatlistmodel.h
    src/chat/forumtopicsmodel.h
    src/chat/mediamessagesmodel.h
    src/chat/messagedata.h
    src/chat/messagesmodel.h
    src/chat/readablemessagesmodel.h
    src/suggestedactionsmanager.h
    src/utilities.h
    src/knownusersmodel.h
    src/mceinterface.h
    src/notificationmanager.h
    src/platformapp.h
    src/stickermanager.h
    src/tdlib/tdlibfile.h
    src/tdlib/tdlibreceiver.h
    src/tdlib/tdlibwrapper.h
    src/tdlib/tdlibresponse.h
    src/textfiltermodel.h
    src/waveformmanager.h
    src/basemessagabledata.h
    src/chat/forumtopicmessagesmodel.h
    src/chat/forumtopic.h
    src/mainhelper.h
    src/chat/invertedmediamessagesmodel.h
    src/userprofilepicturesmodel.h
    src/chat/chatphotosmodel.h
    src/ngfinterface.h
    src/tdlib/tdlibdata.h
    src/usersmodel.h
)
