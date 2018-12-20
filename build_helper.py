import msvcrt
import os
import subprocess

while True:
	print "e to exit, r to rebuild, p to preprocess"
	key = msvcrt.getch()
	os.system("cls")
	if key == "e":
		break
	os.system("build.py "+key)