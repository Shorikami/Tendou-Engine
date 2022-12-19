#ifndef TENDOUDEVICE_HPP
#define TENDOUDEVICE_HPP

#include "../Core/Window.h"

// std lib headers
#include <string>
#include <vector>

namespace Tendou 
{

    struct SwapChainSupportDetails 
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct QueueFamilyIndices 
    {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;
        bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
    };

    struct FrameBufferAttachment 
    {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
    };

    struct RenderPass
    {
        int32_t width, height;
        VkFramebuffer frameBuffer;
        FrameBufferAttachment color, depth;
        VkRenderPass renderPass;
        VkSampler sampler;
        VkDescriptorImageInfo descriptor;
    };

    class TendouDevice
    {
    public:
#ifdef NDEBUG
        const bool enableValidationLayers = false;
#else
        const bool enableValidationLayers = true;
#endif

        TendouDevice(Window& window);
        ~TendouDevice();

        // Not copyable or movable
        TendouDevice(const TendouDevice&) = delete;
        TendouDevice& operator=(const TendouDevice&) = delete;
        TendouDevice(TendouDevice&&) = delete;
        TendouDevice& operator=(TendouDevice&&) = delete;

        VkInstance GetInstance() { return instance; }
        VkCommandPool GetCommandPool() { return commandPool; }
        VkDevice Device() { return device_; }
        VkPhysicalDevice PhysicalDevice() { return physicalDevice; }
        VkSurfaceKHR Surface() { return surface_; }
        VkQueue GraphicsQueue() { return graphicsQueue_; }
        VkQueue PresentQueue() { return presentQueue_; }

        SwapChainSupportDetails GetSwapChainSupport() { return QuerySwapChainSupport(physicalDevice); }
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(physicalDevice); }
        VkFormat FindSupportedFormat(
            const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        // Buffer Helper Functions
        void CreateBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer& buffer,
            VkDeviceMemory& bufferMemory);
        VkCommandBuffer BeginSingleTimeCommands();
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void CopyBufferToImage(
            VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount = 1);

        // Render Pass Helper Functions
        RenderPass CreateRenderPass(int width, int height);

        // Texture/Image Helper Functions
        VkImageView CreateImageView(VkImage image, VkFormat format, 
            uint32_t layerCount = 1, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
            VkImageAspectFlagBits aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);
        void CreateImage(uint32_t width, uint32_t height,
            VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties, VkImage& image,
            VkDeviceMemory& imageMemory, uint32_t layerCount = 1);
        void CreateImageWithInfo(
            const VkImageCreateInfo& imageInfo,
            VkMemoryPropertyFlags properties,
            VkImage& image,
            VkDeviceMemory& imageMemory);
        void TransitionImageLayout(VkImage image, VkFormat format,
            VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount = 1);

        VkPhysicalDeviceProperties properties;

    private:
        void CreateInstance();
        void SetupDebugMessenger();
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateCommandPool();

        // helper functions
        bool IsDeviceSuitable(VkPhysicalDevice device);
        std::vector<const char*> GetRequiredExtensions();
        bool CheckValidationLayerSupport();
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
        void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        void HasGflwRequiredInstanceExtensions();
        bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        Window& window;
        VkCommandPool commandPool;

        VkDevice device_;
        VkSurfaceKHR surface_;
        VkQueue graphicsQueue_;
        VkQueue presentQueue_;

        const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
        const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        friend class Application;
        friend class Editor;
    };

}

#endif