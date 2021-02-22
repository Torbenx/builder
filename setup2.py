import sys
import os
import subprocess
sys.dont_write_bytecode = True

from build_lib import utils, android

shell = True
concat_cmd = True

"""
required packages:
    cmake
    clang
    ninja-build
    default-jdk-headless (or other jdk)
    android-sdk

steps:
    set ANDROID_HOME=sdk location (/usr/lib/android-sdk)
    add apksigner wrapper script that calls apksigner.jar
    copy ndk_bundle from an x86 maschine
    insert ndk_bundle in correct location (symlink)
    in ndk_bundle/toolchains/llvm/prebuilt/ rename:
        windows-x86_64 -> linux-x86_64
    in ndk_bundle/build/cmake/android.toolchain.cmake replace:
        set(ANDROID_C_COMPILER "clang")
        set(ANDROID_CXX_COMPILER "clang++")
        set(ANDROID_ASM_COMPILER "clang")
        set(ANDROID_AR "llvm-ar-7") (not tested)
"""

print("setting up Android...")
android_sdk = os.environ["ANDROID_HOME"]
print("Android SDK: "+android_sdk)

min_sdk_version = 24
target_sdk_version = 27

arch = utils.arch_arm64v8a
name = android.get_lib_dir_name(arch)
print("    "+name+"...")
utils.exec_cmd([
    "cmake", "-G", "Ninja", "-DCMAKE_MAKE_PROGRAM=ninja", "-S", ".", "-B", "build",
    "-DCMAKE_TOOLCHAIN_FILE="+android_sdk+"/ndk_bundle/build/cmake/android.toolchain.cmake",
    "-DANDROID_ABI="+name, "-DANDROID_PLATFORM="+str(min_sdk_version),
    "-DANDROID_SDK="+android_sdk, "-DANDROID_MIN_SDK_VERSION="+str(min_sdk_version), "-DANDROID_TARGET_SDK_VERSION="+str(target_sdk_version), #custom defines
], shell, concat_cmd)
