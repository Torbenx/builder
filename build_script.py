import build_file
import engine.build_file
from build_lib import utils, android, assets
import os
import subprocess

api = 27
jdk = "C:/Program Files/Java/jdk1.8.0_131"
sdk = "C:/Users/Torben/AppData/Local/Android/sdk"
keystore_dir = "C:/Users/Torben/Documents/dev/c++"
python_dir = "C:/Python27"

base_build_dir = "build"
build_dir = base_build_dir+"/"+platform+"/"+utils.arch_string(arch)
output_dir = base_build_dir+"/output/"+platform+"/"+utils.arch_string(arch)

engine_output = ""
main_output = ""
compile_cmd = []
link_cmd = []
compile_args = []
if platform == "windows":
    engine_output = utils.join(output_dir,"engine.dll")
    main_output = utils.join(output_dir,"main.exe")
    compile_cmd = ["g++"]
    link_cmd = ["g++"]
    compile_args += ["-DPLATFORM_WINDOWS"]
if platform == "android":
    lib_dir_rel = utils.join("lib", android.get_lib_dir_name(arch))
    compile_cmd,link_cmd = android.setup(api, arch, sdk)
    engine_output = utils.join("engine",build_dir,lib_dir_rel,"libengine.so")
    main_output = utils.join("engine",build_dir,lib_dir_rel,"libmain.so")
    compile_args += ["-DPLATFORM_ANDROID"]

eb = engine.build_file.Builder(platform)
eb.compile_cmd = compile_cmd
eb.link_cmd = link_cmd
eb.rebuild = rebuild
eb.output_file = utils.join("..",engine_output)
eb.compile_args += compile_args
eb.build_dir = build_dir
eb.build()
print "\n----------------\n"
mb = build_file.Builder(platform)
mb.compile_cmd = compile_cmd
mb.link_cmd = link_cmd
mb.rebuild = rebuild
mb.output_file = main_output
mb.include_dirs += [utils.join(python_dir, "include")]
mb.link_args += ["-L"+os.path.dirname(engine_output), "-L"+utils.join(python_dir, "libs")]
mb.compile_args += compile_args
mb.build_dir = build_dir
if platform == "android":
    mb.compile_args += ["-fPIC"]
    mb.link_args += ["-shared"]
mb.build()
print "\n----------------\n"

if(eb.failed or mb.failed):
    raise Error()

if platform == "windows":
    assets.copy_assets("assets", output_dir)
    runRet = subprocess.call(utils.join(output_dir,"main.exe"), cwd=output_dir)
    print "Exit with "+str(runRet)
if platform == "android":
    an = engine.build_file.App()
    an.package = "com.tt.builder"
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
    an.rebuild = rebuild
    an.build()
    android.run(an.package, "com.tt.engine.MainActivity", sdk=sdk, apk_path=apk_output)