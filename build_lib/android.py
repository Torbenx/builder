import utils
import subprocess
import os
import shutil
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
        
        
def debug_keystore(dir, jdk):
    file = utils.join(dir,"debug.keystore")
    keystore_password = "android"
    key_alias = "androiddebugkey"
    key_password = "android"
    if not os.path.exists(dir):
        os.makedirs(dir)
    if not os.path.exists(file):
        keytool_bin = utils.join(jdk,"bin","keytool")
        #validity in days
        cmd = [keytool_bin, "-genkey", "-keystore", file, "-keyalg", "RSA", "-keysize", "2048", "-validity", str(100*360), "-storepass", keystore_password, "-alias", key_alias, "-keypass", key_password, "-dname", "cn=debug"]
        ret = utils.exec_cmd(cmd, False, False)
        if ret != 0:
            raise Exception("failed to create keystore")
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
        self.adb_bin = ""
        self.std_lib_src = ""
        self.concat_cmd = False
        
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
        
        if not os.path.exists(class_output):
            os.makedirs(class_output)
        if not os.path.exists(os.path.dirname(self.apk_output)):
            os.makedirs(os.path.dirname(self.apk_output))
            
        assets_changed = utils.copy_assets(self.assets_dir, assets_copy_dir, rebuild=self.rebuild)
        
        std_lib_changed = False
        lib_dir_rel = utils.join("lib", get_lib_dir_name(self.arch))
        lib_dir = utils.join(self.build_dir, lib_dir_rel)
        std_lib_dest = utils.join(lib_dir, "libc++_shared.so")
        std_lib_src = self.std_lib_src
        if not std_lib_src:
            std_lib_src = utils.join(get_llvm_prebuild_dir(self.sdk), "sysroot", "usr", "lib", get_triple2(self.arch), "libc++_shared.so")
        if self.rebuild or not os.path.exists(std_lib_dest):
            std_lib_changed = True
            if not os.path.exists(lib_dir):
                os.makedirs(lib_dir)
            shutil.copyfile(std_lib_src, std_lib_dest)
            
        java_changed = False
        for file in self.java_source:
            java_file = utils.join(self.java_dir, file)
            class_file = utils.join(class_output, file.replace(".java",".class"))
            if os.path.exists(class_file) and os.path.getmtime(class_file) > os.path.getmtime(java_file) and not self.rebuild:
                continue
            java_changed = True
            javac_ret = utils.exec_cmd([javac_bin, "-d", class_output, "-classpath", self.java_dir, "-bootclasspath", android_jar, java_file], True, self.concat_cmd)
            if javac_ret != 0:
                return False
        
        dx_changed = False
        if java_changed or self.rebuild or not os.path.exists(utils.join(self.build_dir,"classes.dex")):
            dx_changed = True
            dx_ret = utils.exec_cmd([dx_bin, "--dex", "--output", utils.join(self.build_dir,"classes.dex"), class_output], True, self.concat_cmd)
            if dx_ret != 0:
                print "dx returned with "+str(dx_ret)
                return False
        
        apk_empty = False
        if self.rebuild or assets_changed or (not os.path.exists(apk_unaligned)) or os.path.getmtime(self.android_manifest) > os.path.getmtime(apk_unaligned):
            apk_empty = True
            aapt_p_ret = utils.exec_cmd([aapt_bin, "package", "-f", "-M", self.android_manifest, "-A", assets_copy_dir, "-I", android_jar, "-F", apk_unaligned, "--min-sdk-version", str(self.min_api), "--target-sdk-version", str(self.api), "--version-code", str(self.version_code), "--version-name", self.version_name, "--rename-manifest-package", self.package] + (["--debug-mode"] if self.debug else []), True, self.concat_cmd)
            if aapt_p_ret != 0:
                print "aapt package returned with "+str(aapt_p_ret)
                return False
            
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
                if aapt_r_ret != 0:
                    print "aapt remove returned with "+str(aapt_r_ret)
                    return False
            
            aapt_a_ret = utils.exec_cmd([aapt_bin, "add", apk_unaligned_name] + add_files, True, self.concat_cmd, cwd=self.build_dir)
            if aapt_a_ret != 0:
                print "aapt add returned with "+str(aapt_a_ret)
                return False
        
        if add_files or apk_empty:
            align_ret = utils.exec_cmd([zip_align_bin, "-f" ,"-p", "4", apk_unaligned, apk_unsigned], True, self.concat_cmd)
            if align_ret != 0:
                print "zipalign returned with "+str(align_ret)
                return False
        
        if add_files or apk_empty:
            sign_ret = utils.exec_cmd([apk_sign_bin, "sign", "--out", self.apk_output, "--ks", self.keystore, "--ks-pass", "pass:"+self.keystore_password, "--ks-key-alias", self.key_alias, "--key-pass", "pass:"+self.key_password, apk_unsigned], True, self.concat_cmd)
            if sign_ret != 0:
                print "apksign returned with "+str(sign_ret)
                return False
                
        return True
        
    
    def launch(self, activity, use_pm=False):
        adb_bin = utils.join(self.sdk,"platform-tools","adb.exe")
        if self.adb_bin:
            adb_bin = self.adb_bin
        
        adb_state = subprocess.check_output([adb_bin, "get-state"])
        if not adb_state.startswith("device"):
            print "Wrong adb state: {"+adb_state+"}"
            return False
        
        install_ret = 0
        if not use_pm:
            install_ret = utils.exec_cmd([adb_bin, "install", "-r", self.apk_output], False, self.concat_cmd)
        else:
            install_ret = utils.exec_cmd([adb_bin, "shell", "pm", "install", self.apk_output], False, self.concat_cmd)
        if install_ret != 0:
            print "install returned with "+str(install_ret)
            return False
        
        utils.exec_cmd([adb_bin, "shell", "am", "start", self.package+"/"+activity], False, self.concat_cmd)
        return True
