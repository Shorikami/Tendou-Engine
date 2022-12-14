#include "Application.h"

#include "../Editor/Editor.h"
#include "../Vulkan/Systems/Default.h"

#include "../Rendering/Buffer.h"
#include "../Rendering/Texture.h"


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
			.AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT)
			.Build();

		editor = std::make_unique<Editor>(appWindow, scene, device);
		LoadGameObjects();
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		std::vector<std::unique_ptr<Buffer>> uboBuffers(2);
		std::vector<std::unique_ptr<Texture>> textures(2);
		

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

		textures[0] = std::make_unique<Texture>(device, "Materials/Models/Shiroko/Texture2D/Shiroko_Original_Weapon.png");
		textures[1] = std::make_unique<Texture>(device, "Materials/Textures/c.png");

		auto globalSetLayout = DescriptorSetLayout::Builder(device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build();

		std::vector<VkDescriptorSet> globalDescriptorSets(2);

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
		auto texInfo = textures[0]->DescriptorInfo();
		auto texInfo2 = textures[1]->DescriptorInfo();
		
		DescriptorWriter(*globalSetLayout, *globalPool)
			.WriteBuffer(0, &bufInfo)
			.WriteBuffer(1, &bufInfo2)
			.WriteImage(2, &texInfo)
			.Build(globalDescriptorSets[0]);

		DescriptorWriter(*globalSetLayout, *globalPool)
			.WriteBuffer(0, &bufInfo)
			.WriteBuffer(1, &bufInfo2)
			.WriteImage(2, &texInfo2)
			.Build(globalDescriptorSets[1]);

		DefaultSystem defaultSys
		{ 
			device, 
			scene.GetSwapChainRenderPass(), 
			globalSetLayout->GetDescriptorSetLayout()
		};

		auto currTime = std::chrono::high_resolution_clock::now();

		do
		{
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currTime).count();
			currTime = newTime;

			
			if (auto cmdBuf = scene.BeginFrame())
			{
				int frameIdx = scene.GetFrameIndex();

				FrameInfo f(frameIdx, frameTime, cmdBuf, c, globalDescriptorSets, gameObjects);

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
				editor.get()->Setup();

				scene.BeginSwapChainRenderPass(cmdBuf);
				
				defaultSys.RenderGameObjects(f);

				// NOTE: Render the editor AFTER all render passes;
				// rendering the editor first draws it behind objects
				editor.get()->Draw(cmdBuf);

				scene.EndSwapChainRenderPass(cmdBuf);
				scene.EndFrame();
			}
		} while (appWindow.ShouldClose());
		
		vkDeviceWaitIdle(device.Device());
	}


	// TODO: Clean this up
	void Application::InitImGUI()
	{

	}

	void Application::LoadGameObjects()
	{
		std::shared_ptr<Model> model = Model::CreateModelFromFile(device, Model::Type::OBJ,
			"Materials/Models/Shiroko/Mesh/Shiroko_Original_Weapon.obj",
			"Materials/Models/Shiroko/Mesh/Texture2D/", true);

		auto whiteFang = GameObject::CreateGameObject();
		whiteFang.SetModel(model);
		whiteFang.GetTransform().SetTranslation(glm::vec3(0.f));
		whiteFang.GetTransform().SetScale(glm::vec3(3.5f));

		gameObjects.emplace(whiteFang.GetID(), std::move(whiteFang));


		model = Model::CreateModelFromFile(device, Model::Type::OBJ,
			"Materials/Models/sphere.obj", std::string(), true);

		for (unsigned i = 0; i < 8; ++i)
		{
			auto sphere = GameObject::CreateGameObject();
			sphere.SetModel(model);
			sphere.GetTransform().SetTranslation(glm::vec3(0.f));
			sphere.GetTransform().SetScale(glm::vec3(0.08f));

			gameObjects.emplace(sphere.GetID(), std::move(sphere));
		}

		model = Model::CreateModelFromFile(device, Model::Type::OBJ, "Materials/Models/quad.obj");
		
		auto floor = GameObject::CreateGameObject();
		floor.SetModel(model);
		floor.GetTransform().SetTranslation(glm::vec3(0.0f, 0.5f, 0.0f));
		floor.GetTransform().SetScale(glm::vec3(3.5f));
		
		gameObjects.emplace(floor.GetID(), std::move(floor));


		//model = Model::CreateModelFromFile(device, "Materials/Models/flat_vase.obj");
		//
		//auto flatVase = GameObject::CreateGameObject();
		//flatVase.model = model;
		//flatVase.transform.translation = glm::vec3(0.0f, 0.0f, 1.5f);
		//flatVase.transform.scale = glm::vec3(3.5f);
		//
		//gameObjects.emplace(flatVase.GetID(), std::move(flatVase));
;	}

	//void MouseCallback(GLFWwindow* win, double x, double y)
	//{
	//	if (firstMouse)
	//	{
	//		lastX = static_cast<float>(x);
	//		lastY = static_cast<float>(y);
	//		firstMouse = false;
	//	}
	//
	//	float xoffset = static_cast<float>(x) - lastX;
	//	float yoffset = lastY - static_cast<float>(y); // reversed since y-coordinates go from bottom to top
	//
	//	lastX = static_cast<float>(x);
	//	lastY = static_cast<float>(y);
	//
	//	c.UpdateCameraDir(xoffset, yoffset);
	//}
}