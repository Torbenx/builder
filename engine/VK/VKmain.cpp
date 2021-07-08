#define VMA_IMPLEMENTATION
#include "VK/VKmain.hpp"

namespace VulkanState {

vk::Instance instance;
vk::SurfaceKHR surface;
vk::PhysicalDevice physicalDevice;
vk::Device device;
vk::SwapchainKHR swapchain;
VmaAllocator allocator;
uint32_t queueFamilyIndex = std::numeric_limits<uint32_t>::max();

void initialize(const char* surfaceExtensionName) {
    auto appInfo = vk::ApplicationInfo()
                       .setPApplicationName("Engine")
                       .setApplicationVersion(0)
                       .setPEngineName("Engine")
                       .setEngineVersion(0)
                       .setApiVersion(VK_API_VERSION_1_2);

    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
    const char* extensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, surfaceExtensionName };
    auto instanceInfo = vk::InstanceCreateInfo()
                            .setPApplicationInfo(&appInfo)
                            .setEnabledLayerCount(1)
                            .setPpEnabledLayerNames(layers)
                            .setEnabledExtensionCount(2)
                            .setPpEnabledExtensionNames(extensions);

    VULKAN_VERIFY(vk::createInstance(&instanceInfo, nullptr, &instance));

    uint32_t count = 0;
    VULKAN_VERIFY(instance.enumeratePhysicalDevices(&count, nullptr));
    if (count != 1) {
        LOGD("count is not 1");
    }
    VULKAN_VERIFY(instance.enumeratePhysicalDevices(&count, &physicalDevice));
    auto properties = physicalDevice.getProperties();
    LOGD("physicalDevice: %s, type: %s", properties.deviceName.data(), to_string(properties.deviceType).c_str());
}

void initializeSurface(vk::SurfaceKHR newSurface) {
    surface = newSurface;
    uint32_t familyCount = 0;
    physicalDevice.getQueueFamilyProperties(&familyCount, nullptr);
    vk::QueueFamilyProperties familyProperties[familyCount];
    physicalDevice.getQueueFamilyProperties(&familyCount, familyProperties);

    for (queueFamilyIndex = 0; queueFamilyIndex < familyCount; queueFamilyIndex++) {
        vk::Bool32 supportsPresent = VK_FALSE;
        VULKAN_VERIFY(physicalDevice.getSurfaceSupportKHR(queueFamilyIndex, surface, &supportsPresent));

        if (supportsPresent && (familyProperties[queueFamilyIndex].queueFlags & vk::QueueFlagBits::eGraphics)) {
            break;
        }
    }
    if (queueFamilyIndex == familyCount) {
        LOGD("no supported queue family");
        return;
    }

    float priorities[] = { 0.0f };
    auto queueInfo = vk::DeviceQueueCreateInfo()
                         .setQueueFamilyIndex(queueFamilyIndex)
                         .setQueueCount(1)
                         .setPQueuePriorities(priorities);

    const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    auto deviceInfo = vk::DeviceCreateInfo()
                          .setQueueCreateInfoCount(1)
                          .setPQueueCreateInfos(&queueInfo)
                          .setEnabledLayerCount(0)
                          .setPpEnabledLayerNames(nullptr)
                          .setEnabledExtensionCount(1)
                          .setPpEnabledExtensionNames(extensions)
                          .setPEnabledFeatures(nullptr);

    VULKAN_VERIFY(physicalDevice.createDevice(&deviceInfo, nullptr, &device));

    vk::Queue queue;
    device.getQueue(queueFamilyIndex, 0, &queue);

    VmaAllocatorCreateInfo allocInfo = {};
    allocInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocInfo.instance = instance;
    allocInfo.physicalDevice = physicalDevice;
    allocInfo.device = device;

    VULKAN_VERIFY((vk::Result)vmaCreateAllocator(&allocInfo, &allocator));
}

void updateSurface(vk::SurfaceKHR newSurface) {
    vkDestroySurfaceKHR(instance, surface, nullptr);
    surface = newSurface;

    vk::Bool32 supportsPresent = VK_FALSE;
    VULKAN_VERIFY(physicalDevice.getSurfaceSupportKHR(queueFamilyIndex, surface, &supportsPresent));
    if (!supportsPresent) {
        LOGD("queue can't present on current surface");
        return;
    }

    vk::SurfaceCapabilitiesKHR capabilities;
    vk::Result result = physicalDevice.getSurfaceCapabilitiesKHR(surface, &capabilities);
    if (result != vk::Result::eSuccess) {
        LOGD("surface might be invalid");
        VULKAN_VERIFY(result);
        return;
    }
    LOGD("surface:");
    LOGD("  extent: %d x %d", capabilities.currentExtent.width, capabilities.currentExtent.height);
    LOGD("  imageCount: %d - %d", capabilities.minImageCount, capabilities.maxImageCount);
    LOGD("  transform: 0x%x", capabilities.currentTransform);

    uint32_t targetImageCount = 2;
    if (capabilities.minImageCount > targetImageCount || (capabilities.maxImageCount > 0 && capabilities.maxImageCount < targetImageCount)) {
        LOGD("target image count of %u not supported", targetImageCount);
        return;
    }

    uint32_t formatCount = 0;
    VULKAN_VERIFY(physicalDevice.getSurfaceFormatsKHR(surface, &formatCount, nullptr));
    vk::SurfaceFormatKHR formats[formatCount];
    VULKAN_VERIFY(physicalDevice.getSurfaceFormatsKHR(surface, &formatCount, formats));
    LOGD("  formats: %u", formatCount);

    vk::Format targetFormat = vk::Format::eB8G8R8A8Unorm;
    vk::ColorSpaceKHR colorSpace;
    if (formatCount == 1 && formats[0].format == vk::Format::eUndefined) {
        colorSpace = formats[0].colorSpace;
    } else {
        uint32_t i;
        for (i = 0; i < formatCount; i++) {
            if (formats[i].format == targetFormat) {
                colorSpace = formats[i].colorSpace;
                break;
            }
        }
        if (i == formatCount) {
            LOGD("target format not supported");
            return;
        }
    }

    auto swapchainInfo = vk::SwapchainCreateInfoKHR()
                             .setSurface(surface)
                             .setMinImageCount(targetImageCount)
                             .setImageFormat(targetFormat)
                             .setImageColorSpace(colorSpace)
                             .setImageExtent(capabilities.currentExtent)
                             .setImageArrayLayers(1)
                             .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                             .setImageSharingMode(vk::SharingMode::eExclusive)
                             .setQueueFamilyIndexCount(0)
                             .setPQueueFamilyIndices(nullptr)
                             .setPreTransform(capabilities.currentTransform)
                             .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                             .setPresentMode(vk::PresentModeKHR::eFifo)
                             .setClipped(true)
                             .setOldSwapchain(swapchain);

     vk::SwapchainKHR newSwapchain;
     VULKAN_VERIFY(device.createSwapchainKHR(&swapchainInfo, nullptr, &newSwapchain));

     if (swapchain) {
         device.destroySwapchainKHR(swapchain, nullptr);
     }
     swapchain = newSwapchain;


}

}
