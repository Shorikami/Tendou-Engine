#include "Editor.h"

#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>

namespace Tendou
{
	Editor::Editor(Window& appWindow, Scene* scene, TendouDevice& device)
		: td(device)
		, activeScene(scene)
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
		pool_info.maxSets = static_cast<uint32_t>(1000);
		pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
		pool_info.pPoolSizes = pool_sizes;

		if (vkCreateDescriptorPool(td.Device(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to initialize vulkan ImGUI pool!");
		}

		// Setup Dear ImGui context
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		ImGui_ImplGlfw_InitForVulkan(appWindow.GetGLFWwindow(), true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = scene->device.instance;
		init_info.PhysicalDevice = scene->device.physicalDevice;
		init_info.Device = td.Device();
		init_info.QueueFamily = scene->device.FindQueueFamilies(td.physicalDevice).graphicsFamily;
		init_info.Queue = td.graphicsQueue_;
		init_info.DescriptorPool = imguiPool;
		init_info.MinImageCount = static_cast<uint32_t>(scene->swapChain->ImageCount());
		init_info.ImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
		ImGui_ImplVulkan_Init(&init_info, scene->GetSwapChainRenderPass());

		// IMGUI COMMAND BUFFER
		VkCommandBuffer commandBuffer = td.BeginSingleTimeCommands();
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		td.EndSingleTimeCommands(commandBuffer);

		vkDeviceWaitIdle(td.Device());

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	Editor::~Editor()
	{
		vkDestroyDescriptorPool(td.Device(), imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
	}

	void Editor::Setup()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// TODO: ImGui options
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Test"))
			{
				if (ImGui::MenuItem("Test 1"))
				{
				}
				if (ImGui::MenuItem("Test 2"))
				{
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		activeScene->PreUpdate();
		// DEMO WINDOW
		// TODO: Remove this when you don't need it anymore
		ImGui::ShowDemoWindow();
	}

	void Editor::Draw(VkCommandBuffer buf)
	{
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buf);
	}
}