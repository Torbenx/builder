import sys
import os
import subprocess
sys.dont_write_bytecode = True

from build_lib import utils, android

shell = True
concat_cmd = False

print("setting up Windows...")
utils.exec_cmd(["cmake", "-G", "Ninja", "-DCMAKE_MAKE_PROGRAM=ninja", "-S", ".", "-B", "build"], shell, concat_cmd)

print("setting up Android...")
android_sdk = os.environ["ANDROID_HOME"]
print("Android SDK: "+android_sdk)

min_sdk_version = 24
target_sdk_version = 27

current_dir = os.path.dirname(os.path.realpath(__file__))

arch_list = [utils.arch_x86_32, utils.arch_x86_64, utils.arch_armv7a, utils.arch_arm64v8a]
for arch in arch_list:
    name = android.get_lib_dir_name(arch)
    print("    "+name+"...")
    utils.exec_cmd([
        "cmake", "-G", "Ninja", "-DCMAKE_MAKE_PROGRAM=ninja", "-S", ".", "-B", "abuild/"+name,
        "-DCMAKE_TOOLCHAIN_FILE="+android_sdk+"/ndk-bundle/build/cmake/android.toolchain.cmake",
        "-DANDROID_ABI="+name, "-DANDROID_PLATFORM="+str(min_sdk_version),
        "-DANDROID_SDK="+android_sdk, "-DANDROID_MIN_SDK_VERSION="+str(min_sdk_version), "-DANDROID_TARGET_SDK_VERSION="+str(target_sdk_version), #custom defines
        "-DANDROID_LOGGER="+current_dir+"/build_lib/android_log.bat"
    ], shell, concat_cmd)