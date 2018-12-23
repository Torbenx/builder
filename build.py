input_files = [
"main.cpp", 
"GL/glwrapper.cpp",
"graphic/Buffer.cpp",
"graphic/CubeMap.cpp",
"graphic/FrameBuffer.cpp",
"graphic/RenderBuffer.cpp",
"graphic/Shader.cpp",
"graphic/Texture.cpp",
"math/Mat4x4.cpp",
"math/Vec2.cpp",
"math/Vec3.cpp",
"math/Vec4.cpp",
"lgraphic/DepthCubeMap.cpp",
"lgraphic/DepthMap.cpp",
"lgraphic/LShader.cpp",
"lgraphic/ShaderBuilder.cpp",
"lgraphic/lighting/PointLighting.cpp",
"lgraphic/lighting/SpotLighting.cpp",
"lgraphic/reflection/Reflection.cpp",
"renderer.cpp",
"GL/glLog.cpp",
"resource/ObjParser.cpp",
]

include_dirs = ["include"]

libs = ["Gdi32", "Opengl32"]

output_file = "main"

args = ["-Wno-attributes", "-Wall"]

build_dir = "build"

preprocess_headers = False


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

global rebuild
rebuild = len(sys.argv)==1 or sys.argv[1] == 'r' or sys.argv[1] == 'p'
global rePreprocess
rePreprocess = len(sys.argv)>1 and sys.argv[1] == 'p'

global failed
failed = False

def build(source):
	output = join(build_dir,source.replace(".cpp",".o"))
	global rebuild
	if (not rebuild) and os.path.exists(output) and os.path.getmtime(output) > os.path.getmtime(source):
		#print "Skipping "+source
		return
	print "Building "+source
	sourceDir = os.path.dirname(source);
	outputDir = join(build_dir,sourceDir)
	if not os.path.exists(outputDir):
		os.makedirs(outputDir)
	cmd = ["g++"] + [source] + ["-o"+output] + (["-iquote"+join(build_dir,s,sourceDir) for s in include_dirs] if preprocess_headers else []) + ["-iquote"+join(s,sourceDir) for s in include_dirs] + ["-c"] + args
	ret = subprocess.call(cmd, stderr=subprocess.STDOUT)
	if ret != 0:
		global failed
		failed = True
		
def preprocess(file, outFile):
	print "Preprocessing "+file
	outDir = os.path.dirname(outFile)
	if not os.path.exists(outDir):
		os.makedirs(outDir)
	ret = subprocess.call(["g++"] + [file] + ["-o"+outFile] + args, stderr=subprocess.STDOUT)
	if ret != 0:
		global failed
		failed = True
		
def scan_include(dir):
	outDir = join(build_dir,dir)
	for item in os.listdir(dir):
		file = join(dir, item)
		outFile = join(outDir,item)+".gch"
		if os.path.isfile(file):
			if (not rePreprocess) and os.path.exists(outFile) and os.path.getmtime(outFile) > os.path.getmtime(file):
				#print "Skipping "+file
				continue
			thread = threading.Thread(target=preprocess, args=[file, outFile])
			thread.start()
			threads.append(thread)
		else:
			scan_include(file)

start_time = time.time()
if preprocess_headers:
	threads = []
	for include_dir in include_dirs:
		scan_include(include_dir)
	
	for thread in threads:
		thread.join()
		
	if failed:
		print "PREPROCESSING FAILED"
		os._exit(1)
			
threads = []

for source in input_files:
	#build(source)
	thread = threading.Thread(target=build, args=[source])
	thread.start()
	threads.append(thread)
	
for thread in threads:
	thread.join()

if failed:
	print "BUILD FAILED"
	os._exit(1)
	
buildRet = subprocess.call(["g++"] + [join(build_dir,s.replace(".cpp",".o")) for s in input_files] + ["-l" + s for s in libs] + ["-o"+output_file] + args, stderr=subprocess.STDOUT)

if buildRet==0:
	elapsed = time.time() - start_time
	print "Build finished in "+str(elapsed)+"s"
	mainRet = subprocess.call(["main.exe"])
	print "Exit with "+str(mainRet)
else:
	print "Build failed with "+str(buildRet)
