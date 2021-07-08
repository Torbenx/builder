#pragma once

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"

#define VULKAN_VERIFY(input)                                                                                    \
    {                                                                                                           \
        vk::Result _result = input;                                                                             \
        if (_result != vk::Result::eSuccess) {                                                                  \
            LOGD("VULKAN_VERIFY failed: %s:%d: %s(): " #input " -> %d", __FILE__, __LINE__, __func__, _result); \
        }                                                                                                       \
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
