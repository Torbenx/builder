cmake_minimum_required(VERSION 3.18)
project(engine)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

    add_definitions(-DPLATFORM_WINDOWS)
    add_definitions(-Wno-attributes)
    set(PLATFORM_MAIN_SOURCE ../engine/windows/main.cpp)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ..)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ..)
    
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    
    find_package(Java COMPONENTS Development REQUIRED)
    include(UseJava)
    include(android_app.cmake)
    
    add_definitions(-DPLATFORM_ANDROID)
    set(PLATFORM_MAIN_SOURCE ../engine/android/main.cpp)
    set(output_dir lib/${ANDROID_ABI})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ../${output_dir})
    
else()
    message(FATAL_ERROR "Unsupported system:  ${CMAKE_SYSTEM_NAME}")
endif()

add_subdirectory(engine)

include_directories(engine)

add_subdirectory(builder)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")

    set(JAVA_SOURCE_DIR engine/android/java)
    set(CMAKE_JAVA_COMPILE_FLAGS -bootclasspath ${Android_ANDROID_JAR} -target 1.7 -source 1.7)
    set(CMAKE_JAVA_INCLUDE_PATH ${JAVA_SOURCE_DIR})
    add_jar(
        classes
        ${JAVA_SOURCE_DIR}/com/tt/engine/MainActivity.java
        ${JAVA_SOURCE_DIR}/com/tt/engine/Text.java
    )
    get_target_property(classes_jar_file classes JAR_FILE)
    
    add_custom_command(
        OUTPUT classes.dex
        COMMAND ${Android_DX_EXECUTABLE} --dex --output=classes.dex ${classes_jar_file}
        DEPENDS ${classes_jar_file}
        USES_TERMINAL
    )
     
    set(cxx_lib c++_shared)
    find_library(cxx_lib_source ${cxx_lib})
    
    add_custom_command(
        OUTPUT ${output_dir}/lib${cxx_lib}.so
        COMMAND ${CMAKE_COMMAND} -E copy ${cxx_lib_source} ${output_dir}
        DEPENDS ${cxx_lib_source}
    )
    
    add_apk(builder_apk SOURCES
        ${output_dir}/libengine.so
        ${output_dir}/libbuilder.so
        #${output_dir}/lib${cxx_lib}.so
        classes.dex
        PACKAGE com.tt.builder
        MIN_SDK_VERSION ${ANDROID_MIN_SDK_VERSION} TARGET_SDK_VERSION ${ANDROID_TARGET_SDK_VERSION}
        MANIFEST engine/android/AndroidManifest.xml
        DEBUG
    )
    
endif()
