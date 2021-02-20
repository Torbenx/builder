cmake_minimum_required(VERSION 3.13)

project(engine)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

    add_definitions("-DPLATFORM_WINDOWS")
    add_definitions("-Wno-attributes")
    set(platform_main_source "../engine/windows/main.cpp")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "..")
    
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")

    add_definitions("-DPLATFORM_ANDROID")
    set(platform_main_source "../engine/android/main.cpp")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../lib/${ANDROID_ABI}")
    
else()
    message(FATAL_ERROR "Unsupported system:  ${CMAKE_SYSTEM_NAME}")
endif()

add_subdirectory(engine)

include_directories(engine)

add_subdirectory(builder)