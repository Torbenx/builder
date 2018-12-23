import msvcrt
import os
import subprocess
import sys
from engine.build import Builder

b = Builder()

b.input_files = [
"main.cpp",
"TestRenderer.cpp"
]

b.output_file = "main.exe"
if len(sys.argv) > 1:
	b.output_file = sys.argv[1]

b.compile_args = ["-Wno-attributes", "-Wall"]

b.include_dirs = ["engine/include"]

b.libs = ["engine","Opengl32","Gdi32"]

b.link_args = ["-L."]

b.rebuild = True
if len(sys.argv) > 2 and sys.argv[2] == "0":
	b.rebuild = False

b.repreprocess = True
if len(sys.argv) > 3 and sys.argv[3] == "0":
	b.repreprocess = False
	
b.build()

sys.exit(1 if b.failed else 0)