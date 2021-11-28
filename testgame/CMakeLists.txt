cmake_minimum_required(VERSION 3.13)

set(source
    "main.cpp"
    "TestRenderer.cpp"
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(win32_executable)
    if(MSVC)
        set(win32_executable WIN32)
    endif()
    add_executable(testgame ${win32_executable} ${source} ${PLATFORM_MAIN_SOURCE})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    add_library(testgame SHARED ${source} ${PLATFORM_MAIN_SOURCE})
endif()

target_link_libraries(testgame engine)

set(assets
    PawnSideA/piece.obj
    PawnSideA/piece.jpg
    PawnSideB/piece.obj
    PawnSideB/piece.jpg
    RookSideB/piece.obj
    RookSideB/piece.jpg
    KnightSideA/piece.obj
    KnightSideA/piece.jpg
    KnightSideB/piece.obj
    KnightSideB/piece.jpg
    BishopSideA/piece.obj
    BishopSideA/piece.jpg
    BishopSideB/piece.obj
    BishopSideB/piece.jpg
    KingSideA/piece.obj
    KingSideA/piece.jpg
    KingSideB/piece.obj
    KingSideB/piece.jpg
    QueenSideA/piece.obj
    QueenSideA/piece.jpg
    QueenSideB/piece.obj
    QueenSideB/piece.jpg
)

add_assets(testgame SOURCES ${assets})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(ANDROID_APP_PACKAGE com.tt.testgame)
    set(ANDROID_APP_NAME "TestGame")
    set(ANDROID_LIB_NAME testgame)
    set(android_manifest ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml)
    configure_file(${CMAKE_SOURCE_DIR}/engine/android/AndroidManifest.xml ${android_manifest})

    #file paths relative to ANDROID_APK_WORKING_DIR
    list(TRANSFORM assets PREPEND assets/)
    add_apk(testgame_apk SOURCES
        ${lib_dir}/libtestgame.so
        ${PLATFORM_ANDROID_APK_FILES}
        ${assets}
        MANIFEST ${android_manifest}
    )

    add_custom_target(run_testgame
        COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/build_lib/android_log.py ${Android_ADB_EXECUTABLE} ${ANDROID_APP_PACKAGE} com.tt.engine.MainActivity
        DEPENDS install_testgame_apk
        USES_TERMINAL
    )
endif()
