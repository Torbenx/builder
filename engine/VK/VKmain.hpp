#pragma once

#include "util/log.hpp"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan.hpp>

namespace Tag {
struct VK {
    static constexpr char tag[] = "vk";
};
}

#define VULKAN_VERIFY(input)                                                                                            \
    {                                                                                                                   \
        vk::Result _result = input;                                                                                     \
        if (_result != vk::Result::eSuccess) {                                                                          \
            log<Tag::VK>("VULKAN_VERIFY failed: {}:{}: {}(): " #input " -> {}", __FILE__, __LINE__, __func__, _result); \
        }                                                                                                               \
    }

namespace VulkanState {

extern vk::Instance instance;
extern vk::SurfaceKHR surface;
extern vk::PhysicalDevice physicalDevice;
extern vk::Device device;
extern vk::SwapchainKHR swapchain;
extern VmaAllocator allocator;
extern uint32_t queueFamilyIndex;

void initialize(const char* surfaceExtensionName);
void initializeSurface(vk::SurfaceKHR);
void updateSurface(vk::SurfaceKHR);

};
