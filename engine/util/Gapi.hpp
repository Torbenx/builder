#pragma once

template<typename T>
concept Gapi = true;

struct Vulkan { };
struct OpenGL { };

#define ENUMERATE_G_APIS(o) o(OpenGL) o(Vulkan)

#define INSTANTIATE_G_API_CLASS(clz) template class clz<OpenGL>; template class clz<Vulkan>;
