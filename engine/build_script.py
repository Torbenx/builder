import msvcrt
import os
import subprocess
import build
import sys

b = build.Builder()

b.input_files = [
"engine.cpp",
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
"GL/glLog.cpp",
"resource/ObjParser.cpp",
]

b.output_file = "engine.dll"
if len(sys.argv) > 1:
	b.output_file = sys.argv[1]

b.include_dirs = ["include"]

b.libs = ["Gdi32", "Opengl32"]

b.compile_args = ["-Wno-attributes", "-Wall", "-fPIC"]

b.link_args = ["-shared"]

b.rebuild = True
if len(sys.argv) > 2 and sys.argv[2] == "0":
	b.rebuild = False

b.repreprocess = True
if len(sys.argv) > 3 and sys.argv[3] == "0":
	b.repreprocess = False
	
b.build()

sys.exit(1 if b.failed else 0)