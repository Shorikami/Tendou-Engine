#include "Application.h"
#include "../Editor/Editor.h"

#include "../Vulkan/Systems/Default.h"
#include "../Vulkan/Systems/OffscreenSystem.h"

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
			scene->GetSetLayout("Global")->GetDescriptorSetLayout()
		};

		OffscreenSystem offscreenSys[6]
		{
			{ device, scene->renderPasses["Offscreen1"].renderPass, scene->GetSetLayout("Offscreen")->GetDescriptorSetLayout()},
			{ device, scene->renderPasses["Offscreen2"].renderPass, scene->GetSetLayout("Offscreen")->GetDescriptorSetLayout()},
			{ device, scene->renderPasses["Offscreen3"].renderPass, scene->GetSetLayout("Offscreen")->GetDescriptorSetLayout()},
			{ device, scene->renderPasses["Offscreen4"].renderPass, scene->GetSetLayout("Offscreen")->GetDescriptorSetLayout()},
			{ device, scene->renderPasses["Offscreen5"].renderPass, scene->GetSetLayout("Offscreen")->GetDescriptorSetLayout()},
			{ device, scene->renderPasses["Offscreen6"].renderPass, scene->GetSetLayout("Offscreen")->GetDescriptorSetLayout()}
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
					scene->GetDescriptorSet("Global"), scene->GetGameObjects());

				FrameInfo g(frameIdx, frameTime, cmdBuf, scene->GetCamera(),
					scene->GetDescriptorSet("Offscreen"), scene->GetGameObjects());

				//render
				// TODO: Move render pass calls to a scene renderer function
				// -----
				editor.get()->Setup();

				static glm::vec3 directionLookup[] =
				{
						{1.f, 0.f, 0.f},  // +x
						{-1.f, 0.f, 0.f}, // -x
						{0.f, 1.0f, 0.f}, // +y
						{0.f, -1.0f, 0.f},// -y
						{0.f, 0.f, 1.0f}, // +z
						{0.f, 0.f, -1.0f} // -z
				};
				static glm::vec3 upLookup[] =
				{
						{0.f, -1.0f, 0.f},   // +x
						{0.f, -1.0f, 0.f},   // -x
						{0.f, 0.0f, 1.f},	// +y
						{0.f, 0.0f, -1.f},   // -y
						{0.f, -1.0f, 0.f},   // +z
						{0.f, -1.0f, 0.f}    // -z
				};

				// Offscreen render test
				for (int i = 0; i < 6; ++i)
				{
					scene->BeginRenderPass(cmdBuf, std::string("Offscreen") + std::to_string(i + 1));
					glm::vec3 objPos = g.gameObjects.find(0)->second.GetTransform().PositionVec3();

					//dynamic_cast<LightingScene*>(scene.get())->
					//	OverwriteWorldUBO(glm::lookAt(
					//		objPos, directionLookup[i], -upLookup[i]), i);
					
					offscreenSys[i].Render(g);
					scene->EndRenderPass(cmdBuf);
				}

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