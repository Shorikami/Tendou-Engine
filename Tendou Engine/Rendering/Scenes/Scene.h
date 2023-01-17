#ifndef SCENE_H
#define SCENE_H

#include "../../Core/Window.h"
#include "../../Vulkan/SwapChain.h"
#include "../../Vulkan/TendouDevice.h"
#include "../../Vulkan/Descriptor.h"

#include "../../Vulkan/Systems/Default.h"
#include "../../Vulkan/Systems/Offscreen.h"
#include "../../Vulkan/Systems/Deferred.h"

#include "../../Rendering/Camera.h"

#include "../../Components/GameObject.h"

// Include ImGUI
// It's here because all scenes will have independent
// editor options
// TODO: When you make it closer to an actual editor,
// have scenes pass variables to the editor so these
// include files can be moved only to the editor
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <cassert>
#include <memory>
#include <vector>
#include <stdexcept>

namespace Tendou
{
	class Scene
	{
	public:
		Scene(Window& window, TendouDevice& device);
		virtual ~Scene();

		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		virtual int Init();
		virtual int PreUpdate();
		virtual int Update();
		virtual int PostUpdate();

		virtual int Render(VkCommandBuffer buf, FrameInfo& f);

		__inline bool IsFrameInProgress() const { return isFrameStarted; }
		__inline VkRenderPass GetSwapChainRenderPass() const { return swapChain->GetRenderPass(); }
		__inline SwapChain* GetSwapChain()  { return swapChain.get(); }
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

		DescriptorPool* GetGlobalPool() { return globalPool.get(); }
		DescriptorSetLayout* GetSetLayout(std::string key) { return setLayouts[key].get(); }

		GameObject::Map& GetGameObjects() { return gameObjects; }
		Camera& GetCamera() { return c; }

		std::vector<VkDescriptorSet> GetDescriptorSet(std::string key) { return descriptorSets[key]; }

		VkDescriptorSet GetDescriptorSet(int idx, std::string key)
		{
			if (idx >= descriptorSets[key].size())
			{
				return nullptr;
			}
			return descriptorSets[key][idx];
		}

		VkCommandBuffer BeginFrame();
		void EndFrame();

		void ProcessInput(float dt, Camera& c);
		void ProcessMouse(float x, float y, Camera& c);

		void BeginRenderPass(VkCommandBuffer cmdBuf, std::string key);
		void EndRenderPass(VkCommandBuffer cmdBuf);

		void BeginSwapChainRenderPass(VkCommandBuffer cmdBuf);
		void EndSwapChainRenderPass(VkCommandBuffer cmdBuf);

	protected:
		void CreateCommandBuffers();
		void FreeCommandBuffers();
		void RecreateSwapChain();

		Window& appWindow;
		TendouDevice& device;

		std::unordered_map<std::string, std::vector<std::unique_ptr<RenderSystem>>> renderSystems;
		std::unordered_map<std::string, Tendou::RenderPass> renderPasses;

		std::unique_ptr<SwapChain> swapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currImageIdx;
		int currFrameIdx = 0;
		bool isFrameStarted = false;

		std::unordered_map<std::string, std::unique_ptr<DescriptorSetLayout>> setLayouts;
		std::unordered_map<std::string, std::vector<VkDescriptorSet>> descriptorSets;

		// Note: order of declarations matters - need the global pool to be destroyed
		// before the device
		std::unique_ptr<DescriptorPool> globalPool{};
		GameObject::Map gameObjects;
		Camera c;

		friend class Application;
		friend class Editor;
	};
}

#endif