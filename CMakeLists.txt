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
    
    set(lib_dir lib/${ANDROID_ABI}) #library output directory relative from ANDROID_APK_WORKING_DIR
    set(ANDROID_APK_WORKING_DIR .) #apk root directory relative to build directory
    set(ANDROID_KEYSTORE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ../${ANDROID_APK_WORKING_DIR}/${lib_dir})
    
else()
    message(FATAL_ERROR "Unsupported system:  ${CMAKE_SYSTEM_NAME}")
endif()

add_subdirectory(engine)

include_directories(engine)

add_subdirectory(builder)
add_subdirectory(testgame)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")

    #classes.dex
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
        OUTPUT ${ANDROID_APK_WORKING_DIR}/classes.dex
        COMMAND ${Android_DX_EXECUTABLE} --dex --output=${ANDROID_APK_WORKING_DIR}/classes.dex ${classes_jar_file}
        DEPENDS ${classes_jar_file}
        COMMENT "Dexing"
    )
    
    add_custom_target(classes_dex SOURCES classes.dex)
     
    #c++ standard library
    set(cxx_lib_name c++_shared)
    find_library(cxx_lib_source ${cxx_lib_name})
    
    set(cxx_lib_file_name lib${cxx_lib_name}.so)
    add_custom_command(
        OUTPUT  ${ANDROID_APK_WORKING_DIR}/${lib_dir}/${cxx_lib_file_name}
        COMMAND ${CMAKE_COMMAND} -E copy ${cxx_lib_source} ${ANDROID_APK_WORKING_DIR}/${lib_dir}
        DEPENDS ${cxx_lib_source}
        COMMENT "Copying c++ standard library"
    )
    
    add_custom_target(cxx_lib SOURCES ${ANDROID_APK_WORKING_DIR}/${lib_dir}/${cxx_lib_file_name})
    
endif()
