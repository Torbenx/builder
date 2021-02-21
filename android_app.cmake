
function(add_apk _TARGET_NAME)

    cmake_parse_arguments(
        _add_apk
        ""
        "MANIFEST;MIN_SDK_VERSION;TARGET_SDK_VERSION;VERSION_CODE;VERSION_NAME;PACKAGE"
        "SOURCES"
        ${ARGN}
    )
    if(NOT DEFINED _add_apk_MANIFEST)
        message(FATAL_ERROR "must pass a AndroidManifest")
    endif()
    if(NOT DEFINED _add_apk_MIN_SDK_VERSION)
        message(FATAL_ERROR "must pass a minmum sdk version")
    endif()
    if(NOT DEFINED _add_apk_TARGET_SDK_VERSION)
        message(FATAL_ERROR "must pass a target sdk version")
    endif()
    if(NOT DEFINED _add_apk_PACKAGE)
        message(FATAL_ERROR "must pass a package name")
    endif()
    if(NOT DEFINED _add_apk_VERSION_CODE)
        set(_add_apk_VERSION_CODE 1)
        message(DEBUG "version code not passed, defaulting to 1")
    endif()
    if(NOT DEFINED _add_apk_VERSION_NAME)
        set(_add_apk_VERSION_NAME "1.0")
        message(DEBUG "version name not passed, defaulting to 1.0")
    endif()

    set(apk_file ${_TARGET_NAME}_wip.apk)
    set(apk_file_aligned ${_TARGET_NAME}_aligned.apk)
    set(apk_file_output ${_TARGET_NAME}.apk)
    
    set(guard_job_pool ${_TARGET_NAME}_mod_apk)
    set_property(GLOBAL PROPERTY JOB_POOLS ${guard_job_pool}=1)
    
    set(package_depfile dep/${_TARGET_NAME}/package.dep)
    add_custom_command(
        OUTPUT ${package_depfile}
        COMMAND ${Android_AAPT_EXECUTABLE} package -f -M ${CMAKE_CURRENT_SOURCE_DIR}/${_add_apk_MANIFEST} -I ${Android_ANDROID_JAR} -F ${apk_file}
            --min-sdk-version ${_add_apk_MIN_SDK_VERSION} --target-sdk-version ${_add_apk_TARGET_SDK_VERSION}
            --version-code ${_add_apk_VERSION_CODE} --version-name ${_add_apk_VERSION_NAME} --rename-manifest-package ${_add_apk_PACKAGE} --debug-mode
        COMMAND ${CMAKE_COMMAND} -E touch ${package_depfile}
        BYPRODUCTS ${apk_file}
    )
    
    set(add_depfiles)
    foreach(_source IN LISTS _add_apk_SOURCES)
        set(_depfile dep/${_TARGET_NAME}/${_source}.dep)
        add_custom_command(
            OUTPUT ${_depfile}
            COMMAND ${Java_JAR_EXECUTABLE} -uf ${apk_file} ${_source}
            COMMAND ${CMAKE_COMMAND} -E touch ${_depfile}
            DEPENDS ${_source} ${package_depfile}
            JOB_POOL ${guard_job_pool}
        )
        list(APPEND add_depfiles ${_depfile})
    endforeach()
    
    add_custom_command(
        OUTPUT ${apk_file_aligned}
        COMMAND ${Android_ZIPALIGN_EXECUTABLE} -f -p 4 ${apk_file} ${apk_file_aligned}
        DEPENDS ${package_depfile} ${add_depfiles} ${apk_file}
    )
    
    add_custom_command(
        OUTPUT ${apk_file_output}
        COMMAND ${Android_APKSIGNER_EXECUTABLE} sign --out ${apk_file_output}
            --ks ${KEYSTORE_FILE} --ks-pass pass:${KEYSTORE_PASS} --ks-key-alias ${KEYSTORE_KEY_ALIAS} --key-pass pass:${KEYSTORE_KEY_PASS}
            ${apk_file_aligned}
        DEPENDS ${apk_file_aligned}
    )
    
    add_custom_target(${_TARGET_NAME} ALL SOURCES ${apk_file_output})
    
endfunction()