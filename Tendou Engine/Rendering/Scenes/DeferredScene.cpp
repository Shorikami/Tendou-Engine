#include "DeferredScene.h"

namespace Tendou
{
	DeferredScene::DeferredScene(Window& window, TendouDevice& device)
		: Scene(window, device)
	{
		globalPool = DescriptorPool::Builder(device)
			.SetMaxSets(20)
			.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 20)
			.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 20)
			.AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 20)
			.Build();

		LoadGameObjects();
		CreateRenderPasses();
	}

	// TODO: More abstraction
	DeferredScene::~DeferredScene()
	{
		// Frame buffer
		for (int i = 0; i < 6; ++i)
		{
			std::string key = std::string("Offscreen") + std::to_string(i + 1);
			// Color attachment
			vkDestroyImageView(device.Device(), renderPasses[key].color.view, nullptr);
			vkDestroyImage(device.Device(), renderPasses[key].color.image, nullptr);
			vkFreeMemory(device.Device(), renderPasses[key].color.memory, nullptr);

			// Depth attachment
			vkDestroyImageView(device.Device(), renderPasses[key].depth.view, nullptr);
			vkDestroyImage(device.Device(), renderPasses[key].depth.image, nullptr);
			vkFreeMemory(device.Device(), renderPasses[key].depth.memory, nullptr);

			vkDestroyRenderPass(device.Device(), renderPasses[key].renderPass, nullptr);
			vkDestroySampler(device.Device(), renderPasses[key].sampler, nullptr);
			vkDestroyFramebuffer(device.Device(), renderPasses[key].frameBuffer, nullptr);
		}
	}

	int DeferredScene::Init()
	{
		CreateUBOs();
		CreateSetLayouts();
		CreateRenderSystems();

		for (unsigned i = 0; i < MAX_LIGHTS; ++i)
		{
			editorVars.ambient[i] = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
			editorVars.diffuse[i] = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
			editorVars.specular[i] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		return 0;
	}

	int DeferredScene::PreUpdate()
	{
		if (ImGui::BeginMenu("Scene Settings"))
		{
			ImGui::SliderInt("No. of Lights", &editorVars.currLights, 1, 16);
			ImGui::SliderFloat("Orbit Radius", &editorVars.sphereLineRad, 0.1f, 100.0f);
			ImGui::Checkbox(("Enable Rotation"), &editorVars.rotateSpheres);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Global Values"))
		{
			ImGui::SliderFloat("Camera Near", &editorVars.nearFar.x, 0.1f, 100.0f);
			ImGui::SliderFloat("Camera Far", &editorVars.nearFar.y, 1.0f, 100.0f);

			ImGui::ColorEdit4("Emissive", &editorVars.emissive[0]);

			ImGui::SliderFloat3("Light Coefficients", &editorVars.lightCoeffs[0], 0.0f, 1.0f);
			ImGui::SliderFloat("Att. Constant", &editorVars.attenuation.x, 0.0f, 1.0f);
			ImGui::SliderFloat("Att. Linear", &editorVars.attenuation.y, 0.0f, 1.0f);
			ImGui::SliderFloat("Att. Quadratic", &editorVars.attenuation.z, 0.0f, 1.0f);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Per-Light Values"))
		{
			for (unsigned i = 0; i < editorVars.currLights; ++i)
			{
				ImGui::PushID(i);
				std::string id = std::string(1, (char)('0' + i));
				if (ImGui::TreeNode(id.c_str(), "Light %i", i))
				{
					ImGui::ColorEdit4("Ambient Color", &editorVars.ambient[i][0]);
					ImGui::ColorEdit4("Diffuse Color", &editorVars.diffuse[i][0]);
					ImGui::ColorEdit4("Specular Color", &editorVars.specular[i][0]);

					ImGui::TreePop();
				}
				ImGui::PopID();
			}

			ImGui::EndMenu();
		}

		return 0;
	}

	int DeferredScene::Update()
	{
		static float angle = 0.0f;
		int idx = 0;

		//LightsUBO lightUbo{};
		//lightUbo.eyePos = glm::vec4(c.cameraPos, 1.0f);
		//
		//for (unsigned i = 0; i < editorVars.currLights; ++i)
		//{
		//	lightUbo.lightColor[i] = glm::vec4(1.0f);
		//
		//	lightUbo.ambient[i] = glm::vec4(editorVars.ambient[i], 1.0f);
		//	lightUbo.diffuse[i] = glm::vec4(editorVars.diffuse[i], 1.0f);
		//	lightUbo.specular[i] = glm::vec4(editorVars.specular[i], 1.0f);
		//
		//	// x = outer, y = inner, z = falloff, w = type
		//	lightUbo.lightInfo[i] = glm::vec4(80.0f, 45.0f, 10.0f, 0.0f);
		//}
		//
		//lightUbo.emissive = glm::vec4(editorVars.emissive, 1.0f);
		//lightUbo.attenuation = editorVars.attenuation;
		//
		//lightUbo.coefficients = glm::vec4(editorVars.lightCoeffs, 1.0f);
		//lightUbo.fogColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		//lightUbo.numLights = editorVars.currLights;
		//
		//// x = use gpu, y = use normals, z = uv type
		//lightUbo.modes = glm::ivec4(0, 0, 0, 0);

		// TODO: IDs to string tags?
		GameObject& mainObj = gameObjects.find(0)->second;

		for (auto& a : gameObjects)
		{
			auto& obj = a.second;
			if (obj.GetTag() == "Light")
			{
				if (idx >= editorVars.currLights)
				{
					obj.SetRender(false);
				}
				else
				{
					obj.SetRender(true);
				}

				float res = angle + (glm::pi<float>() / static_cast<float>(editorVars.currLights)) * idx;
				obj.GetTransform().SetTranslation(glm::vec3(0.0f, 0.0f, editorVars.sphereLineRad));
				obj.GetTransform().SetRotationAngle(res);
				obj.GetTransform().Update(true);
				//lightUbo.lightPos[idx] = obj.GetTransform().PositionVec4();
				//lightUbo.lightDir[idx] = mainObj.GetTransform().PositionVec4() - lightUbo.lightPos[idx];
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
		localUBO.nearFar = glm::vec2(editorVars.nearFar.x, editorVars.nearFar.y);
		worldUBO->WriteToBuffer(&localUBO);
		worldUBO->Flush();

		//lightUbo.lightColor[0] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		//lightUbo.lightPos[0] = glm::vec4(-2.0f, -1.0f, 1.0f, 1.0f);
		//lightUBO->WriteToBuffer(&lightUbo);
		//lightUBO->Flush();

		if (editorVars.rotateSpheres)
		{
			angle += 0.01f;
		}

		if (angle > glm::pi<float>())
		{
			angle = 0.0f;
		}

		return 0;
	}

	int DeferredScene::PostUpdate()
	{
		return 0;
	}

	int DeferredScene::Render(VkCommandBuffer buf, FrameInfo& f)
	{
		SceneInfo deferred(GetDescriptorSet("Deferred"), GetGameObjects());
		SceneInfo global(GetDescriptorSet("Global"), GetGameObjects());

		BeginRenderPass(buf, "Deferred");
		renderSystems["Deferred"][0].get()->Render(f, deferred);
		EndRenderPass(buf);

		// Render the actual scene (swapchain)
		BeginSwapChainRenderPass(buf);

		renderSystems["Global"][0].get()->Render(f, global);

		return 0;
	}

	void DeferredScene::LoadGameObjects()
	{
		std::shared_ptr<Model> model = Model::CreateModelFromFile(device, Model::Type::OBJ,
			"Materials/Models/smooth_vase.obj");

		auto whiteFang = GameObject::CreateGameObject("TextureTarget", "Vase");
		whiteFang.SetModel(model);
		whiteFang.GetTransform().SetTranslation(glm::vec3(0.f));
		whiteFang.GetTransform().SetRotation(glm::vec3(0.0f, 0.5f, 0.0f));
		whiteFang.GetTransform().SetScale(glm::vec3(5.5f));

		gameObjects.emplace(whiteFang.GetID(), std::move(whiteFang));


		//model = Model::CreateModelFromFile(device, Model::Type::OBJ,
		//	"Materials/Models/sphere.obj", std::string(), true);
		//
		//for (unsigned i = 0; i < MAX_LIGHTS; ++i)
		//{
		//	auto sphere = GameObject::CreateGameObject("Light", "Sphere");
		//	sphere.SetModel(model);
		//	sphere.GetTransform().SetTranslation(glm::vec3(0.0f, 0.0f, editorVars.sphereLineRad));
		//	sphere.GetTransform().SetRotation(glm::vec3(0.0f, 1.0f, 0.0f));
		//	sphere.GetTransform().SetScale(glm::vec3(0.08f));
		//
		//	gameObjects.emplace(sphere.GetID(), std::move(sphere));
		//}
		//
		//model = Model::CreateModelFromFile(device, Model::Type::OBJ,
		//	"Materials/Models/cube.obj", std::string(), true);
		//
		//auto skybox = GameObject::CreateGameObject("Skybox", "Sky");
		//skybox.SetModel(model);
		//skybox.GetTransform().SetTranslation(glm::vec3(0.f));
		////skybox.GetTransform().SetRotation(glm::vec3(0.0f, 0.5f, 0.0f));
		//skybox.GetTransform().SetScale(glm::vec3(50.0f));
		//
		//gameObjects.emplace(skybox.GetID(), std::move(skybox));
	}

	void DeferredScene::CreateUBOs()
	{
		worldUBO = std::make_unique<UniformBuffer<WorldUBO>>(
			UBO::Type::WORLD,
			device,
			UBO::SizeofUBO(UBO::Type::WORLD),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		worldUBO->Map();

		lightUBO = std::make_unique<UniformBuffer<LightsUBO>>(
			UBO::Type::LIGHTS,
			device,
			UBO::SizeofUBO(UBO::Type::LIGHTS),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		lightUBO->Map();

	}

	void DeferredScene::CreateSetLayouts()
	{
		//std::string path = "Materials/Textures/skybox/skybox_";
		//std::vector<std::string> faces =
		//{
		//	path + std::string("right.png"),
		//	path + std::string("left.png"),
		//	path + std::string("top.png"),
		//	path + std::string("bottom.png"),
		//	path + std::string("front.png"),
		//	path + std::string("back.png"),
		//};
		//
		//textures.push_back(std::make_unique<Texture>(device, "Materials/Models/Shiroko/Texture2D/Shiroko_Original_Weapon.png"));
		//textures.push_back(std::make_unique<Texture>(device, "Materials/Textures/hoshino.png"));
		//textures.push_back(std::make_unique<Texture>(device, faces));
		//textures.push_back(std::make_unique<Texture>(device, 1024, 1024, true));

		setLayouts["Global"] = DescriptorSetLayout::Builder(device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			//.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			//.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			//.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			//.AddBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
			.Build();

		setLayouts["Deferred"] = DescriptorSetLayout::Builder(device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			//.AddBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
			//.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build();

		auto worldBuf = worldUBO->DescriptorInfo();
		auto lightBuf = lightUBO->DescriptorInfo();

		//auto whiteFangTex = textures[0]->DescriptorInfo();
		//auto hoshino = textures[1]->DescriptorInfo();
		//auto skyboxTex = textures[2]->DescriptorInfo();
		//auto emptyMap = textures[3]->DescriptorInfo();

		descriptorSets["Global"].resize(1);
		descriptorSets["Deferred"].resize(1);

		// Object set
		DescriptorWriter(*setLayouts["Global"], *globalPool)
			.WriteBuffer(0, &worldBuf)
			//.WriteBuffer(1, &lightBuf)
			//.WriteImage(2, &emptyMap)
			.Build(descriptorSets["Global"][0]);

		// Object set
		DescriptorWriter(*setLayouts["Deferred"], *globalPool)
			.WriteBuffer(0, &worldBuf)
			//.WriteBuffer(1, &lightBuf)
			//.WriteImage(2, &emptyMap)
			.Build(descriptorSets["Deferred"][0]);
	}

	void DeferredScene::CreateRenderPasses()
	{
		renderPasses["Deferred"] = device.CreateDeferredPass(
			swapChain.get()->GetSwapChainExtent().width,
			swapChain.get()->GetSwapChainExtent().height);
	}

	void DeferredScene::CreateRenderSystems()
	{
		renderSystems["Global"].reserve(1);
		renderSystems["Deferred"].reserve(1);

		renderSystems["Global"].emplace_back(std::make_unique<DefaultSystem>(
			device,
			GetSwapChainRenderPass(),
			GetSetLayout("Global")->GetDescriptorSetLayout()
		));

		renderSystems["Deferred"].emplace_back(std::make_unique<DeferredSystem>(
			device,
			GetSwapChainRenderPass(),
			GetSetLayout("Deferred")->GetDescriptorSetLayout()
			));
	}
}