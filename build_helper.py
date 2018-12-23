import sys
sys.dont_write_bytecode = True

import msvcrt
import os
import subprocess
import build_script
import engine.build_script


output_dir = "."

while True:
	print "e to exit, r to rebuild, p to preprocess"
	key = msvcrt.getch()
	if key == "e":
		break
	os.system("cls")
	rebuild = False
	repreprocess = False
	if key == "r":
		rebuild = True
	if key == "p":
		rebuild = True
		repreprocess = True
		
	reload(engine.build)
	reload(engine.build_script)
	reload(build_script)
	
	eb = engine.build_script.Builder()
	eb.rebuild = rebuild
	eb.repreprocess = repreprocess
	eb.output_file = "../"+output_dir+"/engine.dll"
	eb.build()
	print "\n----------------\n"
	mb = build_script.Builder()
	mb.rebuild = rebuild
	mb.repreprocess = repreprocess
	mb.output_file = output_dir+"/main.exe"
	mb.build()
	print "\n----------------\n"
	if((not mb.failed) and (not eb.failed)):
		runRet = subprocess.call(output_dir+"/main.exe", stderr=subprocess.STDOUT)
		print "Exit with "+str(runRet)