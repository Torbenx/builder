import os
import time
import threading
import sys
import string
import traceback
import utils

def compile(self, source, logger):
    handle = logger.log(source)
    try:
        output = utils.join(self.ibuild_dir, source+".o")
        dep = utils.join(self.ibuild_dir, source+".d")
        rebuild = self.rebuild or (not os.path.exists(output)) or (not os.path.exists(dep))
        if not rebuild:
            lastBuildTime = os.path.getmtime(output);
            with open(dep, "r") as f:
                for line in f:
                    files = string.split(line)
                    for file in files:
                        if file == ":"or file == "\"\":" or file == "\\":
                            continue
                        if os.path.getmtime(file) > lastBuildTime:
                            rebuild = True
                            break
                    if rebuild: break
        if not rebuild:
            return
        sourceDir = os.path.dirname(source);
        cmd = self.compile_cmd + [source] + ["-o"+output] + ["-iquote"+utils.join(s, sourceDir) for s in self.include_dirs] + ["-c","-MMD","-MF",dep,"-MT",'""'] + self.compile_args
        if source in self.file_compile_args:
            cmd += self.file_compile_args[source]
        
        handle.command(cmd)
        ret = utils.exec_cmd(cmd, True, self.concat_cmd, stdout=handle.stdout())
        self.changed = True
        if ret != 0:
            self.failed = True
            handle.failed()
        else:
            handle.success()
    except Exception as ex:
        type, value, trace = sys.exc_info()
        handle.failed(ex, trace)
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
        self.concat_cmd = False

    def build(self):
        self.failed = False
        self.changed = False
        self.buildTime = time.time()
        self.ibuild_dir = self.build_dir
        
        threads = []
        logger = utils.Logger()
        for source in self.input_files:
            sourceDir = os.path.dirname(source)
            outputDir = utils.join(self.ibuild_dir, sourceDir)
            if not os.path.exists(outputDir):
                os.makedirs(outputDir)
            #compile(self, source)
            thread = threading.Thread(target=compile, args=[self, source, logger])
            thread.start()
            threads.append(thread)
            
        while True:
            logger.print_waiting()
            running = False
            for thread in threads:
                if thread.is_alive():
                    running = True
                    break
            if not running: break
                
            
        if (not self.changed) and os.path.exists(self.output_file) and not self.rebuild:
            print "NOTHING TO DO"
            return

        if self.failed:
            print "BUILD FAILED"
            return
            
        output_dir = os.path.dirname(self.output_file)
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
        cmd = self.link_cmd + [utils.join(self.ibuild_dir, s+".o") for s in self.input_files] + ["-l" + s for s in self.libs] + ["-o"+self.output_file] + self.link_args
        buildRet = utils.exec_cmd(cmd, True, self.concat_cmd)

        if buildRet==0:
            elapsed = time.time() - self.buildTime
            print "Build finished in "+str(elapsed)+"s"
        else:
            print str(cmd)
            print "Build failed with "+str(buildRet)
            self.failed = True
