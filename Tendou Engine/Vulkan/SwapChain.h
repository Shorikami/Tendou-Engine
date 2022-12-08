#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include "TendouDevice.h"

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <memory>
#include <string>
#include <vector>

namespace Tendou
{

    class SwapChain 
    {
    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 20;

        SwapChain(TendouDevice& deviceRef, VkExtent2D windowExtent);
        SwapChain(TendouDevice& deviceRef, VkExtent2D windowExtent, std::shared_ptr<SwapChain> prev);
        ~SwapChain();

        SwapChain(const SwapChain&) = delete;
        SwapChain& operator=(const SwapChain&) = delete;

        __inline VkFramebuffer GetFrameBuffer(int index) { return swapChainFramebuffers[index]; }
        __inline VkRenderPass GetRenderPass() { return renderPass; }
        __inline VkImageView GetImageView(int index) { return swapChainImageViews[index]; }
        __inline size_t ImageCount() { return swapChainImages.size(); }
        __inline VkFormat GetSwapChainImageFormat() { return swapChainImageFormat; }
        __inline VkExtent2D GetSwapChainExtent() { return swapChainExtent; }
        __inline uint32_t Width() { return swapChainExtent.width; }
        __inline uint32_t Height() { return swapChainExtent.height; }

        float ExtentAspectRatio() 
        {
            return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        }
        VkFormat FindDepthFormat();

        VkResult AcquireNextImage(uint32_t* imageIndex);
        VkResult SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

        bool CompareSwapFormats(const SwapChain& swapChain) const
        {
            return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
                swapChain.swapChainImageFormat == swapChainImageFormat;
        }

    private:
        void Init();
        void CreateSwapChain();
        void CreateImageViews();
        void CreateDepthResources();
        void CreateRenderPass();
        void CreateFramebuffers();
        void CreateSyncObjects();

        // Helper functions
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(
            const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        VkFormat swapChainImageFormat;
        VkFormat swapChainDepthFormat;
        VkExtent2D swapChainExtent;

        std::vector<VkFramebuffer> swapChainFramebuffers;
        VkRenderPass renderPass;

        std::vector<VkImage> depthImages;
        std::vector<VkDeviceMemory> depthImageMemorys;
        std::vector<VkImageView> depthImageViews;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;

        TendouDevice& device;
        VkExtent2D windowExtent;

        VkSwapchainKHR swapChain;
        std::shared_ptr<SwapChain> oldSwapChain;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
        size_t currentFrame = 0;
    };

}  // namespace lve

#endif