#include "Application.h"

// Include ImGUI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "../Rendering/RenderSystem.h"
#include "../Rendering/Buffer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>

namespace Drevis
{
	struct WorldUBO
	{
		glm::mat4 proj{ 1.f };
		glm::mat4 view{ 1.f };
	};

	struct LightUBO
	{
		glm::vec4 ambient{ 1.f, 1.f, 1.f, .02f };
		glm::vec3 lightPos{ -1.f };
		alignas(16) glm::vec4 lightColor{ 1.f }; // w is light intensity
	};

	Application::Application()
	{
		globalPool = DescriptorPool::Builder(device)
			.SetMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
			.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
			.Build();

		LoadGameObjects();
		glfwSetCursorPosCallback(appWindow.GetGLFWwindow(), MouseCallback);
	}

	Application::~Application()
	{
		vkDestroyDescriptorPool(device.Device(), imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
	}

	void Application::Run()
	{
		std::vector<std::unique_ptr<Buffer>> uboBuffers(2);

		//for (int i = 0; i < uboBuffers.size(); ++i)
		//{
		//	uboBuffers[i] = std::make_unique<Buffer>(
		//		device,
		//		sizeof(WorldUBO),
		//		1,
		//		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		//		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		//	uboBuffers[i]->Map();
		//}

		uboBuffers[0] = std::make_unique<Buffer>(
			device,
			sizeof(WorldUBO),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		uboBuffers[0]->Map();

		uboBuffers[1] = std::make_unique<Buffer>(
			device,
			sizeof(LightUBO),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		uboBuffers[1]->Map();

		auto globalSetLayout = DescriptorSetLayout::Builder(device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build();

		std::vector<VkDescriptorSet> globalDescriptorSets(1);

		//for (int i = 0; i < globalDescriptorSets.size(); ++i)
		//{
		//	auto bufInfo = uboBuffers[i]->DescriptorInfo();
		//
		//	DescriptorWriter(*globalSetLayout, *globalPool)
		//		.WriteBuffer(0, &bufInfo)
		//		//.WriteBuffer(1, &bufInfo)
		//		.Build(globalDescriptorSets[i]);
		//}

		auto bufInfo = uboBuffers[0]->DescriptorInfo();
		auto bufInfo2 = uboBuffers[1]->DescriptorInfo();
		
		DescriptorWriter(*globalSetLayout, *globalPool)
			.WriteBuffer(0, &bufInfo)
			.WriteBuffer(1, &bufInfo2)
			.Build(globalDescriptorSets[0]);

		RenderSystem renderSys
		{ 
			device, 
			scene.GetSwapChainRenderPass(), 
			globalSetLayout->GetDescriptorSetLayout()
		};

		auto currTime = std::chrono::high_resolution_clock::now();

		InitImGUI();

		do
		{
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currTime).count();
			currTime = newTime;

			
			if (auto cmdBuf = scene.BeginFrame())
			{
				int frameIdx = scene.GetFrameIndex();

				FrameInfo f(frameIdx, frameTime, cmdBuf, c, globalDescriptorSets[0], gameObjects);

				// update
				// -----
				scene.ProcessInput(frameTime, c);

				WorldUBO localUBO{};
				localUBO.proj = c.perspective();
				localUBO.view = c.view();
				uboBuffers[0]->WriteToBuffer(&localUBO);
				uboBuffers[0]->Flush();

				LightUBO lightUbo{};
				lightUbo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				lightUbo.lightPos = glm::vec3(-2.0f, -1.0f, 1.0f);
				uboBuffers[1]->WriteToBuffer(&lightUbo);
				uboBuffers[1]->Flush();

				//render
				// -----
				ImGui_ImplVulkan_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();

				// TODO: ImGui options?

				// DEMO WINDOW
				ImGui::ShowDemoWindow();


				scene.BeginSwapChainRenderPass(cmdBuf);

				ImGui::Render();
				renderSys.RenderGameObjects(f);

				// DO NOT FORGET TO DO THIS CALL!!!
				// Todo: Abstract ImGui calls?
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);
				scene.EndSwapChainRenderPass(cmdBuf);

				scene.EndFrame();
			}
		} while (appWindow.ShouldClose());
		
		vkDeviceWaitIdle(device.Device());
	}

	void Application::InitImGUI()
	{
		IMGUI_CHECKVERSION();
		//1: create descriptor pool for IMGUI
		// the size of the pool is very oversize, but it's copied from imgui demo itself.
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		if (vkCreateDescriptorPool(device.Device(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to initialize vulkan ImGUI pool!");
		}

		// Setup Dear ImGui context
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		//ImGui::StyleColorsDark();
		ImGui::StyleColorsClassic();

		ImGui_ImplGlfw_InitForVulkan(appWindow.GetGLFWwindow(), true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = device.instance;
		init_info.PhysicalDevice = device.physicalDevice;
		init_info.Device = device.Device();
		init_info.QueueFamily = device.FindQueueFamilies(device.physicalDevice).graphicsFamily;
		init_info.Queue = device.graphicsQueue_;
		init_info.DescriptorPool = imguiPool;
		init_info.MinImageCount = scene.swapChain->ImageCount();
		init_info.ImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
		ImGui_ImplVulkan_Init(&init_info, scene.GetSwapChainRenderPass());

		// IMGUI COMMAND BUFFER
		// TODO: Move these to single command buffer functions
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = device.commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device.Device(), &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(device.GraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(device.GraphicsQueue());

		vkFreeCommandBuffers(device.Device(), device.commandPool, 1, &commandBuffer);

		vkDeviceWaitIdle(device.Device());

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void Application::LoadGameObjects()
	{
		std::shared_ptr<Model> model = Model::CreateModelFromFile(device, "Materials/Models/smooth_vase.obj");

		auto smoothVase = GameObject::CreateGameObject();
		smoothVase.model = model;
		smoothVase.transform.translation = glm::vec3(0.0f, 0.0f, 0.f);
		smoothVase.transform.scale = glm::vec3(3.5f);

		gameObjects.emplace(smoothVase.GetID(), std::move(smoothVase));

		model = Model::CreateModelFromFile(device, "Materials/Models/quad.obj");

		auto floor = GameObject::CreateGameObject();
		floor.model = model;
		floor.transform.translation = glm::vec3(0.0f, 0.5f, 0.0f);
		floor.transform.scale = glm::vec3(3.5f);

		gameObjects.emplace(floor.GetID(), std::move(floor));

		for (unsigned i = 0; i < 1; ++i)
		{
			model = Model::CreateModelFromFile(device, "Materials/Models/sphere.obj");
			
			auto sphere = GameObject::CreateGameObject();
			sphere.model = model;
			sphere.transform.translation = glm::vec3(0.0f, 0.0f, 1.5f * (0.25f * i));
			sphere.transform.scale = glm::vec3(1.f);
			
			gameObjects.emplace(sphere.GetID(), std::move(sphere));
		}



		//model = Model::CreateModelFromFile(device, "Materials/Models/flat_vase.obj");
		//
		//auto flatVase = GameObject::CreateGameObject();
		//flatVase.model = model;
		//flatVase.transform.translation = glm::vec3(0.0f, 0.0f, 1.5f);
		//flatVase.transform.scale = glm::vec3(3.5f);
		//
		//gameObjects.emplace(flatVase.GetID(), std::move(flatVase));
;	}

	void MouseCallback(GLFWwindow* win, double x, double y)
	{
		if (firstMouse)
		{
			lastX = static_cast<float>(x);
			lastY = static_cast<float>(y);
			firstMouse = false;
		}

		float xoffset = static_cast<float>(x) - lastX;
		float yoffset = lastY - static_cast<float>(y); // reversed since y-coordinates go from bottom to top

		lastX = static_cast<float>(x);
		lastY = static_cast<float>(y);

		c.UpdateCameraDir(xoffset, yoffset);
	}
}