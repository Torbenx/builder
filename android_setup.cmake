
if(NOT DEFINED CMAKE_SCRIPT_MODE_FILE)
    message(FATAL_ERROR "run with cmake -P ")
endif()

if(NOT DEFINED ANDROID_SDK)
    if(DEFINED ENV{ANDROID_HOME})
        set(ANDROID_SDK $ENV{ANDROID_HOME} CACHE PATH "Android SDK location")
    else()
        message(FATAL_ERROR "No Android SDK")
    endif()
endif()

message(STATUS "Using Android SDK ${ANDROID_SDK}")

get_filename_component(current_dir ${CMAKE_SCRIPT_MODE_FILE} DIRECTORY)

set(ANDROID_MIN_SDK_VERSION 24)
set(ANDROID_TARGET_SDK_VERSION 27)

set(ALL_ABIS "x86" "x86_64" "armeabi-v7a" "arm64-v8a")

if(NOT DEFINED ABIS)
    set(ABIS ${ALL_ABIS})
endif()

include(android_app.cmake)

foreach(abi IN LISTS ABIS)
    list(FIND ALL_ABIS ${abi} _found)
    if(${_found} EQUAL -1)
        message(STATUS "Invalid ABI: ${abi}")
        continue()
    endif()
    message(STATUS "------------------------------------------")
    message(CHECK_START "setting up ANDROID_ABI=${abi}")
    set(ANDROID_STL c++_static)
    if(ENGINE_LINK_DYNAMIC)
        set(ANDROID_STL c++_shared)
    endif()
    execute_process(
        COMMAND ${CMAKE_COMMAND} -G Ninja -S . -B build/android/${abi}
        -DCMAKE_TOOLCHAIN_FILE=${Android_CMAKE_TOOLCHAIN} -DANDROID_ABI=${abi} -DANDROID_PLATFORM=${ANDROID_MIN_SDK_VERSION} -DANDROID_STL=${ANDROID_STL}
        -DANDROID_SDK=${ANDROID_SDK} -DANDROID_MIN_SDK_VERSION=${ANDROID_MIN_SDK_VERSION} -DANDROID_TARGET_SDK_VERSION=${ANDROID_TARGET_SDK_VERSION}
        WORKING_DIRECTORY ${current_dir}
    )
    message(CHECK_PASS "done")
endforeach()
