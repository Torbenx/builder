import sys
import os
import subprocess
sys.dont_write_bytecode = True

from build_lib import utils, android

keystore_dir = "C:/Users/Torben/Documents/dev/c++"
jdk = "C:/Program Files/Java/jdk1.8.0_131"
sdk = "C:/Users/Torben/AppData/Local/Android/sdk"
build_tools_version = "28.0.3"

arch = utils.arch_armv7a

project_name = "builder"
project_dir = project_name
package = "com.tt."+project_name
apk_name = project_name+".apk"
assets_dir = utils.join(project_dir, "assets")
libs = ["libengine.so", "lib"+project_name+".so"]

build_dir = "android_build"
output_dir = build_dir

an = android.Builder()

an.version_code = 1
an.version_name = "1.0"
an.package = package
an.apk_output = utils.join(output_dir,apk_name)
an.assets_dir = assets_dir

an.api = 27
an.min_api = 24

an.rebuild = True
an.arch = arch

an.build_dir = build_dir
an.sdk = sdk
an.jdk = jdk
an.build_tools_version = build_tools_version

an.java_dir = "engine/android/java"
an.java_source = ["com/tt/engine/MainActivity.java", "com/tt/engine/Text.java"]
an.android_manifest = "engine/android/AndroidManifest.xml"

an.keystore, an.keystore_password, an.key_alias, an.key_password = android.debug_keystore(keystore_dir, jdk)

lib_dir_rel = utils.join("lib", android.get_lib_dir_name(arch))
an.native_libs = [utils.join(lib_dir_rel,s) for s in libs]
an.native_libs_changed = [True, True]

if an.build():
    an.launch("com.tt.engine.MainActivity")
