import sys
sys.dont_write_bytecode = True

import msvcrt
import os
import subprocess
import build_script
import engine.build_script
import traceback
import shutil
from engine.build_lib import build
from engine.build_lib import android

cwd = os.getcwd()

def choose_target():
    print "a for android, w for windows"
    key = msvcrt.getch()
    global target
    global arch
    if key == "a":
        target = "android"
        arch = build.arch.armv7a
    elif key == "w":
        target = "windows"
        arch = build.arch.x86
    else:
        print "Invalid target: "+key

while not "target" in globals():
    choose_target()
    
api = 26
jdk = "C:/Program Files/Java/jdk1.8.0_131"
sdk = "C:/Users/Torben/AppData/Local/Android/sdk"
toolchain_install_dir = "C:/Users/Torben/Documents/dev/c++/toolchains"

while True:

    print "Target: "+target
    print "e to exit, r to rebuild, t to change target"
    key = msvcrt.getch()
    if key == "e":
        break
    os.system("cls")
    rebuild = False
    if key == "r":
        rebuild = True
    if key == "t":
        choose_target()
        continue
        
    try:
        reload(build)
        reload(android)
        reload(engine.build_script)
        reload(build_script)
        
        base_build_dir = "build"
        build_dir = base_build_dir+"/"+target
        output_dir = base_build_dir+"/output/"+target

        engine_output = ""
        main_output = ""
        compile_cmd = []
        link_cmd = []
        compile_args = []
        if target == "windows":
            engine_output = build.join(output_dir,"engine.dll")
            main_output = build.join(output_dir,"main.exe")
            compile_cmd = ["g++"]
            link_cmd = ["g++"]
            compile_args += ["-DTARGET_WINDOWS"]
        if target == "android":
            lib_dir_rel = build.join("lib", android.get_lib_dir_name(arch))
            compile_cmd,link_cmd = android.setup(api, arch, toolchain_install_dir, sdk, build.join(build_dir, lib_dir_rel))
            engine_output = build.join(build_dir,lib_dir_rel,"libengine.so")
            main_output = build.join(build_dir,lib_dir_rel,"libmain.so")
            compile_args += ["-DTARGET_ANDROID"]
    
        eb = engine.build_script.Builder(target)
        eb.compile_cmd = compile_cmd
        eb.link_cmd = link_cmd
        eb.rebuild = rebuild
        eb.output_file = "../"+engine_output
        eb.compile_args += compile_args
        eb.build_dir = build_dir
        eb.build()
        print "\n----------------\n"
        mb = build_script.Builder(target)
        mb.compile_cmd = compile_cmd
        mb.link_cmd = link_cmd
        mb.rebuild = rebuild
        mb.output_file = main_output
        mb.link_args += ["-L"+os.path.dirname(engine_output)]
        mb.compile_args += compile_args
        mb.build_dir = build_dir
        if target == "android":
            mb.compile_args += ["-fPIC"]
            mb.link_args += ["-shared"]
        mb.build()
        print "\n----------------\n"
        
        if(eb.failed or mb.failed):
            continue
        
        if target == "windows":
            runRet = subprocess.call(build.join(output_dir,"main.exe"))
            print "Exit with "+str(runRet)
        if target == "android":
            min_api = 21
            java_dir = "engine/android/java"
            java_source = ["com/tt/engine/MainActivity.java"]
            android_manifest = "engine/android/AndroidManifest.xml"
            package = "com.tt.testgame"
            build_tools_version = "28.0.3"
            
            keystore, keystore_password, key_alias, key_password = android.debug_keystore(toolchain_install_dir, jdk)
            apk_output = build.join(output_dir,"main.apk")
            
            lib_dir_rel = build.join("lib", android.get_lib_dir_name(arch))
            android.android_build(build_dir, apk_output, sdk, jdk, build_tools_version, min_api, api, [build.join(lib_dir_rel,s) for s in ["libengine.so", "libmain.so", android.get_std_lib_name(arch)]], java_dir, java_source, android_manifest, package, 1, "1.0", keystore, keystore_password, key_alias, key_password)
            android.android_run(sdk, apk_output, package, "com.tt.engine.MainActivity")
            
            
    except Exception as ex:
        os.chdir(cwd)
        traceback.print_exc()
        print ex