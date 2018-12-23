import subprocess
import msvcrt
import os
import time
import threading
import sys

def join(*paths):
	ret = ""
	for path in paths:
		if ret != "": ret += "/"
		ret += path
	return ret

def compile(self,source):
	output = join(self.build_dir,source+".o")
	if (not self.rebuild) and os.path.exists(output) and os.path.getmtime(output) > os.path.getmtime(source):
		#print "Skipping "+source
		return
	print "Building "+source
	sourceDir = os.path.dirname(source);
	outputDir = join(self.build_dir,sourceDir)
	if not os.path.exists(outputDir):
		os.makedirs(outputDir)
	cmd = ["g++"] + [source] + ["-o"+output] + \
		(["-iquote"+join(self.build_dir,s,sourceDir) for s in self.include_dirs] if self.preprocess_headers else []) + \
		["-iquote"+join(s,sourceDir) for s in self.include_dirs] + ["-c"] + self.compile_args
	ret = subprocess.call(cmd, stderr=subprocess.STDOUT)
	if ret != 0:
		self.failed = True
		
def preprocess(self, file, outFile):
	print "Preprocessing "+file
	outDir = os.path.dirname(outFile)
	if not os.path.exists(outDir):
		os.makedirs(outDir)
	ret = subprocess.call(["g++"] + [file] + ["-o"+outFile] + self.compile_args, stderr=subprocess.STDOUT)
	if ret != 0:
		self.failed = True

class Builder:
	input_files = []
	output_file = "a.exe"
	include_dirs = []
	libs = []
	compile_args = []
	link_args = []
	build_dir = "build"
	rebuild = False
	repreprocess = False
	preprocess_headers = False
	threads = []
			
	def scan_include(self, dir):
		outDir = join(self.build_dir,dir)
		for item in os.listdir(dir):
			file = join(dir, item)
			outFile = join(outDir,item)+".gch"
			if os.path.isfile(file):
				if (not self.repreprocess) and os.path.exists(outFile) and os.path.getmtime(outFile) > os.path.getmtime(file):
					#print "Skipping "+file
					continue
				thread = threading.Thread(target=preprocess, args=[self, file, outFile])
				thread.start()
				self.threads.append(thread)
			else:
				self.scan_include(file)

	def build(self):
		self.failed = False
		start_time = time.time()
		if self.preprocess_headers:
			self.threads = []
			for include_dir in self.include_dirs:
				self.scan_include(include_dir)
			
			for thread in self.threads:
				thread.join()
				
			if self.failed:
				print "PREPROCESSING FAILED"
				os._exit(1)
					
		self.threads = []
		for source in self.input_files:
			#build(source)
			thread = threading.Thread(target=compile, args=[self, source])
			thread.start()
			self.threads.append(thread)
			
		for thread in self.threads:
			thread.join()

		if self.failed:
			print "BUILD FAILED"
			os._exit(1)
			
		buildRet = subprocess.call(["g++"] + [join(self.build_dir,s+".o") for s in self.input_files] + ["-l" + s for s in self.libs] + ["-o"+self.output_file] + self.link_args, stderr=subprocess.STDOUT)

		if buildRet==0:
			elapsed = time.time() - start_time
			print "Build finished in "+str(elapsed)+"s"
		else:
			print "Build failed with "+str(buildRet)
			self.failed = True
