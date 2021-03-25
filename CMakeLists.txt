cmake_minimum_required(VERSION 3.18)
project(engine)

function(add_assets _TARGET)
    cmake_parse_arguments(
        _add_assets
        ""
        ""
        "SOURCES"
        ${ARGN}
    )
    set(_OUTPUTS)
    foreach(_source IN LISTS _add_assets_SOURCES)
        set(_output ${ASSETS_OUTPUT_DIR}/assets/${_source})
        list(APPEND _OUTPUTS ${_output})
        add_custom_command(
            OUTPUT ${_output}
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/assets/${_source} ${_output}
            COMMENT "Copying asset ${_source}"
        )
    endforeach()
    add_custom_target(${_TARGET}_assets SOURCES ${_OUTPUTS})
    add_dependencies(${_TARGET} ${_TARGET}_assets)
endfunction()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

    add_definitions(-DPLATFORM_WINDOWS)
    add_definitions(-Wno-attributes)
    set(PLATFORM_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/engine/windows/main.cpp)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(ASSETS_OUTPUT_DIR ${CMAKE_BINARY_DIR})

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")

    find_package(Java COMPONENTS Development REQUIRED)
    include(UseJava)
    include(android_app.cmake)

    add_definitions(-DPLATFORM_ANDROID)
    set(PLATFORM_MAIN_SOURCE ../engine/android/main.cpp)

    set(lib_dir lib/${ANDROID_ABI}) #library output directory relative from ANDROID_APK_WORKING_DIR
    set(ANDROID_APK_WORKING_DIR ${CMAKE_BINARY_DIR}) #apk root directory relative to build directory
    if(NOT DEFINED ANDROID_KEYSTORE_DIR)
        set(ANDROID_KEYSTORE_DIR ${CMAKE_SOURCE_DIR})
    endif()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ANDROID_APK_WORKING_DIR}/${lib_dir})
    set(ASSETS_OUTPUT_DIR ${ANDROID_APK_WORKING_DIR})

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
    set(CMAKE_JAVA_COMPILE_FLAGS -bootclasspath ${Android_ANDROID_JAR} -target 1.8 -source 1.8)
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
