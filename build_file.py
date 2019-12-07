import os
from build_lib import c

class Builder(c.Builder):

    def __init__(self, target):
        super(Builder, self).__init__(target)
        
        self.input_files += [
            "main.cpp",
            "BuildRenderer.cpp",
            "FileExplorer.cpp",
        ]

        self.compile_args += ["-Wno-attributes", "-Wall", "-D_hypot=hypot"]

        self.include_dirs += ["engine"]
        self.libs += ["engine"]
        
        if target == "windows":
            self.libs += ["Opengl32"]
            self.input_files += ["engine/windows/main.cpp"]
            self.file_compile_args["engine/windows/main.cpp"] = ["-iquoteengine"]
        if target == "android":
            self.libs += ["log", "GLESv2"]
            self.input_files += ["engine/android/main.cpp"]
            self.file_compile_args["engine/android/main.cpp"] = ["-iquoteengine"]
        
    def build(self):
        oldDir = os.getcwd()
        if os.path.dirname(__file__): os.chdir(os.path.dirname(__file__))
        super(Builder, self).build()
        os.chdir(oldDir)
