import sys
sys.dont_write_bytecode = True

import msvcrt
import os
import subprocess
import build_script
import engine.build_script
import traceback
import shutil
from engine.build_lib import build, android, assets

cwd = os.getcwd()

def choose_target():
    print "a for android, w for windows"
    key = msvcrt.getch()
    global platform
    global arch
    if key == "a":
        platform = "android"
        print "1 for armv7a, 2 for arm64v8a, 3 for x86_32, 4 for x86_64"
        key = msvcrt.getch()
        if key == "1":
            arch = build.arch_armv7a
        elif key == "2":
            arch = build.arch_arm64v8a
        elif key == "3":
            arch = build.arch_x86_32
        elif key == "4":
            arch = build.arch_x86_64
        else:
            print "invalid arch"
    elif key == "w":
        platform = "windows"
        arch = build.arch_x86_64
    else:
        print "Invalid target: "+key

while (not "platform" in globals()) or not "arch" in globals():
    choose_target()
    
api = 27
jdk = "C:/Program Files/Java/jdk1.8.0_131"
sdk = "C:/Users/Torben/AppData/Local/Android/sdk"
toolchain_install_dir = "C:/Users/Torben/Documents/dev/c++/toolchains"

while True:

    print "Target: "+platform+", "+build.arch_string(arch)
    print "e to exit, r to rebuild, t to change target"
    while msvcrt.kbhit():
        msvcrt.getch()
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
        reload(assets)
        reload(android)
        reload(engine.build_script)
        reload(build_script)
        
        base_build_dir = "build"
        build_dir = base_build_dir+"/"+platform+"/"+build.arch_string(arch)
        output_dir = base_build_dir+"/output/"+platform+"/"+build.arch_string(arch)

        engine_output = ""
        main_output = ""
        compile_cmd = []
        link_cmd = []
        compile_args = []
        if platform == "windows":
            engine_output = build.join(output_dir,"engine.dll")
            main_output = build.join(output_dir,"main.exe")
            compile_cmd = ["g++"]
            link_cmd = ["g++"]
            compile_args += ["-DPLATFORM_WINDOWS"]
        if platform == "android":
            lib_dir_rel = build.join("lib", android.get_lib_dir_name(arch))
            compile_cmd,link_cmd = android.setup(api, arch, toolchain_install_dir, sdk)
            engine_output = build.join(build_dir,lib_dir_rel,"libengine.so")
            main_output = build.join(build_dir,lib_dir_rel,"libmain.so")
            compile_args += ["-DPLATFORM_ANDROID"]
    
        eb = engine.build_script.Builder(platform)
        eb.compile_cmd = compile_cmd
        eb.link_cmd = link_cmd
        eb.rebuild = rebuild
        eb.output_file = "../"+engine_output
        eb.compile_args += compile_args
        eb.build_dir = build_dir
        eb.build()
        print "\n----------------\n"
        mb = build_script.Builder(platform)
        mb.compile_cmd = compile_cmd
        mb.link_cmd = link_cmd
        mb.rebuild = rebuild
        mb.output_file = main_output
        mb.link_args += ["-L"+os.path.dirname(engine_output)]
        mb.compile_args += compile_args
        mb.build_dir = build_dir
        if platform == "android":
            mb.compile_args += ["-fPIC"]
            mb.link_args += ["-shared"]
        mb.build()
        print "\n----------------\n"
        
        if(eb.failed or mb.failed):
            continue
        
        if platform == "windows":
            assets.copy_assets("assets", output_dir)
            runRet = subprocess.call(build.join(output_dir,"main.exe"), cwd=output_dir)
            print "Exit with "+str(runRet)
        if platform == "android":
            an = android.Android()
            an.min_api = 21
            an.java_dir = "engine/android/java"
            an.java_source = ["com/tt/engine/MainActivity.java", "com/tt/engine/Text.java"]
            an.android_manifest = "engine/android/AndroidManifest.xml"
            an.package = "com.tt.testgame"
            an.build_tools_version = "28.0.3"
            
            an.keystore, an.keystore_password, an.key_alias, an.key_password = android.debug_keystore(toolchain_install_dir, jdk)
            an.apk_output = build.join(output_dir,"main.apk")
            
            an.build_dir = build_dir
            an.arch = arch
            an.api = api
            an.sdk = sdk
            an.jdk = jdk
            an.install_dir = toolchain_install_dir
            lib_dir_rel = build.join("lib", android.get_lib_dir_name(arch))
            an.native_libs = [build.join(lib_dir_rel,s) for s in ["libengine.so", "libmain.so"]]
            an.native_libs_changed = [eb.changed, mb.changed]
            an.assets_dir = "assets"
            an.version_code = 1
            an.version_name = "1.0"
            an.rebuild = rebuild
            an.build()
            an.run("com.tt.engine.MainActivity")
            
            
    except Exception as ex:
        os.chdir(cwd)
        traceback.print_exc()
        print ex