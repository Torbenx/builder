
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)

find_package(Java COMPONENTS Development REQUIRED)
if(NOT DEFINED Java_KEYTOOL_EXECUTABLE)
    find_program(Java_KEYTOOL_EXECUTABLE
      NAMES keytool
      HINTS ${_JAVA_HINTS}
      PATHS ${_JAVA_PATHS}
      REQUIRED
    )
endif()

function(find_latest_version_triple _dir _out_var)
    file(GLOB _versions RELATIVE ${_dir}/ ${_dir}/*)
    list(FILTER _versions INCLUDE REGEX "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    list(SORT _versions COMPARE NATURAL ORDER DESCENDING)
    list(GET _versions 0 _version)
    set(${_out_var} ${_version} PARENT_SCOPE)
endfunction()

if(NOT DEFINED Android_BUILD_TOOLS_DIR)
    find_latest_version_triple(${ANDROID_SDK}/build-tools build_tools_version)
    set(Android_BUILD_TOOLS_DIR ${ANDROID_SDK}/build-tools/${build_tools_version} CACHE PATH "Android Build Tools Directory")
    message(STATUS "Using Android Build Tools Version ${build_tools_version}")
endif()
find_file(Android_DX_JAVA_EXECUTABLE dx.jar PATHS ${Android_BUILD_TOOLS_DIR} ${Android_BUILD_TOOLS_DIR}/lib REQUIRED)
find_file(Android_APKSIGNER_JAVA_EXECUTABLE apksigner.jar PATHS ${Android_BUILD_TOOLS_DIR} ${Android_BUILD_TOOLS_DIR}/lib REQUIRED)
set(Android_DX_EXECUTABLE ${Java_JAVA_EXECUTABLE} -jar ${Android_DX_JAVA_EXECUTABLE})
set(Android_APKSIGNER_EXECUTABLE ${Java_JAVA_EXECUTABLE} -jar ${Android_APKSIGNER_JAVA_EXECUTABLE})
find_program(Android_AAPT_EXECUTABLE aapt PATHS ${Android_BUILD_TOOLS_DIR} REQUIRED)
find_program(Android_ZIPALIGN_EXECUTABLE zipalign PATHS ${Android_BUILD_TOOLS_DIR} REQUIRED)
find_program(Android_ADB_EXECUTABLE adb PATHS ${ANDROID_SDK}/platform-tools REQUIRED)

find_file(Android_ANDROID_JAR android.jar PATHS ${ANDROID_SDK}/platforms/android-${ANDROID_TARGET_SDK_VERSION} REQUIRED)

if(NOT DEFINED Android_NDK_DIR)
    if(EXISTS ${ANDROID_SDK}/ndk)
        find_latest_version_triple(${ANDROID_SDK}/ndk ndk_version)
        message(STATUS "Using Android NDK version ${ndk_version}")
        set(Android_NDK_DIR ${ANDROID_SDK}/ndk/${ndk_version} CACHE PATH "Android NDK Directory")
    elseif(EXISTS ${ANDROID_SDK}/ndk-bundle)
        message(STATUS "Using Android NDK-Bundle")
        set(Android_NDK_DIR ${ANDROID_SDK}/ndk-bundle CACHE PATH "Android NDK Directory")
    else()
        message(FATAL_ERROR "Couldn't find Android NDK")
    endif()
endif()
find_file(Android_CMAKE_TOOLCHAIN android.toolchain.cmake PATHS ${Android_NDK_DIR}/build/cmake REQUIRED)

function(add_apk _TARGET_NAME)

    cmake_parse_arguments(
        _add_apk
        ""
        "MANIFEST"
        "SOURCES"
        ${ARGN}
    )
    if(NOT DEFINED _add_apk_MANIFEST)
        message(FATAL_ERROR "must pass a AndroidManifest")
    endif()

    #relative to ANDROID_APK_WORKING_DIR
    set(apk_file_name ${_TARGET_NAME}_wip.apk)

    set(apk_file ${ANDROID_APK_WORKING_DIR}/${apk_file_name})
    set(apk_file_aligned ${ANDROID_APK_WORKING_DIR}/${_TARGET_NAME}_aligned.apk)
    set(apk_file_output ${ANDROID_APK_WORKING_DIR}/${_TARGET_NAME}.apk)

    set(apk_modification_pool ${_TARGET_NAME}_mod_apk)
    set_property(GLOBAL APPEND PROPERTY JOB_POOLS ${apk_modification_pool}=1)

    set(depfile_dir dep)
    set(package_depfile ${depfile_dir}/package.dep)

    set(package_source_dependencies ${_add_apk_SOURCES})
    list(TRANSFORM package_source_dependencies PREPEND ${ANDROID_APK_WORKING_DIR}/)
    add_custom_command(
        OUTPUT ${package_depfile}
        COMMAND ${Android_AAPT_EXECUTABLE} package -f -M ${_add_apk_MANIFEST} -I ${Android_ANDROID_JAR} -F ${apk_file_name}
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/${package_depfile}
        WORKING_DIRECTORY ${ANDROID_APK_WORKING_DIR}
        DEPENDS ${_add_apk_MANIFEST}
        BYPRODUCTS ${apk_file}
        COMMENT "Packaging ${_TARGET_NAME}"
    )

    set(add_depfiles)
    foreach(_source IN LISTS _add_apk_SOURCES)
        set(_depfile ${depfile_dir}/${_source}.dep)

        add_custom_command(
            OUTPUT ${_depfile}
            COMMAND zip -q ${apk_file_name} ${_source}
            COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/${_depfile}
            WORKING_DIRECTORY ${ANDROID_APK_WORKING_DIR}
            DEPENDS ${ANDROID_APK_WORKING_DIR}/${_source} ${package_depfile}
            JOB_POOL ${apk_modification_pool}
            COMMENT "Updating ${_source} in ${_TARGET_NAME}"
        )
        list(APPEND add_depfiles ${_depfile})
    endforeach()

    add_custom_command(
        OUTPUT ${apk_file_aligned}
        COMMAND ${Android_ZIPALIGN_EXECUTABLE} -f -p 4 ${apk_file} ${apk_file_aligned}
        DEPENDS ${add_depfiles} ${package_depfile}
        COMMENT "Aligning ${_TARGET_NAME}"
    )

    set(keystore_file ${ANDROID_KEYSTORE_DIR}/debug.keystore)
    set(keystore_pass android)
    set(keystore_key_alias androiddebugkey)
    set(keystore_key_pass android)
    math(EXPR keystore_validity "100 * 360") #validity in days

    if(NOT EXISTS ${keystore_file})
        message(CHECK_START "Generating debug.keystore")
        execute_process(
            COMMAND ${Java_KEYTOOL_EXECUTABLE} -genkey -keystore ${keystore_file} -keyalg RSA -keysize 2048 -validity ${keystore_validity}
            -storepass ${keystore_pass} -alias ${keystore_key_alias} -keypass ${keystore_key_pass} -dname "cn=debug"
        )
        message(CHECK_PASS "done")
    endif()

    add_custom_command(
        OUTPUT ${apk_file_output}
        COMMAND ${Android_APKSIGNER_EXECUTABLE} sign --out ${apk_file_output}
            --ks ${keystore_file} --ks-pass pass:${keystore_pass} --ks-key-alias ${keystore_key_alias} --key-pass pass:${keystore_key_pass}
            ${apk_file_aligned}
        DEPENDS ${apk_file_aligned} ${keystore_file}
        COMMENT "Signing ${_TARGET_NAME}"
    )

    add_custom_target(${_TARGET_NAME} ALL SOURCES ${apk_file_output})

    add_custom_target(install_${_TARGET_NAME}
        COMMAND ${Android_ADB_EXECUTABLE} install ${apk_file_output}
        DEPENDS ${_TARGET_NAME}
        COMMENT "Installing ${_TARGET_NAME}"
    )

endfunction()
