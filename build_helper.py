import sys
sys.dont_write_bytecode = True

import msvcrt
import os
import subprocess
import build_script
import engine.build_script
import traceback

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
		output_dir = "build/output/"+target

		engineOutput = ""
		mainOutput = ""
		bin = ""
		compile_args = []
		if target == "windows":
			engineOutput = output_dir+"/engine.dll"
			mainOutput = output_dir+"/main.exe"
			bin = "g++"
			compile_args += ["-DTARGET_WINDOWS"]
		if target == "android":
			engineOutput = output_dir+"/libengine.so"
			mainOutput = output_dir+"/libmain.so"
			bin = "C:/Users/Torben/Documents/dev/c++/android21armv7a/bin/arm-linux-androideabi-g++.cmd"
			compile_args += ["-DTARGET_ANDROID"]
	
		eb = engine.build_script.Builder(target)
		eb.bin = bin
		eb.rebuild = rebuild
		eb.output_file = "../"+engineOutput
		eb.compile_args += compile_args
		eb.build()
		print "\n----------------\n"
		mb = build_script.Builder(target)
		mb.bin = bin
		mb.rebuild = rebuild
		mb.output_file = mainOutput
		mb.link_args += ["-L"+output_dir]
		mb.compile_args += compile_args
		if target == "android":
			mb.compile_args += ["-fPIC"]
			mb.link_args += ["-shared"]
		mb.build()
		print "\n----------------\n"
		
		if((not mb.failed) and (not eb.failed)):
			if target == "windows":
				runRet = subprocess.call(output_dir+"/main.exe", stderr=subprocess.STDOUT)
				print "Exit with "+str(runRet)
		
	except Exception as ex:
		os.chdir(cwd)
		traceback.print_exc()
		print ex