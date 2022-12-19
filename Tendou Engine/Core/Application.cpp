#include "Application.h"
#include "../Editor/Editor.h"

#include "../Vulkan/Systems/Default.h"

#include "../Rendering/Texture.h"

#include "../Rendering/Scenes/SimpleScene.h"
#include "../Rendering/Scenes/LightingScene.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>

namespace Tendou
{
	Application::Application()
	{
		scene = std::make_unique<LightingScene>(appWindow, device);
		editor = std::make_unique<Editor>(appWindow, scene.get(), device);
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		scene->Init();

		DefaultSystem defaultSys
		{ 
			device, 
			scene->GetSwapChainRenderPass(), 
			scene->GetGlobalSetLayout()->GetDescriptorSetLayout()
		};

		DefaultSystem offscreenSys
		{
			device,
			scene->renderPasses["Offscreen"].renderPass,
			scene->GetGlobalSetLayout()->GetDescriptorSetLayout()
		};

		auto currTime = std::chrono::high_resolution_clock::now();

		do
		{
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currTime).count();
			currTime = newTime;
			
			if (auto cmdBuf = scene->BeginFrame())
			{
				int frameIdx = scene->GetFrameIndex();

				// update
				// -----
				scene->ProcessInput(frameTime, scene->GetCamera());
				scene->Update();

				FrameInfo f(frameIdx, frameTime, cmdBuf, scene->GetCamera(),
					scene->GetGlobalDescriptorSets(), scene->GetGameObjects());

				std::vector<VkDescriptorSet> testSets;
				testSets.resize(3);
				
				testSets[0] = f.descriptorSets[2];
				testSets[1] = f.descriptorSets[1];
				testSets[2] = f.descriptorSets[0];

				FrameInfo g(frameIdx, frameTime, cmdBuf, scene->GetCamera(),
					testSets, scene->GetGameObjects());

				//render
				// TODO: Move render pass calls to a scene renderer function
				// -----
				editor.get()->Setup();

				// Offscreen render test
				scene->BeginRenderPass(cmdBuf, "Offscreen");
				offscreenSys.Render(g);
				scene->EndRenderPass(cmdBuf);


				scene->BeginSwapChainRenderPass(cmdBuf);
				defaultSys.Render(f);
				
				// NOTE: Render the editor AFTER all render passes;
				// rendering the editor first draws it behind objects
				editor.get()->Draw(cmdBuf);
				scene->EndSwapChainRenderPass(cmdBuf);
				
				scene->EndFrame();
			}
		} 
		while (appWindow.ShouldClose());
		
		vkDeviceWaitIdle(device.Device());
	}
}