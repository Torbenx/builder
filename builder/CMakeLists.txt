cmake_minimum_required(VERSION 3.13)

set(source
    "main.cpp"
    "BuildRenderer.cpp"
    "FileExplorer.cpp"
    "HeadBar.cpp"
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(win32_executable)
    if(MSVC)
        set(win32_executable WIN32)
    endif()
    add_executable(builder ${win32_executable} ${source} ${PLATFORM_MAIN_SOURCE})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    add_library(builder SHARED ${source} ${PLATFORM_MAIN_SOURCE})
endif()

target_link_libraries(builder engine)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(ANDROID_APP_PACKAGE com.tt.builder)
    set(ANDROID_APP_NAME "Builder")
    set(ANDROID_LIB_NAME builder)
    set(android_manifest ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml)
    configure_file(${CMAKE_SOURCE_DIR}/engine/android/AndroidManifest.xml ${android_manifest})

    #file paths relative to ANDROID_APK_WORKING_DIR
    add_apk(builder_apk SOURCES
        ${lib_dir}/libbuilder.so
        ${PLATFORM_ANDROID_APK_FILES}
        MANIFEST ${android_manifest}
    )

    add_custom_target(run_builder
        COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/build_lib/android_log.py ${Android_ADB_EXECUTABLE} ${ANDROID_APP_PACKAGE} com.tt.engine.MainActivity
        DEPENDS install_builder_apk
        USES_TERMINAL
    )
endif()
