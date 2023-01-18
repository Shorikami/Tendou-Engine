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
		// Position
		vkDestroyImageView(device.Device(), renderPasses["Geometry"].position.view, nullptr);
		vkDestroyImage(device.Device(), renderPasses["Geometry"].position.image, nullptr);
		vkFreeMemory(device.Device(), renderPasses["Geometry"].position.memory, nullptr);

		// Normal
		vkDestroyImageView(device.Device(), renderPasses["Geometry"].normal.view, nullptr);
		vkDestroyImage(device.Device(), renderPasses["Geometry"].normal.image, nullptr);
		vkFreeMemory(device.Device(), renderPasses["Geometry"].normal.memory, nullptr);

		// Albedo
		vkDestroyImageView(device.Device(), renderPasses["Geometry"].albedo.view, nullptr);
		vkDestroyImage(device.Device(), renderPasses["Geometry"].albedo.image, nullptr);
		vkFreeMemory(device.Device(), renderPasses["Geometry"].albedo.memory, nullptr);

		// Depth attachment
		vkDestroyImageView(device.Device(), renderPasses["Geometry"].depth.view, nullptr);
		vkDestroyImage(device.Device(), renderPasses["Geometry"].depth.image, nullptr);
		vkFreeMemory(device.Device(), renderPasses["Geometry"].depth.memory, nullptr);

		// Render pass/frame buffer
		vkDestroyRenderPass(device.Device(), renderPasses["Geometry"].renderPass, nullptr);
		vkDestroySampler(device.Device(), renderPasses["Geometry"].sampler, nullptr);
		vkDestroyFramebuffer(device.Device(), renderPasses["Geometry"].frameBuffer, nullptr);
	}

	float RandomNum(float min, float max)
	{
		return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
	}

	int DeferredScene::Init()
	{
		CreateUBOs();
		CreateSetLayouts();
		CreateRenderSystems();

		LightPassUBO passUBO{};
		passUBO.eyePos = glm::vec4(c.cameraPos, 1.0f);

		for (unsigned i = 0; i < MAX_LIGHTS; ++i)
		{
			passUBO.lights[i].pos = lightValues[i].pos;
			passUBO.lights[i].color = lightValues[i].color;
			passUBO.lights[i].radius = lightValues[i].radius;
		}
		lightingPass->WriteToBuffer(&passUBO);
		lightingPass->Flush();

		//for (unsigned i = 0; i < MAX_LIGHTS; ++i)
		//{
		//	editorVars.ambient[i] = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
		//	editorVars.diffuse[i] = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
		//	editorVars.specular[i] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		//}

		return 0;
	}

	int DeferredScene::PreUpdate()
	{
		ImGui::Text("Camera X: %f", c.cameraPos.x);
		ImGui::Text("Camera Y: %f", c.cameraPos.y);
		ImGui::Text("Camera Z: %f", c.cameraPos.z);

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

		LightPassUBO passUBO{};
		passUBO.eyePos = glm::vec4(c.cameraPos, 1.0f);

		for (unsigned i = 0; i < MAX_LIGHTS; ++i)
		{
			passUBO.lights[i].pos = lightValues[i].pos;
			passUBO.lights[i].color = lightValues[i].color;
			passUBO.lights[i].radius = lightValues[i].radius;
		}
		lightingPass->WriteToBuffer(&passUBO);
		lightingPass->Flush();

		for (auto& a : gameObjects)
		{
			auto& obj = a.second;
			obj.GetTransform().Update();
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
		SceneInfo lighting(GetDescriptorSet("Lighting"), GetGameObjects());
		SceneInfo geometry(GetDescriptorSet("Geometry"), GetGameObjects());
		SceneInfo lights(GetDescriptorSet("LocalLights"), localLights);

		std::vector<VkClearValue> clearValues(4);
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[3].depthStencil = { 1.0f, 0 };

		BeginRenderPass(buf, "Geometry", clearValues);
		renderSystems["Geometry"][0].get()->Render(f, geometry);
		EndRenderPass(buf);

		//BeginRenderPass(buf, "Lighting", clearValues);
		//renderSystems["Lighting"][0].get()->Render(f, lighting);
		//EndRenderPass(buf);

		// Render the actual scene (swapchain)
		BeginSwapChainRenderPass(buf);

		renderSystems["Lighting"][0].get()->Render(f, lighting);
		renderSystems["LocalLights"][0].get()->Render(f, lights);

		return 0;
	}

	void DeferredScene::LoadGameObjects()
	{
		for (int i = 0; i < 6; ++i)
		{
			std::shared_ptr<Model> model = Model::CreateModelFromFile(device, Model::Type::OBJ,
				"Materials/Models/BA/Misaki/Mesh/Misaki_Original_Weapon.obj",
				"Materials/Models/BA/Misaki/Mesh/Texture2D/", true);

			auto whiteFang = GameObject::CreateGameObject("Model", "WhiteFang556");
			whiteFang.SetModel(model);
			whiteFang.GetTransform().SetTranslation(glm::vec3((i % 3) * 10.0f + 0.f, 0.0f, i >= 3 ? 0.0f : 5.0f));
			whiteFang.GetTransform().SetRotation(glm::vec3(0.0f, 0.5f, 0.0f));
			whiteFang.GetTransform().SetScale(glm::vec3(10.0f));

			gameObjects.emplace(whiteFang.GetID(), std::move(whiteFang));
		}

		for (int i = 0; i < MAX_LIGHTS; ++i)
		{
			float xPos = RandomNum(-20.0f, 20.0f);
			float yPos = RandomNum(-10.0f, 10.0f);
			float zPos = RandomNum(-10.0f, 10.0f);

			float r = RandomNum(0.0f, 1.0f);
			float g = RandomNum(0.0f, 1.0f);
			float b = RandomNum(0.0f, 1.0f);

			lightValues[i].pos = glm::vec4(xPos, yPos, zPos, 1.0f);
			lightValues[i].color = glm::vec3(r, g, b);
			lightValues[i].radius = 3.0f;

			std::shared_ptr<Model> light = Model::CreateModelFromFile(device, Model::Type::OBJ, "Materials/Models/sphere.obj");
			
			auto cube = GameObject::CreateGameObject("Light", "CubeLight");
			cube.SetModel(light);
			cube.GetTransform().SetTranslation(glm::vec3(lightValues[i].pos));
			cube.GetTransform().SetRotation(glm::vec3(0.0f, 0.5f, 0.0f));
			cube.GetTransform().SetScale(glm::vec3(0.1f));
			
			localLights.emplace(cube.GetID(), std::move(cube));
		}
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

		lightingPass = std::make_unique<UniformBuffer<LightPassUBO>>(
			UBO::Type::LIGHTPASS,
			device,
			UBO::SizeofUBO(UBO::Type::LIGHTPASS),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
		lightingPass->Map();
	}

	void DeferredScene::CreateSetLayouts()
	{
		textures.push_back(std::make_unique<Texture>(device, "Materials/Models/BA/Misaki/Texture2D/Misaki_Original_Weapon.png"));
		auto texInfo = textures[0]->DescriptorInfo();

		setLayouts["Geometry"] = DescriptorSetLayout::Builder(device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			//.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			//.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			//.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			//.AddBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
			.Build();

		setLayouts["Lighting"] = DescriptorSetLayout::Builder(device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			//.AddBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
			//.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build();

		setLayouts["LocalLights"] = DescriptorSetLayout::Builder(device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			//.AddBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
			//.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build();

		auto worldBuf = worldUBO->DescriptorInfo();
		auto lightPassBuf = lightingPass->DescriptorInfo();

		//auto whiteFangTex = textures[0]->DescriptorInfo();
		//auto hoshino = textures[1]->DescriptorInfo();
		//auto skyboxTex = textures[2]->DescriptorInfo();
		//auto emptyMap = textures[3]->DescriptorInfo();

		descriptorSets["Geometry"].resize(1);
		descriptorSets["Lighting"].resize(1);
		descriptorSets["LocalLights"].resize(1);

		// Object set
		DescriptorWriter(*setLayouts["Geometry"], *globalPool)
			.WriteBuffer(0, &worldBuf)
			.WriteImage(1, &texInfo)
			//.WriteBuffer(1, &lightBuf)
			//.WriteImage(2, &emptyMap)
			.Build(descriptorSets["Geometry"][0]);


		auto posTex = VkDescriptorImageInfo{ renderPasses["Geometry"].sampler,
			renderPasses["Geometry"].position.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		auto normalTex = VkDescriptorImageInfo{ renderPasses["Geometry"].sampler,
			renderPasses["Geometry"].normal.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		auto albedoTex = VkDescriptorImageInfo{ renderPasses["Geometry"].sampler,
			renderPasses["Geometry"].albedo.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		DescriptorWriter(*setLayouts["Lighting"], *globalPool)
			.WriteImage(0, &posTex)
			.WriteImage(1, &normalTex)
			.WriteImage(2, &albedoTex)
			.WriteBuffer(3, &lightPassBuf)
			//.WriteBuffer(1, &lightBuf)
			//.WriteImage(2, &emptyMap)
			.Build(descriptorSets["Lighting"][0]);

		DescriptorWriter(*setLayouts["LocalLights"], *globalPool)
			.WriteBuffer(0, &worldBuf)
			.WriteImage(1, &posTex)
			.WriteImage(2, &normalTex)
			.WriteImage(3, &albedoTex)
			.WriteBuffer(4, &lightPassBuf)
			//.WriteBuffer(1, &lightBuf)
			//.WriteImage(2, &emptyMap)
			.Build(descriptorSets["LocalLights"][0]);
	}

	void DeferredScene::CreateRenderPasses()
	{
		renderPasses["Geometry"] = device.CreateDeferredPass(
			swapChain.get()->GetSwapChainExtent().width,
			swapChain.get()->GetSwapChainExtent().height);

		renderPasses["Lighting"] = device.CreateRenderPass(
			swapChain.get()->GetSwapChainExtent().width,
			swapChain.get()->GetSwapChainExtent().height);
	}

	void DeferredScene::CreateRenderSystems()
	{
		renderSystems["Geometry"].reserve(1);
		renderSystems["Lighting"].reserve(1);
		renderSystems["LocalLights"].reserve(1);

		renderSystems["Geometry"].emplace_back(std::make_unique<GeometrySystem>(
			device,
			renderPasses["Geometry"].renderPass,
			GetSetLayout("Geometry")->GetDescriptorSetLayout()
		));

		renderSystems["Lighting"].emplace_back(std::make_unique<DeferredSystem>(
			device,
			GetSwapChainRenderPass(),
			GetSetLayout("Lighting")->GetDescriptorSetLayout()
			));

		renderSystems["LocalLights"].emplace_back(std::make_unique<LocalLightSystem>(
			device,
			GetSwapChainRenderPass(),
			GetSetLayout("LocalLights")->GetDescriptorSetLayout()
			));
	}
}