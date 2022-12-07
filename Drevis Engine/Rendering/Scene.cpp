#include "Scene.h"
#include "../Core/Application.h"

#include "../IO/Mouse.h"
#include "../IO/Keyboard.h"

#include <stdexcept>
#include <array>
#include <cassert>

namespace Drevis
{
	Scene::Scene(Window& window, DrevisDevice& device_)
		: appWindow(window)
		, device(device_)
	{
		RecreateSwapChain();
		CreateCommandBuffers();
	}

	Scene::~Scene()
	{
		FreeCommandBuffers();
	}

	void Scene::CreateCommandBuffers()
	{
		commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = device.GetCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(device.Device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate command buffer!");
		}
	}

	void Scene::FreeCommandBuffers()
	{
		vkFreeCommandBuffers(device.Device(), device.GetCommandPool(),
			static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		commandBuffers.clear();
	}

	void Scene::RecreateSwapChain()
	{
		auto ext = appWindow.GetExtent();
		while (ext.width == 0 || ext.height == 0)
		{
			ext = appWindow.GetExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device.Device());

		if (swapChain == nullptr)
		{
			swapChain = std::make_unique<SwapChain>(device, ext);
		}
		else
		{
			std::shared_ptr<SwapChain> oldSwapChain = std::move(swapChain);
			swapChain = std::make_unique<SwapChain>(device, ext, std::move(swapChain));

			if (!oldSwapChain->CompareSwapFormats(*swapChain.get()))
			{
				throw std::runtime_error("Swap chain image (or depth) format has changed!");
			}
		}

		
	}

	VkCommandBuffer Scene::BeginFrame()
	{
		assert(!isFrameStarted && "Can't call BeginFrame while already in progress!");

		auto res = swapChain->AcquireNextImage(&currImageIdx);
		if (res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapChain();
			return nullptr;
		}

		if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		isFrameStarted = true;

		auto cmdBuf = GetCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(cmdBuf, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to begin recording command buffer!");
		}

		return cmdBuf;
	}

	void Scene::EndFrame()
	{
		assert(isFrameStarted && "Can't call EndFrame while already in progress!");

		auto cmdBuf = GetCurrentCommandBuffer();

		if (vkEndCommandBuffer(cmdBuf) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer!");
		}

		auto res = swapChain->SubmitCommandBuffers(&cmdBuf, &currImageIdx);
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || appWindow.WasWindowResized())
		{
			appWindow.ResetWindowResizedFlag();
			RecreateSwapChain();
		}
		else if (res != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image!");
		}

		isFrameStarted = false;
		currFrameIdx = (currFrameIdx + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void Scene::ProcessInput(float dt, Camera& c)
	{
		if (Keyboard::Key(GLFW_KEY_W))
			c.UpdateCameraPos(CameraDirection::FORWARD, dt);
		if (Keyboard::Key(GLFW_KEY_S))
			c.UpdateCameraPos(CameraDirection::BACKWARDS, dt);
		if (Keyboard::Key(GLFW_KEY_A))
			c.UpdateCameraPos(CameraDirection::LEFT, dt);
		if (Keyboard::Key(GLFW_KEY_D))
			c.UpdateCameraPos(CameraDirection::RIGHT, dt);
		if (Keyboard::Key(GLFW_KEY_Q))
			c.UpdateCameraPos(CameraDirection::UP, dt);
		if (Keyboard::Key(GLFW_KEY_E))
			c.UpdateCameraPos(CameraDirection::DOWN, dt);

		double dx = Mouse::GetDX(); double dy = Mouse::GetDY();
		if (dx != 0 || dy != 0)
		{
			float sens = 1.f;
			c.UpdateCameraDir(dx * sens, dy * sens);
		}

		if (Keyboard::KeyDown(GLFW_KEY_GRAVE_ACCENT))
		{
			c.rotateCamera = !c.rotateCamera;
		}
	}

	void Scene::ProcessMouse(float x, float y, Camera& c)
	{
		c.UpdateCameraDir(x, y);
	}

	void Scene::BeginSwapChainRenderPass(VkCommandBuffer cmdBuf)
	{
		assert(isFrameStarted && "Can't call BeginSwapChainRenderPass while not in progress!");
		assert(cmdBuf == GetCurrentCommandBuffer() && "Can't begin render pass on command buffer from different frame!");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = swapChain->GetRenderPass();
		renderPassInfo.framebuffer = swapChain->GetFrameBuffer(currImageIdx);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChain->GetSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.5f, 0.5f, 0.5f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChain->GetSwapChainExtent().width);
		viewport.height = static_cast<float>(swapChain->GetSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, swapChain->GetSwapChainExtent() };
		vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
		vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
	}

	void Scene::EndSwapChainRenderPass(VkCommandBuffer cmdBuf)
	{
		assert(isFrameStarted && "Can't call EndSwapChainRenderPass while not in progress!");
		assert(cmdBuf == GetCurrentCommandBuffer() && "Can't end render pass on command buffer from different frame!");
	
		vkCmdEndRenderPass(cmdBuf);
	}

}