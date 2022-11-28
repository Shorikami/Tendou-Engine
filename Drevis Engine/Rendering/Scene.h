#ifndef SCENE_H
#define SCENE_H

#include "../Core/Window.h"
#include "../Vulkan/SwapChain.h"
#include "../Vulkan/DrevisDevice.h"
#include "../Rendering/Camera.h"

#include <cassert>
#include <memory>
#include <vector>

namespace Drevis
{
	class Scene
	{
	public:
		Scene(Window& window, DrevisDevice& device);
		~Scene();

		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		__inline bool IsFrameInProgress() const { return isFrameStarted; }
		__inline VkRenderPass GetSwapChainRenderPass() const { return swapChain->GetRenderPass(); }
		float GetAspectRatio() const { return swapChain->ExtentAspectRatio(); }

		VkCommandBuffer GetCurrentCommandBuffer() const 
		{ 
			assert(isFrameStarted && "Cannot get command buffer when frame is not in progress!");
			return commandBuffers[currFrameIdx];
		}

		int GetFrameIndex() const
		{
			assert(isFrameStarted && "Cannot get frame index when frame is not in progress!");
			return currFrameIdx;
		}

		VkCommandBuffer BeginFrame();
		void EndFrame();

		void ProcessInput(float dt, Camera& c);
		void ProcessMouse(float x, float y, Camera& c);

		void BeginSwapChainRenderPass(VkCommandBuffer cmdBuf);
		void EndSwapChainRenderPass(VkCommandBuffer cmdBuf);

	private:
		void CreateCommandBuffers();
		void FreeCommandBuffers();
		void RecreateSwapChain();

		Window& appWindow;
		DrevisDevice& device;

		std::unique_ptr<SwapChain> swapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currImageIdx;
		int currFrameIdx = 0;
		bool isFrameStarted = false;

		friend class Application;
	};
}

#endif