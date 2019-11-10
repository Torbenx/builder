import sys
sys.dont_write_bytecode = True

import os
import subprocess
import build_script
import engine.build_script
import traceback
import shutil
from engine.build_lib import android, assets, utils

cwd = os.getcwd()

def choose_target():
    global platform
    global arch
    platform = "android"
    arch = utils.arch_arm64v8a

while (not "platform" in globals()) or not "arch" in globals():
    choose_target()
    
api = 27
jdk = "/usr/lib/jvm/java-8-openjdk-arm64"
sdk = "~/sdk"
keystore_dir = "/storage/internal/dev"

while True:

    print "Target: "+platform+", "+utils.arch_string(arch)
    print "e to exit, r to rebuild, t to change target"
    while utils.kbhit():
        utils.getch()
    key = utils.getch()
    if key == "e":
        break
    utils.clear()
    rebuild = False
    if key == "r":
        rebuild = True
    if key == "t":
        choose_target()
        continue
        
    try:
        reload(assets)
        reload(android)
        reload(engine.build_script)
        reload(build_script)
        
        base_build_dir = "build"
        build_dir = base_build_dir+"/"+platform+"/"+utils.arch_string(arch)
        output_dir = base_build_dir+"/output/"+platform+"/"+utils.arch_string(arch)

        engine_output = ""
        main_output = ""
        compile_cmd = []
        link_cmd = []
        compile_args = []
        if platform == "windows":
            raise Exception()
        if platform == "android":
            lib_dir_rel = utils.join("lib", android.get_lib_dir_name(arch))
            compile_cmd = ["clang++", "-std=c++11", "-nostdinc", "-nostdinc++", "-isystem", "~/ndk/include/c++", "-isystem", "~/ndk/include/sysroot", "-isystem", "~/ndk/include/clang", "-D__ANDROID__", "-D__ANDROID_API__="+str(api)]
            link_cmd = ["clang++", "-L", "~/ndk/lib/lib", "-nostdlib", "-lc", "-ldl", "-lm", "-lgcc", "-lc++_shared", "-fexceptions", "~/ndk/lib/lib/crtbegin_so.o"]
            engine_output = utils.join("engine",build_dir,lib_dir_rel,"libengine.so")
            main_output = utils.join("engine",build_dir,lib_dir_rel,"libmain.so")
            compile_args += ["-DPLATFORM_ANDROID"]
    
        eb = engine.build_script.Builder(platform)
        eb.compile_cmd = compile_cmd
        eb.link_cmd = link_cmd
        eb.rebuild = rebuild
        eb.output_file = utils.join("..",engine_output)
        eb.compile_args += compile_args
        eb.build_dir = build_dir
        eb.concat_cmd = True
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
        mb.concat_cmd = True
        mb.build()
        print "\n----------------\n"
        
        if(eb.failed or mb.failed):
            continue
        
        if platform == "windows":
            assets.copy_assets("assets", output_dir)
            runRet = subprocess.call(utils.join(output_dir,"main.exe"), cwd=output_dir)
            print "Exit with "+str(runRet)
        if platform == "android":
            an = engine.build_script.App()
            an.package = "com.tt.testgame"
            an.build_tools_version = "28.0.3"
            
            an.keystore, an.keystore_password, an.key_alias, an.key_password = android.debug_keystore(keystore_dir, jdk)
            apk_output = utils.join(output_dir,"main.apk")
            an.apk_output = utils.join("..",apk_output)
            
            an.build_dir = build_dir
            an.arch = arch
            an.api = api
            an.sdk = sdk
            an.jdk = jdk
            lib_dir_rel = utils.join("lib", android.get_lib_dir_name(arch))
            an.native_libs = [utils.join(lib_dir_rel,s) for s in ["libengine.so", "libmain.so"]]
            an.native_libs_changed = [eb.changed, mb.changed]
            an.assets_dir = utils.join("..","assets")
            an.aapt_bin = "aapt"
            an.zip_align_bin = "zipalign"
            an.adb_bin = "adb"
            an.std_lib_src = "/home/asdf12/ndk/lib/lib/libc++_shared.so"
            an.rebuild = rebuild
            an.concat_cmd = True
            an.build()
            android.run(an.package,"com.tt.engine.MainActivity",adb_bin="adb",apk_path=apk_output)
            
            
    except Exception as ex:
        os.chdir(cwd)
        traceback.print_exc()
        print ex
