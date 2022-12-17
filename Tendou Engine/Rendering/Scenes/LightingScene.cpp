#include "LightingScene.h"

namespace Tendou
{
	LightingScene::LightingScene(Window& window, TendouDevice& device)
		: Scene(window, device)
	{
		globalPool = DescriptorPool::Builder(device)
			.SetMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
			.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
			.AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT)
			.Build();

		LoadGameObjects();
	}

	LightingScene::~LightingScene()
	{
	}

	int LightingScene::Init()
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

	int LightingScene::PreUpdate()
	{
		ImGui
		return 0;
	}

	int LightingScene::Update()
	{
		static float angle = 0.0f;
		int idx = 0;

		LightsUBO lightUbo{};
		lightUbo.eyePos = glm::vec4(c.cameraPos, 1.0f);

		for (unsigned i = 0; i < 8; ++i)
		{
			lightUbo.lightColor[i] = glm::vec4(1.0f);

			lightUbo.ambient[i] = glm::vec4(0.5f);
			lightUbo.diffuse[i] = glm::vec4(1.0f, glm::vec2(0.0f), 1.0f);
			lightUbo.specular[i] = glm::vec4(0.2f, 0.8f, 0.0f, 1.0f);

			// x = outer, y = inner, z = falloff, w = type
			lightUbo.lightInfo[i] = glm::vec4(80.0f, 45.0f, 10.0f, 0.0f);
		}

		lightUbo.emissive = glm::vec4(0.0f);
		lightUbo.globalAmbient = glm::vec4(0.0f, 0.0f, 26.0f / 255.0f, 1.0f);
		lightUbo.coefficients = glm::vec4(1.0f);
		lightUbo.fogColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		lightUbo.numLights = 2;

		// x = use gpu, y = use normals, z = uv type
		lightUbo.modes = glm::ivec4(0, 0, 0, 0);

		// TODO: IDs to string tags?
		GameObject& mainObj = gameObjects.find(0)->second;

		for (auto& a : gameObjects)
		{
			auto& obj = a.second;
			if (obj.GetTag() == "Sphere")
			{
				float res = angle + (glm::pi<float>() / 2.0f) * idx;
				obj.GetTransform().SetRotationAngle(res);
				obj.GetTransform().Update(true);
				lightUbo.lightPos[idx] = obj.GetTransform().PositionVec4();
				lightUbo.lightDir[idx] = mainObj.GetTransform().PositionVec4() - lightUbo.lightPos[idx];
				++idx;
			}
			else
			{
				obj.GetTransform().Update();
			}
		}

		WorldUBO localUBO{};
		localUBO.proj = c.perspective();
		localUBO.view = c.view();
		localUBO.nearFar = glm::vec2(0.1f, 20.0f);
		worldUBO->WriteToBuffer(&localUBO);
		worldUBO->Flush();

		
		//lightUbo.lightColor[0] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		//lightUbo.lightPos[0] = glm::vec4(-2.0f, -1.0f, 1.0f, 1.0f);
		lightUBO->WriteToBuffer(&lightUbo);
		lightUBO->Flush();

		angle += 0.01f;
		if (angle > glm::pi<float>())
		{
			angle = 0.0f;
		}

		return 0;
	}

	int LightingScene::PostUpdate()
	{
		return 0;
	}

	void LightingScene::LoadGameObjects()
	{
		std::shared_ptr<Model> model = Model::CreateModelFromFile(device, Model::Type::OBJ,
			"Materials/Models/cube2.obj");

		auto whiteFang = GameObject::CreateGameObject("Object");
		whiteFang.SetModel(model);
		whiteFang.GetTransform().SetTranslation(glm::vec3(0.f));
		whiteFang.GetTransform().SetRotation(glm::vec3(0.0f, 1.0f, 0.0f));
		whiteFang.GetTransform().SetScale(glm::vec3(1.0f));

		gameObjects.emplace(whiteFang.GetID(), std::move(whiteFang));


		model = Model::CreateModelFromFile(device, Model::Type::OBJ,
			"Materials/Models/sphere.obj", std::string(), true);

		for (unsigned i = 0; i < 2; ++i)
		{
			auto sphere = GameObject::CreateGameObject("Sphere");
			sphere.SetModel(model);
			sphere.GetTransform().SetTranslation(glm::vec3(0.0f, 0.0f, 25.0f));
			sphere.GetTransform().SetRotation(glm::vec3(0.0f, 1.0f, 0.0f));
			sphere.GetTransform().SetScale(glm::vec3(0.08f));

			gameObjects.emplace(sphere.GetID(), std::move(sphere));
		}
	}
}