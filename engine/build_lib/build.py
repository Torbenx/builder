import os
import subprocess
import msvcrt
import time
import threading
import sys
import string
import traceback

class arch:
    x86 = 1
    armv7a = 2

def join(*paths):
    ret = ""
    for path in paths:
        if path == "": continue
        if ret != "": ret += "/"
        ret += path
    return ret

def compile(self, source):
    try:
        output = join(self.ibuild_dir, source+".o")
        dep = join(self.ibuild_dir, source+".d")
        rebuild = self.rebuild or (not os.path.exists(output)) or (not os.path.exists(dep))
        if not rebuild:
            with open(dep, "r") as f:
                for line in f:
                    files = string.split(line)
                    for file in files:
                        if file == ":" or file == "\\":
                            continue
                        if os.path.getmtime(file) > self.lastBuildTime:
                            rebuild = True
                            break
                    if rebuild: break
        if not rebuild:
            return
        print "Building "+source
        sourceDir = os.path.dirname(source);
        cmd = self.compile_cmd + [source] + ["-o"+output] + ["-iquote"+join(s, sourceDir) for s in self.include_dirs] + ["-c","-MMD","-MF",dep,"-MT",""] + self.compile_args
        if source in self.file_compile_args:
            cmd += self.file_compile_args[source]
        ret = subprocess.call(cmd)
        if ret != 0:
            print str(cmd)
            self.failed = True
    except Exception as ex:
        traceback.print_exc()
        print ex
        self.failed = True

class Builder(object):
    def __init__(self, target):
        self.input_files = []
        self.output_file = ""
        self.include_dirs = []
        self.libs = []
        self.compile_args = []
        self.link_args = []
        self.build_dir = "build"
        self.target = target
        self.rebuild = False
        self.compile_cmd = []
        self.link_cmd = []
        self.file_compile_args = dict()
        
        self.buildTimeFile = "buildTime"
        self.lastBuildTime = 0
        self.buildTime = 0

    def build(self):
        self.failed = False
        self.buildTime = time.time()
        self.ibuild_dir = self.build_dir
        try:
            with open(join(self.ibuild_dir, self.buildTimeFile), "r") as f:
                self.lastBuildTime = float(f.read())
        except Exception as ex:
            print "First build: "+str(ex)
        
        threads = []
        for source in self.input_files:
            sourceDir = os.path.dirname(source);
            outputDir = join(self.ibuild_dir, sourceDir)
            if not os.path.exists(outputDir):
                os.makedirs(outputDir)
            compile(self, source)
            thread = threading.Thread(target=compile, args=[self, source])
            #thread.start()
            threads.append(thread)
            
        #for thread in threads:
        #    thread.join()

        if self.failed:
            print "BUILD FAILED"
            return
        
        try:
            with open(join(self.ibuild_dir, self.buildTimeFile), "w") as f:
                f.write(str(self.buildTime))
        except Exception as ex:
            print ex
        output_dir = os.path.dirname(self.output_file)
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
        cmd = self.link_cmd + [join(self.ibuild_dir, s+".o") for s in self.input_files] + ["-l" + s for s in self.libs] + ["-o"+self.output_file] + self.link_args
        buildRet = subprocess.call(cmd)

        if buildRet==0:
            elapsed = time.time() - self.buildTime
            print "Build finished in "+str(elapsed)+"s"
        else:
            print str(cmd)
            print "Build failed with "+str(buildRet)
            self.failed = True
