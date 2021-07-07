#pragma once

template<typename T>
concept Gapi = true;

struct VK { };
struct GL { };

#define ENUMERATE_G_APIS(o) o(GL) o(VK)

#define INSTANTIATE_G_API_CLASS(clz) template class clz<GL>; template class clz<VK>;
