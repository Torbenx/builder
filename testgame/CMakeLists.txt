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

set(ANDROID_APP_PACKAGE com.tt.testgame)
set(ANDROID_APP_NAME "TestGame")
set(android_manifest ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml)
configure_file(${CMAKE_SOURCE_DIR}/engine/android/AndroidManifest.xml ${android_manifest})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    #file paths relative to ANDROID_APK_WORKING_DIR
    add_apk(testgame_apk SOURCES
        ${lib_dir}/libengine.so
        ${lib_dir}/libtestgame.so
        ${lib_dir}/libturbojpeg.so
        #${lib_dir}/${cxx_lib_file_name}
        classes.dex
        assets/Pawn/Pawn.obj
        PACKAGE com.tt.testgame
        ACTIVITY com.tt.engine.MainActivity
        MANIFEST ${android_manifest}
    )
endif()
