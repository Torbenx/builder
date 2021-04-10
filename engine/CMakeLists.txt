cmake_minimum_required(VERSION 3.16)

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

set(linkage SHARED)
if(ENGINE_LINK_STATIC)
    set(linkage STATIC)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer")

add_library(engine ${linkage}
    "GL/CubeMap.cpp"
    "GL/Framebuffer.cpp"
    "GL/Shader.cpp"
    "GL/Texture.cpp"
    "math/Mat4x4.cpp"
    "math/Mat3x3.cpp"
    "math/Vec2.cpp"
    "math/Vec3.cpp"
    "math/Vec4.cpp"
    "math/Rect.cpp"
    "resource/ObjParser.cpp"
    "resource/asset_istream.cpp"
    "util/time.cpp"
    "util/target.cpp"
    "util/Shaders.cpp"
    #"util/draw_util.cpp"
    "input/Pointer.cpp"
    "input/Look3d.cpp"
    "ui/TestComponent.cpp"
    "ui/TextureComponent.cpp"
    "ui/TextComponent.cpp"
    "ui/DepthMapComponent.cpp"
    "ui/CubeMapComponent.cpp"
    "ui/DepthCubeMapComponent.cpp"
    "ui/TypedSize.cpp"
    "ui/UiComponent.cpp"
    "ui/UiScene.cpp"
    "ui/TypedPoint.cpp"
    "ui/UiInput.cpp"
    "ui/UiSceneComponent.cpp"
    "ui/UiSolver.cpp"
    "3d/Scene.cpp"
    "3d/Effect.cpp"
    "3d/PointLight.cpp"
    "3d/SpotLight.cpp"
    "3d/Reflection.cpp"
    "events/Scheduler.cpp"
    ${platform_source}
)

set_target_properties(engine PROPERTIES UNITY_BUILD ON)
target_link_libraries(engine PUBLIC ${platform_libs} turbojpeg)
