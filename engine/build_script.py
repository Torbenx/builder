import os
import build

class Builder(build.Builder):

	def __init__(self, target):
		super(Builder, self).__init__(target)
		
		self.input_files += [
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

		self.include_dirs += ["include"]

		self.compile_args += ["-Wno-attributes", "-Wall", "-fPIC"]

		self.link_args += ["-shared"]
		
		if target == "windows":
			self.libs += ["Gdi32", "Opengl32"]
			self.input_files += ["windows/glwrapper.cpp", "windows/engine.cpp"]
		if target == "android":
			self.libs += ["android", "log", "GLESv2"]
	
	def build(self):
		oldDir = os.getcwd()
		os.chdir(os.path.dirname(__file__))
		super(Builder, self).build()
		os.chdir(oldDir)