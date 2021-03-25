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

set(ANDROID_APP_PACKAGE com.tt.builder)
set(ANDROID_APP_NAME "Builder")
set(android_manifest ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml)
configure_file(${CMAKE_SOURCE_DIR}/engine/android/AndroidManifest.xml ${android_manifest})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    #file paths relative to ANDROID_APK_WORKING_DIR
    add_apk(builder_apk SOURCES
        ${lib_dir}/libengine.so
        ${lib_dir}/libbuilder.so
        ${lib_dir}/libturbojpeg.so
        #${lib_dir}/${cxx_lib_file_name}
        classes.dex
        PACKAGE com.tt.builder
        ACTIVITY com.tt.engine.MainActivity
        MANIFEST ${android_manifest}
    )
endif()
