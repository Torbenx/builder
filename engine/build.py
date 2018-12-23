import os
import subprocess
import msvcrt
import time
import threading
import sys

def join(*paths):
	ret = ""
	for path in paths:
		if path == "": continue
		if ret != "": ret += "/"
		ret += path
	return ret

def compile(self, source):
	output = join(self.base, self.build_dir, source+".o")
	if (not self.rebuild) and os.path.exists(output) and os.path.getmtime(output) > os.path.getmtime(join(self.base, source)):
		#print "Skipping "+source
		return
	print "Building "+source
	sourceDir = os.path.dirname(source);
	cmd = ["g++"] + [join(self.base, source)] + ["-o"+output] + ["-iquote"+join(self.base, s, sourceDir) for s in self.include_dirs] + ["-c"] + self.compile_args
	ret = subprocess.call(cmd, stderr=subprocess.STDOUT)
	if ret != 0:
		self.failed = True

class Builder:
	base = ""
	input_files = []
	output_file = "a.exe"
	include_dirs = []
	libs = []
	compile_args = []
	link_args = []
	build_dir = "build"
	rebuild = False

	def build(self):
		self.failed = False
		start_time = time.time()
		
		threads = []
		for source in self.input_files:
			sourceDir = os.path.dirname(source);
			outputDir = join(self.base, self.build_dir, sourceDir)
			if not os.path.exists(outputDir):
				os.makedirs(outputDir)
			#build(source)
			thread = threading.Thread(target=compile, args=[self, source])
			thread.start()
			threads.append(thread)
			
		for thread in threads:
			thread.join()

		if self.failed:
			print "BUILD FAILED"
			return
			
		buildRet = subprocess.call(["g++"] + [join(self.base, self.build_dir, s+".o") for s in self.input_files] + ["-l" + s for s in self.libs] + ["-o"+join(self.base, self.output_file)] + self.link_args, stderr=subprocess.STDOUT)

		if buildRet==0:
			elapsed = time.time() - start_time
			print "Build finished in "+str(elapsed)+"s"
		else:
			print "Build failed with "+str(buildRet)
			self.failed = True
