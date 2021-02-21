import sys
import os
import subprocess
sys.dont_write_bytecode = True

from build_lib import utils, android

shell = True
concat_cmd = True

print("setting up Android...")
android_sdk = os.environ["ANDROID_HOME"]
print("Android SDK: "+android_sdk)

android_ndk = "/storage/internal/dev/ndk-bundle"

min_sdk_version = 24
target_sdk_version = 27

arch = utils.arch_arm64v8a
name = android.get_lib_dir_name(arch)
print("    "+name+"...")
utils.exec_cmd([
    "cmake", "-G", "Ninja", "-DCMAKE_MAKE_PROGRAM=ninja", "-S", ".", "-B", "build",
    "-DCMAKE_TOOLCHAIN_FILE="+android_ndk+"/build/cmake/android.toolchain.cmake",
    "-DANDROID_ABI="+name, "-DANDROID_PLATFORM="+str(min_sdk_version),
    "-DANDROID_SDK="+android_sdk, "-DANDROID_MIN_SDK_VERSION="+str(min_sdk_version), "-DANDROID_TARGET_SDK_VERSION="+str(target_sdk_version), #custom defines
], shell, concat_cmd)
