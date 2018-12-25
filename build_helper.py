import sys
sys.dont_write_bytecode = True

import msvcrt
import os
import subprocess
import build_script
import engine.build_script
import traceback
import shutil

cwd = os.getcwd()

while True:

	print "e to exit, r to rebuild"
	key = msvcrt.getch()
	if key == "e":
		break
	os.system("cls")
	rebuild = False
	if key == "r":
		rebuild = True
		
	try:
		reload(engine.build)
		reload(engine.build_script)
		reload(build_script)
		
		target = "android"
		base_build_dir = "build"
		build_dir = base_build_dir+"/"+target
		output_dir = base_build_dir+"/output/"+target

		engine_output = ""
		main_output = ""
		bin = ""
		compile_args = []
		if target == "windows":
			engine_output = output_dir+"/engine.dll"
			main_output = output_dir+"/main.exe"
			bin = "g++"
			compile_args += ["-DTARGET_WINDOWS"]
		if target == "android":
			toolchain = "C:/Users/Torben/Documents/dev/c++/android21armv7a"
			arch_dir_name = "armeabi-v7a"
			android_lib_dir_rel = "lib/"+arch_dir_name
			engine_output_rel = android_lib_dir_rel+"/libengine.so"
			main_output_rel = android_lib_dir_rel+"/libmain.so"
			std_lib_rel = android_lib_dir_rel+"/libc++_shared.so"
			shutil.copyfile(toolchain+"/arm-linux-androideabi/lib/armv7-a/libc++_shared.so", build_dir+"/"+std_lib_rel)
			global android_libs
			android_libs = [engine_output_rel, main_output_rel, std_lib_rel]
			engine_output = build_dir+"/"+engine_output_rel
			main_output = build_dir+"/"+main_output_rel
			bin = toolchain+"/bin/arm-linux-androideabi-g++.cmd"
			compile_args += ["-DTARGET_ANDROID"]
	
		eb = engine.build_script.Builder(target)
		eb.bin = bin
		eb.rebuild = rebuild
		eb.output_file = "../"+engine_output
		eb.compile_args += compile_args
		eb.build_dir = build_dir
		eb.build()
		print "\n----------------\n"
		mb = build_script.Builder(target)
		mb.bin = bin
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
			runRet = subprocess.call(output_dir+"/main.exe")
			print "Exit with "+str(runRet)
		if target == "android":
			min_api = 21
			api = 21
			java_dir = "engine/android/java"
			java_source = ["com/tt/engine/MainActivity.java"]
			android_manifest = "engine/android/AndroidManifest.xml"
			package = "com.tt.testgame"
			
			javac_bin = "javac"
			sdk = "C:/Users/Torben/AppData/Local/Android/sdk"
			build_tools_version = "28.0.3"
			build_tools = sdk+"/build-tools/"+build_tools_version
			android_jar = sdk+"/platforms/android-"+str(api)+"/android.jar"
			dx_bin = build_tools+"/dx.bat"
			aapt_bin = build_tools+"/aapt.exe"
			zip_align_bin = build_tools+"/zipalign.exe"
			apk_sign_bin = build_tools+"/apksigner.bat"
			class_output = build_dir+"/class"
			apk_unaligned_name = "unaligned.apk"
			apk_unaligned = build_dir+"/"+apk_unaligned_name
			apk_output = output_dir+"/main.apk"
			keystore = "C:/Users/Torben/.android/debug.keystore"
			keystore_password = "android"
			adb_bin = sdk+"/platform-tools/adb.exe"
			
			if not os.path.exists(class_output):
				os.makedirs(class_output)
			if not os.path.exists(output_dir):
				os.makedirs(output_dir)
				
			javac_ret = subprocess.call([javac_bin, "-d", class_output, "-classpath", java_dir, "-bootclasspath", android_jar] + [java_dir+"/"+s for s in java_source])
			assert javac_ret==0, "javac failed with %d" % javac_ret
				
			dx_ret = subprocess.call([dx_bin, "--dex", "--output", build_dir+"/classes.dex", class_output])
			assert dx_ret==0, "dx failed with %d" % dx_ret
				
			aapt_p_ret = subprocess.call([aapt_bin, "package", "-f", "-M", android_manifest, "-I", android_jar, "-F", apk_unaligned, "--min-sdk-version", str(min_api), "--target-sdk-version", str(api), "--version-code", "1", "--version-name", "1.0", "--rename-manifest-package", package])
			assert aapt_p_ret==0, "aapt package failed with %d" % aapt_p_ret
			
			os.chdir(build_dir)
			aapt_a_ret = aapt_p_ret = subprocess.call([aapt_bin, "add", apk_unaligned_name, "classes.dex"] + android_libs)
			os.chdir(cwd)
			assert aapt_a_ret==0, "aapt add failed with %d" % aapt_a_ret
			
			align_ret = subprocess.call([zip_align_bin, "-f", "4", apk_unaligned, apk_output])
			assert align_ret==0, "zipalign failed with %d" % align_ret
			
			p = subprocess.Popen([apk_sign_bin, "sign", "--ks", keystore, apk_output], stdin=subprocess.PIPE)
			p.communicate(keystore_password+"\n")
			sign_ret = p.returncode
			assert sign_ret==0, "sign failed with %d" % sign_ret
			
			install_ret = subprocess.call([adb_bin, "install", "-r", apk_output])
			assert install_ret==0, "install failed with %d" % install_ret
			
			subprocess.call([adb_bin, "shell", "am", "start", package+"/com.tt.engine.MainActivity"])
			
			p = subprocess.Popen([adb_bin, "shell", "for p in /proc/[0-9]*; do [[ $(<$p/cmdline) = com.tt.testgame ]] && echo ${p##*/}; done"], stdout = subprocess.PIPE)
			out,err = p.communicate()
			if out == "":
				p = subprocess.Popen([adb_bin, "logcat", "-e", package])
				msvcrt.getch()
				p.kill()
			else:
				pid = int(out)
				p = subprocess.Popen([adb_bin, "logcat", "--pid="+str(pid)])
				msvcrt.getch()
				p.kill()
	except Exception as ex:
		os.chdir(cwd)
		traceback.print_exc()
		print ex