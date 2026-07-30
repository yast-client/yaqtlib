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
pkg_search_module(ZLIB zlib REQUIRED)
pkg_search_module(LIBPULSE libpulse REQUIRED)

set(CMAKE_INSTALL_MESSAGE NEVER)
set(CMAKE_AUTOMOC ON)


# rlottie
# rlottie/CMakeLists.txt is too hard to adapt
add_library(rlottie STATIC)

set(RLOTTIE_CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/config.h)
# we could do file(TOUCH filename), but that requires CMake 3.12
if (NOT EXISTS ${RLOTTIE_CONFIG_PATH})
    file(WRITE ${RLOTTIE_CONFIG_PATH} "")
endif()

set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads)

set_target_properties(rlottie PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(rlottie PUBLIC ${CMAKE_THREAD_LIBS_INIT})

add_definitions(-DLOTTIE_THREAD_SUPPORT)
target_include_directories(rlottie PUBLIC
    rlottie/inc
    rlottie/src/vector
    rlottie/src/vector/freetype
)
target_sources(rlottie
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottieanimation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottieitem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottieitem_capi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottiekeypath.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottieloader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottiemodel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottieparser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/zip/zip.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/freetype/v_ft_math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/freetype/v_ft_raster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/freetype/v_ft_stroker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/stb/stb_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/varenaalloc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vbezier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vbitmap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vbrush.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vdasher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vdrawable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vdrawhelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vdrawhelper_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vdrawhelper_neon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vdrawhelper_sse2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vmatrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vimageloader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vinterpolator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vpainter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vpath.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vpathmesure.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vraster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vrle.cpp
)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("#ifdef __ARM_NEON__
int main() { return 0; }
#else
#error \"NEON not available\"
#endif
" HAVE_NEON)
if (HAVE_NEON)
    message("Using NEON render functions")
    SET(CMAKE_ASM_FLAGS "${CFLAGS} -x assembler-with-cpp")
    target_sources(rlottie PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/pixman/pixman-arm-neon-asm.S
    )
else()
    message("Using default render functions")
endif()


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
    ${ZLIB_INCLUDE_DIRS}
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
    ${ZLIB_LIBRARIES}
    ${LIBPULSE_LIBRARIES}
    TDLib
    rlottie
)

target_compile_definitions(yaqtlib PRIVATE QT_STATICPLUGIN)

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
    src/tgsplugin.cpp
    src/waveformmanager.cpp
    src/lottieitem.cpp
    src/basemessagabledata.cpp
    src/chat/forumtopicmessagesmodel.cpp
    src/chat/forumtopic.cpp
    src/mainhelper.cpp
    src/chat/invertedmediamessagesmodel.cpp
    src/userprofilepicturesmodel.cpp
    src/chat/chatphotosmodel.cpp
    src/ngfinterface.cpp
    src/tdlib/tdlibdata.cpp

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
    src/tgsplugin.h
    src/waveformmanager.h
    src/lottieitem.h
    src/basemessagabledata.h
    src/chat/forumtopicmessagesmodel.h
    src/chat/forumtopic.h
    src/mainhelper.h
    src/chat/invertedmediamessagesmodel.h
    src/userprofilepicturesmodel.h
    src/chat/chatphotosmodel.h
    src/ngfinterface.h
    src/tdlib/tdlibdata.h
)
