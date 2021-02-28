cmake_minimum_required(VERSION 3.13)

set(source
    "main.cpp"
    "TestRenderer.cpp"
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_executable(testgame ${source} ${PLATFORM_MAIN_SOURCE})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    add_library(testgame SHARED ${source} ${PLATFORM_MAIN_SOURCE})
endif()

target_link_libraries(testgame engine)

add_assets(testgame SOURCES
    Pawn/Pawn.obj
    Pawn/Pawn.mtl
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(ANDROID_APK_WORKING_DIR ../${ANDROID_APK_WORKING_DIR})
    #file paths relative to ANDROID_APK_WORKING_DIR except manifest
    add_apk(testgame_apk SOURCES
        ${lib_dir}/libengine.so
        ${lib_dir}/libtestgame.so
        #${lib_dir}/${cxx_lib_file_name}
        classes.dex
        assets/Pawn/Pawn.obj
        PACKAGE com.tt.testgame
        MIN_SDK_VERSION ${ANDROID_MIN_SDK_VERSION} TARGET_SDK_VERSION ${ANDROID_TARGET_SDK_VERSION}
        MANIFEST ../engine/android/AndroidManifest.xml
        ACTIVITY com.tt.engine.MainActivity
        DEBUG
    )
endif()
