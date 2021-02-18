import os
from build_lib import c, android

class Builder(c.Builder):

    def __init__(self, target):
        super(Builder, self).__init__()
        
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
            "math/Rect.cpp",
            "lgraphic/DepthCubeMap.cpp",
            "lgraphic/DepthMap.cpp",
            "lgraphic/LShader.cpp",
            "lgraphic/ShaderBuilder.cpp",
            "lgraphic/light/PointLight.cpp",
            "lgraphic/light/SpotLight.cpp",
            "lgraphic/reflection/Reflection.cpp",
            "gl/glLog.cpp",
            "resource/ObjParser.cpp",
            "resource/asset_istream.cpp",
            "util/time.cpp",
            "util/target.cpp",
            "util/draw_util.cpp",
            "input/Pointer.cpp",
            "input/Look3d.cpp",
            "ui/TestComponent.cpp",
            "ui/TextureComponent.cpp",
            "ui/TextComponent.cpp",
            "ui/DepthMapComponent.cpp",
            "ui/CubeMapComponent.cpp",
            "ui/DepthCubeMapComponent.cpp",
            "ui/TypedSize.cpp",
            "ui/UiComponent.cpp",
            "ui/UiScene.cpp",
            "ui/TypedPoint.cpp",
            "ui/UiInput.cpp",
            "ui/UiSceneComponent.cpp",
            "3d/Scene.cpp"
        ]

        self.compile_args += ["-Wno-attributes", "-Wall", "-fPIC"]

        self.link_args += ["-shared"]
        
        if target == "windows":
            self.libs += ["Gdi32", "Opengl32", "D3D11", "D2D1", "Dwrite"]
            self.input_files += [
                "windows/glwrapper.cpp",
                "windows/engine.cpp",
                "windows/fullscreen.cpp",
                "windows/Renderer.cpp",
                "windows/text.cpp",
                "windows/keycodes.cpp",
            ]
        if target == "android":
            self.libs += ["android", "log", "GLESv2", "EGL"]
            self.input_files += [
                "android/engine.cpp",
                "android/assetbuf.cpp",
                "android/fullscreen.cpp",
                "android/Renderer.cpp",
                "android/android_jni.cpp",
                "android/text.cpp",
                "android/keycodes.cpp",
            ]
    
    def build(self):
        oldDir = os.getcwd()
        if os.path.dirname(__file__): os.chdir(os.path.dirname(__file__))
        super(Builder, self).build()
        os.chdir(oldDir)

class App(android.Builder):

    def __init__(self):
        super(App, self).__init__()
        
        self.min_api = 21
        self.api = 27
        self.java_dir = "android/java"
        self.java_source = ["com/tt/engine/MainActivity.java", "com/tt/engine/Text.java"]
        self.android_manifest = "android/AndroidManifest.xml"
        self.version_code = 1
        self.version_name = "1.0"
        
    def build(self):
        oldDir = os.getcwd()
        if os.path.dirname(__file__): os.chdir(os.path.dirname(__file__))
        super(App, self).build()
        os.chdir(oldDir)
        