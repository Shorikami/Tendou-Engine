#include "LightingScene.h"

namespace Tendou
{
	LightingScene::LightingScene(Window& window, TendouDevice& device)
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
	LightingScene::~LightingScene()
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

	int LightingScene::Init()
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

	int LightingScene::PreUpdate()
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

	int LightingScene::Update()
	{
		static float angle = 0.0f;
		int idx = 0;

		LightsUBO lightUbo{};
		lightUbo.eyePos = glm::vec4(c.cameraPos, 1.0f);

		for (unsigned i = 0; i < editorVars.currLights; ++i)
		{
			lightUbo.lightColor[i] = glm::vec4(1.0f);

			lightUbo.ambient[i] = glm::vec4(editorVars.ambient[i], 1.0f);
			lightUbo.diffuse[i] = glm::vec4(editorVars.diffuse[i], 1.0f);
			lightUbo.specular[i] = glm::vec4(editorVars.specular[i], 1.0f);

			// x = outer, y = inner, z = falloff, w = type
			lightUbo.lightInfo[i] = glm::vec4(80.0f, 45.0f, 10.0f, 0.0f);
		}

		lightUbo.emissive = glm::vec4(editorVars.emissive, 1.0f);
		lightUbo.attenuation = editorVars.attenuation;

		lightUbo.coefficients = glm::vec4(editorVars.lightCoeffs, 1.0f);
		lightUbo.fogColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		lightUbo.numLights = editorVars.currLights;

		// x = use gpu, y = use normals, z = uv type
		lightUbo.modes = glm::ivec4(0, 0, 0, 0);

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
		localUBO.nearFar = glm::vec2(editorVars.nearFar.x, editorVars.nearFar.y);
		worldUBO->WriteToBuffer(&localUBO);
		worldUBO->Flush();

		//lightUbo.lightColor[0] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		//lightUbo.lightPos[0] = glm::vec4(-2.0f, -1.0f, 1.0f, 1.0f);
		lightUBO->WriteToBuffer(&lightUbo);
		lightUBO->Flush();

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

	int LightingScene::PostUpdate()
	{
		return 0;
	}

	int LightingScene::Render(VkCommandBuffer buf, FrameInfo& f)
	{
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

		SceneInfo offscreen(GetDescriptorSet("Offscreen"), GetGameObjects());
		SceneInfo global(GetDescriptorSet("Global"), GetGameObjects());

		// Offscreen render test
		for (int i = 0; i < 6; ++i)
		{
			BeginRenderPass(buf, std::string("Offscreen") + std::to_string(i + 1));
			glm::vec3 objPos = gameObjects.find(0)->second.GetTransform().PositionVec3();
			WriteToCaptureUBO(glm::lookAt(objPos, directionLookup[i], -upLookup[i]), i);
			f.dynamicOffset = testOffset * i;

			renderSystems["Offscreen"][i].get()->Render(f, offscreen);
			EndRenderPass(buf);
		}

		BeginSwapChainRenderPass(buf);

		renderSystems["Global"][0].get()->Render(f, global);

		return 0;
	}

	void LightingScene::WriteToCaptureUBO(glm::mat4 view, int i)
	{
		RenderUBO localUBO;
		localUBO.proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);
		localUBO.view = dynamic_cast<UniformBuffer<RenderUBO>*>(captureUBO.get())->GetData().view;
		*localUBO.view = view;
		captureUBO->WriteToBuffer(&localUBO.proj, sizeof(glm::mat4), (captureUBO->GetAlignmentSize() * i));
		captureUBO->WriteToBuffer(localUBO.view, sizeof(glm::mat4), (captureUBO->GetAlignmentSize() * i) + sizeof(glm::mat4));
		captureUBO->Flush();
	}

	void LightingScene::LoadGameObjects()
	{
		std::shared_ptr<Model> model = Model::CreateModelFromFile(device, Model::Type::OBJ,
			"Materials/Models/smooth_vase.obj");

		auto whiteFang = GameObject::CreateGameObject("TextureTarget", "Vase");
		whiteFang.SetModel(model);
		whiteFang.GetTransform().SetTranslation(glm::vec3(0.f));
		whiteFang.GetTransform().SetRotation(glm::vec3(0.0f, 0.5f, 0.0f));
		whiteFang.GetTransform().SetScale(glm::vec3(5.5f));

		gameObjects.emplace(whiteFang.GetID(), std::move(whiteFang));


		model = Model::CreateModelFromFile(device, Model::Type::OBJ,
			"Materials/Models/sphere.obj", std::string(), true);

		for (unsigned i = 0; i < MAX_LIGHTS; ++i)
		{
			auto sphere = GameObject::CreateGameObject("Light", "Sphere");
			sphere.SetModel(model);
			sphere.GetTransform().SetTranslation(glm::vec3(0.0f, 0.0f, editorVars.sphereLineRad));
			sphere.GetTransform().SetRotation(glm::vec3(0.0f, 1.0f, 0.0f));
			sphere.GetTransform().SetScale(glm::vec3(0.08f));

			gameObjects.emplace(sphere.GetID(), std::move(sphere));
		}

		model = Model::CreateModelFromFile(device, Model::Type::OBJ,
			"Materials/Models/cube.obj", std::string(), true);

		auto skybox = GameObject::CreateGameObject("Skybox", "Sky");
		skybox.SetModel(model);
		skybox.GetTransform().SetTranslation(glm::vec3(0.f));
		//skybox.GetTransform().SetRotation(glm::vec3(0.0f, 0.5f, 0.0f));
		skybox.GetTransform().SetScale(glm::vec3(50.0f));

		gameObjects.emplace(skybox.GetID(), std::move(skybox));
	}

	void LightingScene::CreateUBOs()
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

		captureUBO = std::make_unique<UniformBuffer<RenderUBO>>(
			UBO::Type::CAPTURE,
			device,
			UBO::SizeofUBO(UBO::Type::CAPTURE),
			6,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			device.properties.limits.minUniformBufferOffsetAlignment
			);
		captureUBO->Map();
		captureUBO->GetData().view = (glm::mat4*)captureUBO->AlignedAlloc(captureUBO->GetBufferSize(), captureUBO->GetAlignmentSize());
		assert(captureUBO->GetData().view && "Failed to allocate capture UBO view!");

		testOffset = captureUBO->GetAlignmentSize();

	}

	void LightingScene::CreateSetLayouts()
	{
		std::string path = "Materials/Textures/skybox/skybox_";
		std::vector<std::string> faces =
		{
			path + std::string("right.png"),
			path + std::string("left.png"),
			path + std::string("top.png"),
			path + std::string("bottom.png"),
			path + std::string("front.png"),
			path + std::string("back.png"),
		};

		textures.push_back(std::make_unique<Texture>(device, "Materials/Models/Shiroko/Texture2D/Shiroko_Original_Weapon.png"));
		textures.push_back(std::make_unique<Texture>(device, "Materials/Textures/hoshino.png"));
		textures.push_back(std::make_unique<Texture>(device, faces));

		setLayouts["Global"] = DescriptorSetLayout::Builder(device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			//.AddBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
			.Build();

		setLayouts["Offscreen"] = DescriptorSetLayout::Builder(device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
			.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build();

		auto worldBuf = worldUBO->DescriptorInfo();
		auto lightBuf = lightUBO->DescriptorInfo();
		auto captureBuf = captureUBO->DescriptorInfo();
		captureBuf.range = captureUBO->GetAlignmentSize();

		auto texInfo = textures[0]->DescriptorInfo();
		auto texInfo2 = textures[1]->DescriptorInfo();
		auto texInfo3 = textures[2]->DescriptorInfo();
		auto texInfo4 = VkDescriptorImageInfo{
			renderPasses["Offscreen1"].descriptor.sampler,
			renderPasses["Offscreen1"].descriptor.imageView,
			renderPasses["Offscreen1"].descriptor.imageLayout };

		descriptorSets["Global"].resize(3);
		descriptorSets["Offscreen"].resize(1);

		// Object set
		DescriptorWriter(*setLayouts["Global"], *globalPool)
			.WriteBuffer(0, &worldBuf)
			.WriteBuffer(1, &lightBuf)
			.WriteImage(2, &texInfo4)
			.Build(descriptorSets["Global"][0]);

		// Skybox set
		DescriptorWriter(*setLayouts["Global"], *globalPool)
			.WriteBuffer(0, &worldBuf)
			//.WriteBuffer(1, &bufInfo2)
			.WriteImage(2, &texInfo2)
			.WriteImage(3, &texInfo3)
			.Build(descriptorSets["Global"][1]);

		// Offscreen set
		DescriptorWriter(*setLayouts["Global"], *globalPool)
			.WriteBuffer(0, &worldBuf)
			.WriteBuffer(1, &lightBuf)
			.WriteImage(2, &texInfo2)
			.Build(descriptorSets["Global"][2]);

		DescriptorWriter(*setLayouts["Offscreen"], *globalPool)
			.WriteBuffer(0, &worldBuf)
			.WriteBuffer(5, &captureBuf)
			.WriteImage(3, &texInfo3)
			.Build(descriptorSets["Offscreen"][0]);
	}

	void LightingScene::CreateRenderPasses()
	{
		renderPasses["Offscreen1"] = device.CreateRenderPass(
			swapChain.get()->GetSwapChainExtent().width,
			swapChain.get()->GetSwapChainExtent().height);

		renderPasses["Offscreen2"] = device.CreateRenderPass(
			swapChain.get()->GetSwapChainExtent().width,
			swapChain.get()->GetSwapChainExtent().height);

		renderPasses["Offscreen3"] = device.CreateRenderPass(
			swapChain.get()->GetSwapChainExtent().width,
			swapChain.get()->GetSwapChainExtent().height);

		renderPasses["Offscreen4"] = device.CreateRenderPass(
			swapChain.get()->GetSwapChainExtent().width,
			swapChain.get()->GetSwapChainExtent().height);

		renderPasses["Offscreen5"] = device.CreateRenderPass(
			swapChain.get()->GetSwapChainExtent().width,
			swapChain.get()->GetSwapChainExtent().height);

		renderPasses["Offscreen6"] = device.CreateRenderPass(
			swapChain.get()->GetSwapChainExtent().width,
			swapChain.get()->GetSwapChainExtent().height);
	}

	void LightingScene::CreateRenderSystems()
	{
		renderSystems["Global"].reserve(1);
		renderSystems["Offscreen"].reserve(6);

		renderSystems["Global"].emplace_back(std::make_unique<DefaultSystem>(
			device,
			GetSwapChainRenderPass(),
			GetSetLayout("Global")->GetDescriptorSetLayout()
		));

		for (unsigned i = 0; i < 6; ++i)
		{
			std::string name = std::string("Offscreen") + std::to_string(i + 1);
			renderSystems["Offscreen"].emplace_back(std::make_unique<OffscreenSystem>
			(
				device,
				renderPasses[name].renderPass,
				GetSetLayout("Offscreen")->GetDescriptorSetLayout()
			));
		}
	}
}