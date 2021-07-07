cmake_minimum_required(VERSION 3.18)
project(engine)

set(CMAKE_BUILD_TYPE release)
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O2 -g")
set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O2 -g")

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

    set(PLATFORM_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/engine/windows/main.cpp)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(ASSETS_OUTPUT_DIR ${CMAKE_BINARY_DIR})

    add_definitions("-DNOMINMAX")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")

    include_directories(libs/EGL/api)
    include_directories(libs/OpenGL/api)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")

    find_package(Python COMPONENTS Interpreter REQUIRED)

    include(android_app.cmake)
    include(UseJava)

    set(PLATFORM_MAIN_SOURCE ../engine/android/main.cpp)

    set(lib_dir lib/${ANDROID_ABI}) #library output directory relative from ANDROID_APK_WORKING_DIR
    set(ANDROID_APK_WORKING_DIR ${CMAKE_BINARY_DIR}) #apk root directory relative to build directory
    if(NOT DEFINED ANDROID_KEYSTORE_DIR)
        set(ANDROID_KEYSTORE_DIR ${CMAKE_SOURCE_DIR})
    endif()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ANDROID_APK_WORKING_DIR}/${lib_dir})
    set(ASSETS_OUTPUT_DIR ${ANDROID_APK_WORKING_DIR})

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

    set(PLATFORM_ANDROID_APK_FILES
        ${lib_dir}/libturbojpeg.so
        #${lib_dir}/libclang_rt.asan-aarch64-android.so
        classes.dex
    )
    if(ENGINE_LINK_DYNAMIC)
        #c++ standard library
        set(lib_cxx_name c++_shared)
        find_library(lib_cxx_source ${lib_cxx_name} REQUIRED)

        set(lib_cxx_file_name lib${lib_cxx_name}.so)
        add_custom_command(
            OUTPUT  ${ANDROID_APK_WORKING_DIR}/${lib_dir}/${lib_cxx_file_name}
            COMMAND ${CMAKE_COMMAND} -E copy ${lib_cxx_source} ${ANDROID_APK_WORKING_DIR}/${lib_dir}
            DEPENDS ${lib_cxx_source}
            COMMENT "Copying c++ standard library"
        )

        add_custom_target(lib_cxx SOURCES ${ANDROID_APK_WORKING_DIR}/${lib_dir}/${lib_cxx_file_name})

        list(APPEND PLATFORM_ANDROID_APK_FILES ${lib_dir}/libengine.so ${lib_dir}/${lib_cxx_file_name})
    endif()
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

else()
    message(FATAL_ERROR "Unsupported system:  ${CMAKE_SYSTEM_NAME}")
endif()

find_package(Vulkan)
if (Vulkan_FOUND)
    message("Vulkan_INCLUDE_DIRS=${Vulkan_INCLUDE_DIRS}")
    include_directories(${Vulkan_INCLUDE_DIRS})
endif()

include_directories(libs/libjpeg-turbo)
include_directories(libs/abseil)
include_directories(engine)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(engine)

add_subdirectory(builder)
add_subdirectory(testgame)

add_subdirectory(libs)
