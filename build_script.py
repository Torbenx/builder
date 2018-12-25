import os
import msvcrt
import subprocess
import sys
from engine import build

class Builder(build.Builder):

	def __init__(self, target):
		super(Builder, self).__init__(target)
		
		self.input_files += [
			"main.cpp",
			"TestRenderer.cpp",
		]

		self.compile_args += ["-Wno-attributes", "-Wall"]

		self.include_dirs += ["engine/include"]
		if target == "windows":
			self.libs += ["engine","Opengl32"]
			self.input_files += ["engine/windows/main.cpp"]
			self.file_compile_args["engine/windows/main.cpp"] = ["-iquoteengine/include"]
		if target == "android":
			self.libs += ["engine", "log", "GLESv2"]
			self.input_files += ["engine/android/main.cpp"]
			self.file_compile_args["engine/android/main.cpp"] = ["-iquoteengine/include"]
			pass
		
	def build(self):
		oldDir = os.getcwd()
		os.chdir(os.path.relpath(os.path.dirname(__file__)))
		super(Builder, self).build()
		os.chdir(oldDir)