import build_file
import engine.build_file
from build_lib import utils, android, assets
import os
        
api = 27
jdk = "/usr/lib/jvm/java-8-openjdk-arm64"
sdk = "~/sdk"
keystore_dir = "/storage/internal/dev"
        
base_build_dir = "build"
build_dir = base_build_dir+"/"+platform+"/"+utils.arch_string(arch)
output_dir = base_build_dir+"/output/"+platform+"/"+utils.arch_string(arch)

engine_output = ""
main_output = ""
compile_cmd = []
link_cmd = []
compile_args = []
if platform == "windows":
    raise Error()
if platform == "android":
    lib_dir_rel = utils.join("lib", android.get_lib_dir_name(arch))
    compile_cmd = ["clang++", "-std=c++11", "-nostdinc", "-nostdinc++", "-isystem", "~/ndk/include/sysroot/c++/v1", "-isystem", "~/ndk/include/sysroot", "-isystem", "~/ndk/include/clang", "-isystem", "~/ndk/include/sysroot/aarch64-linux-android", "-D__ANDROID__", "-D__ANDROID_API__="+str(api)]
    link_cmd = ["clang++", "-L", "~/ndk/lib/lib", "-nostdlib", "-lc", "-ldl", "-lm", "-lgcc", "-lc++_shared", "-fexceptions", "~/ndk/lib/lib/crtbegin_so.o"]
    engine_output = utils.join("engine",build_dir,lib_dir_rel,"libengine.so")
    main_output = utils.join("engine",build_dir,lib_dir_rel,"libmain.so")
    compile_args += ["-DPLATFORM_ANDROID"]

eb = engine.build_file.Builder(platform)
eb.logger = logger.child("engine")
eb.compile_cmd = compile_cmd
eb.link_cmd = link_cmd
eb.rebuild = rebuild
eb.output_file = utils.join("..",engine_output)
eb.compile_args += compile_args
eb.build_dir = build_dir
eb.concat_cmd = True
eb.build()

mb = build_file.Builder(platform)
mb.logger = logger.child("main")
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


if(eb.logger.has_failed() or mb.logger.has_failed()):
    logger.failed()
else:
    if platform == "windows":
        pass
    if platform == "android":
        an = engine.build_file.App()
        an.logger = logger.child("android")
        an.package = "com.tt.builder"
        an.build_tools_version = "28.0.3"
        
        an.keystore, an.keystore_password, an.key_alias, an.key_password = android.debug_keystore(keystore_dir, jdk, logger.child("keystore"))
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
        an.std_lib_src = "/home/asdf12/ndk/lib/lib/libc++_shared.so"
        an.rebuild = rebuild
        an.concat_cmd = True
        an.build()
        android.launch(an.package, "com.tt.engine.MainActivity", logger.child("launch"), adb_bin="adb", apk_path=apk_output)
