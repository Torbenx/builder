cmake_minimum_required(VERSION 3.13)

set(source
    "main.cpp"
    "BuildRenderer.cpp"
    "FileExplorer.cpp"
    "HeadBar.cpp"
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_executable(builder ${source} ${PLATFORM_MAIN_SOURCE})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    add_library(builder SHARED ${source} ${PLATFORM_MAIN_SOURCE})
endif()

target_link_libraries(builder engine)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(ANDROID_APK_WORKING_DIR ../${ANDROID_APK_WORKING_DIR})
    #file paths relative to ANDROID_APK_WORKING_DIR except manifest
    add_apk(builder_apk SOURCES
        ${lib_dir}/libengine.so
        ${lib_dir}/libbuilder.so
        #${lib_dir}/${cxx_lib_file_name}
        classes.dex
        PACKAGE com.tt.builder
        MIN_SDK_VERSION ${ANDROID_MIN_SDK_VERSION} TARGET_SDK_VERSION ${ANDROID_TARGET_SDK_VERSION}
        MANIFEST ../engine/android/AndroidManifest.xml
        ACTIVITY com.tt.engine.MainActivity
        DEBUG
    )
endif()