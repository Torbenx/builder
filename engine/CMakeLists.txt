cmake_minimum_required(VERSION 3.16)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

   set(platform_source
        "windows/glwrapper.cpp"
        "windows/engine.cpp"
        "windows/fullscreen.cpp"
        "windows/Renderer.cpp"
        "windows/text.cpp"
        "windows/keycodes.cpp"
        "resource/asset_istream.cpp"
    )
    set(platform_libs Gdi32 Opengl32 D3D11 D2D1 Dwrite)
    set_source_files_properties("windows/glwrapper.cpp" PROPERTIES COMPILE_OPTIONS "-Wno-cast-function-type;-Wno-unknown-warning-option")

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")

    set(platform_source
        "android/engine.cpp"
        "android/assetbuf.cpp"
        "android/fullscreen.cpp"
        "android/Renderer.cpp"
        "android/android_jni.cpp"
        "android/text.cpp"
        "android/keycodes.cpp"
        "android/glwrapper.cpp"
    )
    set(platform_libs android log EGL GLESv3)
    set_source_files_properties("android/glwrapper.cpp" PROPERTIES COMPILE_OPTIONS "-Wno-cast-function-type;-Wno-unknown-warning-option")

else()
    message(FATAL_ERROR "Unsupported system: " ${CMAKE_SYSTEM_NAME})
endif()

set(vulkan_source)
if(${Vulkan_FOUND})
    set(vulkan_source
        "VK/VKmain.cpp"
        "VK/vk_mem_alloc.cpp"
    )
    set_source_files_properties("VK/vk_mem_alloc.cpp" PROPERTIES COMPILE_OPTIONS "-Wno-all;-Wno-extra;-Wno-suggest-override")
endif()

set(linkage STATIC)
if(ENGINE_LINK_DYNAMIC)
    set(linkage SHARED)
endif()

add_library(engine ${linkage}
    "GL/CubeMap.cpp"
    "GL/Framebuffer.cpp"
    "GL/Shader.cpp"
    "GL/Texture.cpp"
    "math/Mat4x4.cpp"
    "math/Mat3x3.cpp"
    "math/Rect.cpp"
    "math/Box.cpp"
    "resource/ObjParser.cpp"
    "resource/JpegDecoder.cpp"
    "util/time.cpp"
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
    "3d/Effect.cpp"
    "3d/PointLight.cpp"
    "3d/SpotLight.cpp"
    "3d/Reflection.cpp"
    "3d/CommonEffects.cpp"
    "3d/Shaderpass.cpp"
    "3d/Renderpass.cpp"
    "events/ExecutionContext.cpp"
    "containers/RangeAllocator.cpp"
    "graphics/GAllocator.cpp"
    ${platform_source}
    ${vulkan_source}
)

set_source_files_properties("events/ExecutionContext.cpp" PROPERTIES COMPILE_OPTIONS "-fomit-frame-pointer")

#set_target_properties(engine PROPERTIES UNITY_BUILD ON)
target_compile_options(engine PRIVATE "-Wall;-Wextra;-Wno-invalid-offsetof;-Wsuggest-override")
target_link_libraries(engine PUBLIC ${platform_libs} turbojpeg ${CMAKE_THREAD_LIBS_INIT} ${Vulkan_LIBRARIES} fmt)
