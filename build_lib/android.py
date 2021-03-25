import utils
import subprocess
import os
import shutil
import time
import re

def get_lib_dir_name(arch):
    if arch == utils.arch_x86_32:
        return "x86"
    if arch == utils.arch_x86_64:
        return "x86_64"
    if arch == utils.arch_armv7a:
        return "armeabi-v7a"
    if arch == utils.arch_arm64v8a:
        return "arm64-v8a"
    raise ValueError("invalid arch")

def get_triple1(arch):
    if arch == utils.arch_armv7a:
        return "armv7a-linux-androideabi"
    if arch == utils.arch_arm64v8a:
        return "aarch64-linux-android"
    if arch == utils.arch_x86_32:
        return "i686-linux-android"
    if arch == utils.arch_x86_64:
        return "x86_64-linux-android"
    raise ValueError("invalid arch")

def get_triple2(arch):
    if arch == utils.arch_armv7a:
        return "arm-linux-androideabi"
    if arch == utils.arch_arm64v8a:
        return "aarch64-linux-android"
    if arch == utils.arch_x86_32:
        return "i686-linux-android"
    if arch == utils.arch_x86_64:
        return "x86_64-linux-android"
    raise ValueError("invalid arch")

def version_triple_key(v):
    v = map(int, v.split("."));
    return v[2] + (v[1] + v[0] * 1000000000) * 1000000000

def find_latest(dir):
    prog = re.compile("^[0-9]+\\.[0-9]+\\.[0-9]+$")
    dirs = os.listdir(dir)
    versions = []
    for version_string in dirs:
        if prog.match(version_string):
            versions += [version_string]
    versions = sorted(versions, key=version_triple_key, reverse=True)
    return versions[0];

def find_latest_ndk(sdk):
    if os.path.isdir(utils.join(sdk, "ndk")):
        return utils.join(sdk, "ndk", find_latest(utils.join(sdk, "ndk")))
    elif os.path.isdir(utils.join(sdk, "ndk-bundle")):
        return utils.join(sdk, "ndk-bundle")
    else:
        raise Exception("couldn't find ndk")

def get_llvm_prebuild_dir(ndk):
    temp = utils.join(ndk, "toolchains", "llvm", "prebuilt")
    prebuilds = os.listdir(temp)
    if len(prebuilds) != 1:
        raise Exception("more than one prebuild architekture")
    return utils.join(temp, prebuilds[0])
