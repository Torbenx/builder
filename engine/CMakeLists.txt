cmake_minimum_required(VERSION 3.13)


if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

   set(platform_source
        "windows/glwrapper.cpp"
        "windows/engine.cpp"
        "windows/fullscreen.cpp"
        "windows/Renderer.cpp"
        "windows/text.cpp"
        "windows/keycodes.cpp"
    )
    set(platform_libs Gdi32 Opengl32 D3D11 D2D1 Dwrite)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")

    set(platform_source
        "android/engine.cpp"
        "android/assetbuf.cpp"
        "android/fullscreen.cpp"
        "android/Renderer.cpp"
        "android/android_jni.cpp"
        "android/text.cpp"
        "android/keycodes.cpp"
    )
    set(platform_libs android log EGL GLESv2)

else()
    message(FATAL_ERROR "Unsupported system: " ${CMAKE_SYSTEM_NAME})
endif()


add_library(engine SHARED
    "graphic/CubeMap.cpp"
    "graphic/Framebuffer.cpp"
    "graphic/Renderbuffer.cpp"
    "graphic/Shader.cpp"
    "graphic/Texture.cpp"
    "math/Mat4x4.cpp"
    "math/Mat3x3.cpp"
    "math/Vec2.cpp"
    "math/Vec3.cpp"
    "math/Vec4.cpp"
    "math/Rect.cpp"
    "lgraphic/DepthCubeMap.cpp"
    "lgraphic/DepthMap.cpp"
    "lgraphic/LShader.cpp"
    "lgraphic/light/PointLight.cpp"
    "lgraphic/light/SpotLight.cpp"
    "lgraphic/reflection/Reflection.cpp"
    "gl/glLog.cpp"
    "resource/ObjParser.cpp"
    "resource/asset_istream.cpp"
    "util/time.cpp"
    "util/target.cpp"
    #"util/draw_util.cpp"
    "input/Pointer.cpp"
    "input/Look3d.cpp"
    #"ui/TestComponent.cpp"
    #"ui/TextureComponent.cpp"
    #"ui/TextComponent.cpp"
    #"ui/DepthMapComponent.cpp"
    #"ui/CubeMapComponent.cpp"
    #"ui/DepthCubeMapComponent.cpp"
    "ui/TypedSize.cpp"
    "ui/UiComponent.cpp"
    "ui/UiScene.cpp"
    "ui/TypedPoint.cpp"
    "ui/UiInput.cpp"
    "ui/UiSceneComponent.cpp"
    "ui/UiSolver.cpp"
    "3d/Scene.cpp"
    ${platform_source}
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_link_options(engine PUBLIC "-static-libstdc++")
endif()

target_link_libraries(engine ${platform_libs} turbojpeg)
