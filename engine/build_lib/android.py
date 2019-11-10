import utils
import subprocess
import os
import shutil
import utils
import time
import assets

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
    
def get_llvm_prebuild_dir(sdk):
    temp = utils.join(sdk,"ndk-bundle","toolchains","llvm","prebuilt")
    prebuilds = os.listdir(temp)
    if len(prebuilds) != 1:
        raise Error("more than one prebuild architekture")
    return utils.join(temp,prebuilds[0])
    
def setup(api, arch, sdk):
    base = utils.join(get_llvm_prebuild_dir(sdk), "bin")
    appendix = ""
    if not utils.is_windows():
        appendix = "++"
    bin = utils.join(base,get_triple1(arch)+str(api)+"-clang"+appendix)
    if arch == utils.arch_armv7a:
        return [bin,"-march=armv7-a","-mfpu=neon"],[bin,"-march=armv7-a","-Wl,--fix-cortex-a8"]
    if arch == utils.arch_arm64v8a:
        return [bin], [bin]
    if arch == utils.arch_x86_32:
        return [bin], [bin]
    if arch == utils.arch_x86_64:
        return [bin], [bin]
        
def gen_key(keystore, keystore_password, key_alias, key_password, dname, validity, jdk):
    keytool_bin = utils.join(jdk,"bin","keytool")
    #validity in days
    gen_ret = utils.exec_cmd([keytool_bin, "-genkey", "-keystore", keystore, "-keyalg", "RSA", "-keysize", "2048", "-validity", str(validity), "-storepass", keystore_password, "-alias", key_alias, "-keypass", key_password, "-dname", dname], False, False)
    assert gen_ret==0, "Key generation failed with %d" % gen_ret
        
def debug_keystore(dir, jdk):
    file = utils.join(dir,"debug.keystore")
    keystore_password = "android"
    key_alias = "androiddebugkey"
    key_password = "android"
    if not os.path.exists(dir):
        os.makedirs(dir)
    if not os.path.exists(file):
        print "Generating debug keystore"
        gen_key(file, key_password, key_alias, key_password, "cn=debug", 100*360, jdk)
    return file, key_password, key_alias, key_password

class Builder(object):
    def __init__(self):
        self.build_dir = ""
        self.apk_output = ""
        self.arch = 0
        self.sdk = ""
        self.jdk = ""
        self.api = 0
        self.min_api = 0
        self.build_tools_version = ""
        self.native_libs = []
        self.native_libs_changed = []
        self.assets_dir = ""
        self.java_dir = ""
        self.java_source = []
        self.android_manifest = ""
        self.package = ""
        self.version_code = 0
        self.version_name = ""
        self.keystore = ""
        self.keystore_password = ""
        self.key_alias = ""
        self.key_password = ""
        self.install_dir = ""
        self.rebuild = False
        self.debug = True
        self.dx_bin = ""
        self.aapt_bin = ""
        self.zip_align_bin = ""
        self.apk_sign_bin = ""
        self.std_lib_src = ""
        self.concat_cmd = False
        self.adb_bin = ""
        
    def build(self):
        build_tools = utils.join(self.sdk,"build-tools",self.build_tools_version)
        android_jar = utils.join(self.sdk,"platforms","android-"+str(self.api),"android.jar")
        dx_bin = utils.join(build_tools,"dx")
        aapt_bin = utils.join(build_tools,"aapt")
        zip_align_bin = utils.join(build_tools,"zipalign")
        apk_sign_bin = utils.join(build_tools,"apksigner")
        class_output = utils.join(self.build_dir,"class")
        apk_unaligned_name = "unaligned.apk"
        apk_unaligned = utils.join(self.build_dir,apk_unaligned_name)
        apk_unsigned = utils.join(self.build_dir,"unsigned.apk")
        javac_bin = utils.join(self.jdk,"bin","javac")
        assets_copy_dir = utils.join(self.build_dir, "assets")
        
        if self.dx_bin: dx_bin = self.dx_bin
        if self.aapt_bin: aapt_bin = self.aapt_bin
        if self.zip_align_bin: zip_align_bin = self.zip_align_bin
        if self.apk_sign_bin: apk_sign_bin = self.apk_sign_bin
        
        start_time = time.time()
        
        if not os.path.exists(class_output):
            os.makedirs(class_output)
        if not os.path.exists(os.path.dirname(self.apk_output)):
            os.makedirs(os.path.dirname(self.apk_output))
            
        assets_changed = assets.copy_assets(self.assets_dir, assets_copy_dir, rebuild=self.rebuild)
        
        std_lib_changed = False
        lib_dir_rel = utils.join("lib", get_lib_dir_name(self.arch))
        lib_dir = utils.join(self.build_dir, lib_dir_rel)
        std_lib_dest = utils.join(lib_dir, "libc++_shared.so")
        std_lib_src = utils.join(get_llvm_prebuild_dir(self.sdk), "sysroot", "usr", "lib", get_triple2(self.arch), "libc++_shared.so");
        if self.std_lib_src: std_lib_src = self.std_lib_src
        if self.rebuild or not os.path.exists(std_lib_dest):
            std_lib_changed = True
            if not os.path.exists(lib_dir):
                os.makedirs(lib_dir)
            shutil.copyfile(std_lib_src, std_lib_dest)
            
        javac_start = time.time()
        java_changed = False
        for file in self.java_source:
            java_file = utils.join(self.java_dir, file)
            class_file = utils.join(class_output, file.replace(".java",".class"))
            if os.path.exists(class_file) and os.path.getmtime(class_file) > os.path.getmtime(java_file) and not self.rebuild:
                continue
            print "Building "+file
            java_changed = True
            javac_ret = utils.exec_cmd([javac_bin, "-d", class_output, "-classpath", self.java_dir, "-bootclasspath", android_jar, java_file], True, self.concat_cmd)
            assert javac_ret==0, "javac for %s failed with %d" % {file, javac_ret}
        
        dx_start = time.time()
        dx_changed = False
        if java_changed or self.rebuild or not os.path.exists(utils.join(self.build_dir,"classes.dex")):
            dx_changed = True
            dx_ret = utils.exec_cmd([dx_bin, "--dex", "--output", utils.join(self.build_dir,"classes.dex"), class_output], True, self.concat_cmd)
            assert dx_ret==0, "dx failed with %d" % dx_ret
        
        aapt_p_start = time.time()
        apk_empty = False
        if self.rebuild or assets_changed or (not os.path.exists(apk_unaligned)) or os.path.getmtime(self.android_manifest) > os.path.getmtime(apk_unaligned):
            apk_empty = True
            aapt_p_ret = utils.exec_cmd([aapt_bin, "package", "-f", "-M", self.android_manifest, "-A", assets_copy_dir, "-I", android_jar, "-F", apk_unaligned, "--min-sdk-version", str(self.min_api), "--target-sdk-version", str(self.api), "--version-code", str(self.version_code), "--version-name", self.version_name, "--rename-manifest-package", self.package] + (["--debug-mode"] if self.debug else []), True, self.concat_cmd)
            assert aapt_p_ret==0, "aapt package failed with %d" % aapt_p_ret
            
        aapt_ar_start = time.time()
        add_files = []
        if dx_changed or apk_empty:
            add_files += ["classes.dex"]
        if std_lib_changed or apk_empty:
            add_files += [utils.join(lib_dir_rel, "libc++_shared.so")]
        for changed, lib in zip(self.native_libs_changed, self.native_libs):
            if changed or apk_empty:
                add_files += [lib]
        if add_files:
            if not apk_empty:
                aapt_r_ret = utils.exec_cmd([aapt_bin, "remove", apk_unaligned] + add_files, True, self.concat_cmd)
                assert aapt_r_ret==0, "aapt remove failed with %d" % aapt_r_ret
            
            aapt_a_ret = utils.exec_cmd([aapt_bin, "add", apk_unaligned_name] + add_files, True, self.concat_cmd, cwd=self.build_dir)
            assert aapt_a_ret==0, "aapt add failed with %d" % aapt_a_ret
        
        align_start = time.time()
        if add_files or apk_empty:
            align_ret = utils.exec_cmd([zip_align_bin, "-f" ,"-p", "4", apk_unaligned, apk_unsigned], True, self.concat_cmd)
            assert align_ret==0, "zipalign failed with %d" % align_ret
        
        sign_start = time.time()
        if add_files or apk_empty:
            sign_ret = utils.exec_cmd([apk_sign_bin, "sign", "--out", self.apk_output, "--ks", self.keystore, "--ks-pass", "pass:"+self.keystore_password, "--ks-key-alias", self.key_alias, "--key-pass", "pass:"+self.key_password, apk_unsigned], True, self.concat_cmd)
            assert sign_ret==0, "sign failed with %d" % sign_ret
        
        end = time.time()
        print ("Android build finished in {total}s"+("\njavac: {javac}s\ndx: {dx}s\naapt package: {aapt_p}s\naapt add/remove: {aapt_ar}\nalign: {align}s\nsign: {sign}s" if False else "")).format(total=end-start_time, javac=dx_start-javac_start, dx=aapt_p_start-dx_start, aapt_p=aapt_ar_start-aapt_p_start, aapt_ar=align_start-aapt_ar_start, align=sign_start-align_start, sign=end-sign_start)
        
    
LOG_USAGE = "r to refresh pid, p to log package, c to clear, s to start, k to kill, e to exit logger"

def log_package(package, adb_bin):
    print "Logging "+package+", "+LOG_USAGE
    p = subprocess.Popen([adb_bin, "logcat", "-e", package])
    key = utils.getch()
    p.kill()
    return key
    
def log_pid(pid, adb_bin):
    print "Logging "+str(pid)+", "+LOG_USAGE
    p = subprocess.Popen([adb_bin, "logcat", "--pid="+str(pid)])
    key = utils.getch()
    p.kill()
    return key
    
def find_pid(package, adb_bin):
    p = subprocess.Popen([adb_bin, "shell", "for p in /proc/[0-9]*; do [[ $(<$p/cmdline) = %s ]] && echo ${p##*/}; done" % package], stdout = subprocess.PIPE)
    out,err = p.communicate()
    if out=="":
        return -1
    else:
        return int(out)
    
def run(package, activity, apk_path="", use_pm=False, sdk="", adb_bin=""):
    if not adb_bin:
        adb_bin = utils.join(sdk,"platform-tools","adb.exe")
    
    adb_state = subprocess.check_output([adb_bin, "get-state"])
    assert adb_state.startswith("device"), "Wrong adb state: "+adb_state
    
    install_ret = 0
    if not use_pm:
        install_ret = utils.exec_cmd([adb_bin, "install", "-r", apk_path], False, False)
    else:
        install_ret = utils.exec_cmd([adb_bin], "shell", "pm", "install", apk_path, False, False)
    assert install_ret==0, "install failed with %d" % install_ret
    
    utils.exec_cmd([adb_bin, "shell", "am", "start", package+"/"+activity], False, False)
    
    key = "r"
    while True:
        if key == "r":
            pid = find_pid(package, adb_bin)
            if pid == -1:
                print "process not running, "+LOG_USAGE
                key = utils.getch()
            else:
                key = log_pid(pid, adb_bin)
        elif key == "p":
            key = log_package(package, adb_bin)
        elif key == "c":
            os.system("cls")
            print LOG_USAGE
            key = utils.getch()
        elif key == "k":
            utils.exec_cmd([adb_bin, "shell", "am force-stop "+package], False, False)
            print LOG_USAGE
            key = utils.getch()
        elif key == "s":
            utils.exec_cmd([adb_bin, "shell", "am", "start", package+"/"+activity], False, False)
            print LOG_USAGE
            key = utils.getch()
        elif key == "e":
            break
        else:
            print LOG_USAGE
            key = utils.getch()
