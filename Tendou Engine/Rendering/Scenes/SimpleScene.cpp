#include "SimpleScene.h"

namespace Tendou
{
	SimpleScene::SimpleScene(Window& window, TendouDevice& device)
		: Scene(window, device)
	{
		globalPool = DescriptorPool::Builder(device)
			.SetMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
			.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
			.AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT)
			.Build();

		LoadGameObjects();
	}

	SimpleScene::~SimpleScene()
	{
	}

	int SimpleScene::Init()
	{
		worldUBO = std::make_unique<UniformBuffer<WorldUBO>>(
			device,
			static_cast<uint32_t>(sizeof(WorldUBO)),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		worldUBO->Map();

		lightUBO = std::make_unique <UniformBuffer<LightsUBO>>(
			device,
			sizeof(LightsUBO),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		lightUBO->Map();

		textures.push_back(std::make_unique<Texture>(device, "Materials/Models/Shiroko/Texture2D/Shiroko_Original_Weapon.png"));
		textures.push_back(std::make_unique<Texture>(device, "Materials/Textures/c.png"));

		globalSetLayout = DescriptorSetLayout::Builder(device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build();

		auto bufInfo = worldUBO->DescriptorInfo();
		auto bufInfo2 = lightUBO->DescriptorInfo();
		auto texInfo = textures[0]->DescriptorInfo();
		auto texInfo2 = textures[1]->DescriptorInfo();

		globalDescriptorSets.resize(2);

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

		return 0;
	}

	int SimpleScene::PreUpdate()
	{
		return 0;
	}

	int SimpleScene::Update()
	{
		WorldUBO localUBO{};
		localUBO.proj = c.perspective();
		localUBO.view = c.view();
		worldUBO->WriteToBuffer(&localUBO);
		worldUBO->Flush();

		LightsUBO lightUbo{};
		lightUbo.lightColor[0] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		lightUbo.lightPos[0] = glm::vec4(-2.0f, -1.0f, 1.0f, 1.0f);
		lightUBO->WriteToBuffer(&lightUbo);
		lightUBO->Flush();

		return 0;
	}

	int SimpleScene::PostUpdate()
	{
		return 0;
	}

	void SimpleScene::LoadGameObjects()
	{
		std::shared_ptr<Model> model = Model::CreateModelFromFile(device, Model::Type::OBJ,
			"Materials/Models/Shiroko/Mesh/Shiroko_Original_Weapon.obj",
			"Materials/Models/Shiroko/Mesh/Texture2D/", true);

		auto whiteFang = GameObject::CreateGameObject();
		whiteFang.SetModel(model);
		whiteFang.GetTransform().SetTranslation(glm::vec3(0.f));
		whiteFang.GetTransform().SetRotation(glm::vec3(0.0f, 1.0f, 0.0f));
		whiteFang.GetTransform().SetScale(glm::vec3(3.5f));

		gameObjects.emplace(whiteFang.GetID(), std::move(whiteFang));


		model = Model::CreateModelFromFile(device, Model::Type::OBJ,
			"Materials/Models/sphere.obj", std::string(), true);

		for (unsigned i = 0; i < 8; ++i)
		{
			auto sphere = GameObject::CreateGameObject("Sphere");
			sphere.SetModel(model);
			sphere.GetTransform().SetTranslation(glm::vec3(0.0f, 0.0f, 20.0f));
			sphere.GetTransform().SetRotation(glm::vec3(0.0f, 1.0f, 0.0f));
			sphere.GetTransform().SetScale(glm::vec3(0.08f));

			gameObjects.emplace(sphere.GetID(), std::move(sphere));
		}

		model = Model::CreateModelFromFile(device, Model::Type::OBJ, "Materials/Models/quad.obj");

		auto floor = GameObject::CreateGameObject();
		floor.SetModel(model);
		floor.GetTransform().SetTranslation(glm::vec3(0.0f, 0.5f, 0.0f));
		floor.GetTransform().SetScale(glm::vec3(3.5f));

		gameObjects.emplace(floor.GetID(), std::move(floor));
		;
	}

}