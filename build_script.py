import os
import msvcrt
import subprocess
import sys
from engine import build

class Builder(build.Builder):
	input_files = [
		"main.cpp",
		"TestRenderer.cpp",
	]

	compile_args = ["-Wno-attributes", "-Wall"]

	include_dirs = ["engine/include"]

	libs = ["engine","Opengl32","Gdi32"]

	link_args = ["-L."]