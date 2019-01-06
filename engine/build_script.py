import os
from build_lib import build

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
            "math/Mat3x3.cpp",
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
            "gl/glLog.cpp",
            "resource/ObjParser.cpp",
            "resource/asset_istream.cpp",
            "util/time.cpp",
            "util/target.cpp",
            "input/Pointer.cpp",
            "input/Look3d.cpp",
            "ui/Layout.cpp",
            "ui/TestComponent.cpp",
        ]

        self.compile_args += ["-Wno-attributes", "-Wall", "-fPIC"]

        self.link_args += ["-shared"]
        
        if target == "windows":
            self.libs += ["Gdi32", "Opengl32"]
            self.input_files += [
                "windows/glwrapper.cpp",
                "windows/engine.cpp",
                "windows/fullscreen.cpp",
                "windows/Renderer.cpp",
            ]
        if target == "android":
            self.libs += ["android", "log", "GLESv2"]
            self.input_files += [
                "android/engine.cpp",
                "android/assetbuf.cpp",
                "android/fullscreen.cpp",
                "android/Renderer.cpp",
                "android/android_jni.cpp",
            ]
    
    def build(self):
        oldDir = os.getcwd()
        os.chdir(os.path.dirname(__file__))
        super(Builder, self).build()
        os.chdir(oldDir)