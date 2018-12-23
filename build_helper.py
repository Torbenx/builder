import msvcrt
import os
import subprocess

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
	engineRet = subprocess.call(["python","build_script.py","../"+output_dir+"/engine.dll","1" if rebuild else "0","1" if repreprocess else "0"], cwd="engine", stderr=subprocess.STDOUT)
	print "\n----------------\n"
	mainRet = subprocess.call(["python","build_script.py",output_dir+"/main.exe","1" if rebuild else "0","1" if repreprocess else "0"], stderr=subprocess.STDOUT)
	print "\n----------------\n"
	if(mainRet == 0 and engineRet == 0):
		runRet = subprocess.call(output_dir+"/main.exe", stderr=subprocess.STDOUT)
		print "Exit with "+str(runRet)