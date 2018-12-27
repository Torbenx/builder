import build
import subprocess
import os
import shutil
import msvcrt

def get_lib_dir_name(arch):
    if arch == build.arch_x86_32:
        return "x86"
    if arch == build.arch_x86_64:
        return "x86_64"
    if arch == build.arch_armv7a:
        return "armeabi-v7a"
    if arch == build.arch_arm64v8a:
        return "arm64-v8a"
    raise ValueError("invalid arch")
    
def get_toolchain_dir_name(api, arch):
    if arch == build.arch_armv7a:
        arch2 = "arm"
    elif arch == build.arch_arm64v8a:
        arch2 = "arm64"
    elif arch == build.arch_x86_32:
        arch2 = "x86"
    elif arch == build.arch_x86_64:
        arch2 = "x86_64"
    else:
        raise ValueError("invalid arch")
    return "android-"+str(api)+"-"+arch2
    
def get_std_lib_dir_name(api, arch):
    if arch == build.arch_armv7a:
        return build.join("arm-linux-androideabi","lib","armv7-a")
    if arch == build.arch_arm64v8a:
        return build.join("aarch64-linux-android","lib")
    if arch == build.arch_x86_32:
        return build.join("i686-linux-android","lib")
    if arch == build.arch_x86_64:
        return build.join("x86_64-linux-android","lib64")
    raise ValueError("invalid arch")

def get_std_lib_name(arch):
    return "libc++_shared.so"

def install_toolchain(api, arch, install_dir, sdk):
    toolchain_dir = build.join(install_dir,get_toolchain_dir_name(api, arch))
    if os.path.exists(toolchain_dir):
        return False
    print "Installing toolchain"
    make_toolchain_bin = build.join(sdk,"ndk-bundle","build","tools","make_standalone_toolchain.py")
    cmd = ["python", make_toolchain_bin, "--api", str(api), "--install-dir", toolchain_dir]
    if arch == build.arch_armv7a:
        cmd += ["--arch", "arm"]
    elif arch == build.arch_arm64v8a:
        cmd += ["--arch", "arm64"]
    elif arch == build.arch_x86_32:
        cmd += ["--arch", "x86"]
    elif arch == build.arch_x86_64:
        cmd += ["--arch", "x86_64"]
    else:
        raise ValueError("invalid arch")
    install_ret = subprocess.call(cmd)
    assert install_ret==0, "Toolchain installation failed with %d" % install_ret
    print "Toolchain install successfully"
    return True
    
def setup(api, arch, install_dir, sdk, std_lib_dest_dir):
    std_lib_dest = build.join(std_lib_dest_dir, get_std_lib_name(arch))
    if install_toolchain(api, arch, install_dir, sdk) or not os.path.exists(std_lib_dest):
        if not os.path.exists(std_lib_dest_dir):
            os.makedirs(std_lib_dest_dir)
        shutil.copyfile(build.join(install_dir, get_toolchain_dir_name(api, arch), get_std_lib_dir_name(api, arch), get_std_lib_name(arch)), std_lib_dest)
    if arch == build.arch_armv7a:
        bin = build.join(install_dir,get_toolchain_dir_name(api, arch),"bin","arm-linux-androideabi-g++")
        return [bin,"-march=armv7-a","-mfpu=neon"],[bin,"-march=armv7-a","-Wl,--fix-cortex-a8"]
    if arch == build.arch_arm64v8a:
        bin = build.join(install_dir,get_toolchain_dir_name(api, arch),"bin","aarch64-linux-android-g++")
        return [bin], [bin]
    if arch == build.arch_x86_32:
        bin = build.join(install_dir,get_toolchain_dir_name(api, arch),"bin","i686-linux-android-g++")
        return [bin], [bin]
    if arch == build.arch_x86_64:
        bin = build.join(install_dir,get_toolchain_dir_name(api, arch),"bin","x86_64-linux-android-g++")
        return [bin], [bin] 
        
def gen_key(keystore, keystore_password, key_alias, key_password, dname, validity, jdk):
    keytool_bin = build.join(jdk,"bin","keytool")
    #validity in days
    gen_ret = subprocess.call([keytool_bin, "-genkey", "-keystore", keystore, "-keyalg", "RSA", "-keysize", "2048", "-validity", str(validity), "-storepass", keystore_password, "-alias", key_alias, "-keypass", key_password, "-dname", dname])
    assert gen_ret==0, "Key generation failed with %d" % gen_ret
        
def debug_keystore(dir, jdk):
    file = build.join(dir,"debug.keystore")
    keystore_password = "android"
    key_alias = "androiddebugkey"
    key_password = "android"
    if not os.path.exists(dir):
        os.makedirs(dir)
    if not os.path.exists(file):
        gen_key(file, key_password, key_alias, key_password, "cn=debug", 100*360, jdk)
    return file, key_password, key_alias, key_password
            
def android_build(build_dir, apk_output, sdk, jdk, build_tools_version, min_api, api, android_libs, assets_dir, java_dir, java_source, android_manifest, package, version_code, version_name, keystore, keystore_password, key_alias, key_password):
    build_tools = build.join(sdk,"build-tools",build_tools_version)
    android_jar = build.join(sdk,"platforms","android-"+str(api),"android.jar")
    dx_bin = build.join(build_tools,"dx")
    aapt_bin = build.join(build_tools,"aapt")
    zip_align_bin = build.join(build_tools,"zipalign")
    apk_sign_bin = build.join(build_tools,"apksigner")
    class_output = build.join(build_dir,"class")
    apk_unaligned_name = "unaligned.apk"
    apk_unaligned = build.join(build_dir,apk_unaligned_name)
    apk_unsigned = build.join(build_dir,"unsigned.apk")
    javac_bin = build.join(jdk,"bin","javac")
    
    if not os.path.exists(class_output):
        os.makedirs(class_output)
    if not os.path.exists(os.path.dirname(apk_output)):
        os.makedirs(os.path.dirname(apk_output))
        
    javac_ret = subprocess.call([javac_bin, "-d", class_output, "-classpath", java_dir, "-bootclasspath", android_jar] + [build.join(java_dir,s) for s in java_source], shell=True)
    assert javac_ret==0, "javac failed with %d" % javac_ret
        
    dx_ret = subprocess.call([dx_bin, "--dex", "--output", build.join(build_dir,"classes.dex"), class_output], shell=True)
    assert dx_ret==0, "dx failed with %d" % dx_ret
        
    aapt_p_ret = subprocess.call([aapt_bin, "package", "-f", "-M", android_manifest, "-A", assets_dir, "-I", android_jar, "-F", apk_unaligned, "--min-sdk-version", str(min_api), "--target-sdk-version", str(api), "--version-code", str(version_code), "--version-name", version_name, "--rename-manifest-package", package], shell=True)
    assert aapt_p_ret==0, "aapt package failed with %d" % aapt_p_ret
    
    aapt_a_ret = aapt_p_ret = subprocess.call([aapt_bin, "add", apk_unaligned_name, "classes.dex"] + android_libs, cwd=build_dir, shell=True)
    assert aapt_a_ret==0, "aapt add failed with %d" % aapt_a_ret
    
    align_ret = subprocess.call([zip_align_bin, "-f" ,"-p", "4", apk_unaligned, apk_unsigned], shell=True)
    assert align_ret==0, "zipalign failed with %d" % align_ret
    
    sign_ret = subprocess.call([apk_sign_bin, "sign", "--out", apk_output, "--ks", keystore, "--ks-pass", "pass:"+keystore_password, "--ks-key-alias", key_alias, "--key-pass", "pass:"+key_password, apk_unsigned], shell=True)
    assert sign_ret==0, "sign failed with %d" % sign_ret
    
    
LOG_USAGE = "r to refresh pid, p to log package, c to clear, s to start, k to kill, e to exit logger"

def log_package(adb_bin, package):
    print "Logging "+package+", "+LOG_USAGE
    p = subprocess.Popen([adb_bin, "logcat", "-e", package])
    key = msvcrt.getch()
    p.kill()
    return key
    
def log_pid(pid, adb_bin):
    print "Logging "+str(pid)+", "+LOG_USAGE
    p = subprocess.Popen([adb_bin, "logcat", "--pid="+str(pid)])
    key = msvcrt.getch()
    p.kill()
    return key
    
def find_pid(package, adb_bin):
    p = subprocess.Popen([adb_bin, "shell", "for p in /proc/[0-9]*; do [[ $(<$p/cmdline) = com.tt.testgame ]] && echo ${p##*/}; done"], stdout = subprocess.PIPE)
    out,err = p.communicate()
    if out=="":
        return -1
    else:
        return int(out)
    
def android_run(sdk, apk, package, activity):
    adb_bin = build.join(sdk,"platform-tools","adb.exe")
    
    adb_state = subprocess.check_output([adb_bin, "get-state"])
    assert adb_state.startswith("device"), "Wrong adb state: "+adb_state
    
    install_ret = subprocess.call([adb_bin, "install", "-r", apk])
    assert install_ret==0, "install failed with %d" % install_ret
    
    subprocess.call([adb_bin, "shell", "am", "start", package+"/"+activity])
    
    key = "r"
    while True:
        if key == "r":
            pid = find_pid(package, adb_bin)
            if pid == -1:
                print "process not running, "+LOG_USAGE
                key = msvcrt.getch()
            else:
                key = log_pid(pid, adb_bin)
        elif key == "p":
            key = log_package(adb_bin, package)
        elif key == "c":
            os.system("cls")
            print LOG_USAGE
            key = msvcrt.getch()
        elif key == "k":
            subprocess.call([adb_bin, "shell", "am force-stop "+package])
            print LOG_USAGE
            key = msvcrt.getch()
        elif key == "s":
            subprocess.call([adb_bin, "shell", "am", "start", package+"/"+activity])
            print LOG_USAGE
            key = msvcrt.getch()
        elif key == "e":
            break
        else:
            print LOG_USAGE
            key = msvcrt.getch()