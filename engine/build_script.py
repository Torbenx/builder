import os
import msvcrt
import subprocess
import build
import sys

class Builder(build.Builder):
	base = "engine"

	input_files = [
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

	include_dirs = ["include"]

	libs = ["Gdi32", "Opengl32"]

	compile_args = ["-Wno-attributes", "-Wall", "-fPIC"]

	link_args = ["-shared"]