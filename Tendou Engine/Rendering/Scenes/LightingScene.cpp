#include "LightingScene.h"

#include <array>

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
		CreateOffscreen();
	}

	LightingScene::~LightingScene()
	{
		// Frame buffer

		// Color attachment
		vkDestroyImageView(device.Device(), offscreenPass.color.view, nullptr);
		vkDestroyImage(device.Device(), offscreenPass.color.image, nullptr);
		vkFreeMemory(device.Device(), offscreenPass.color.mem, nullptr);

		// Depth attachment
		vkDestroyImageView(device.Device(), offscreenPass.depth.view, nullptr);
		vkDestroyImage(device.Device(), offscreenPass.depth.image, nullptr);
		vkFreeMemory(device.Device(), offscreenPass.depth.mem, nullptr);

		vkDestroyRenderPass(device.Device(), offscreenPass.renderPass, nullptr);
		vkDestroySampler(device.Device(), offscreenPass.sampler, nullptr);
		vkDestroyFramebuffer(device.Device(), offscreenPass.frameBuffer, nullptr);
	}

	// TODO: Abstract all this later
	void LightingScene::CreateOffscreen()
	{
		offscreenPass.width = 1280;
		offscreenPass.height = 720;

		// Find a suitable depth format
		VkFormat fbDepthFormat = device.FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = offscreenPass.width;
		imageInfo.extent.height = offscreenPass.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		device.CreateImage(offscreenPass.width, offscreenPass.height, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenPass.color.image, offscreenPass.color.mem);

		offscreenPass.color.view = device.CreateImageView(offscreenPass.color.image, VK_FORMAT_R8G8B8A8_UNORM);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		if (vkCreateSampler(device.Device(), &samplerInfo, nullptr, &offscreenPass.sampler) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create texture sampler!");
		}

		// Depth stencil attachment
		device.CreateImage(offscreenPass.width, offscreenPass.height, fbDepthFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenPass.depth.image, offscreenPass.depth.mem);

		offscreenPass.depth.view = device.CreateImageView(offscreenPass.depth.image, 
			fbDepthFormat, 1, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT);

		//VkImageViewCreateInfo depthStencilView = vks::initializers::imageViewCreateInfo();
		//depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		//depthStencilView.format = fbDepthFormat;
		//depthStencilView.flags = 0;
		//depthStencilView.subresourceRange = {};
		//depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		//if (fbDepthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
		//	depthStencilView.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		//}
		//depthStencilView.subresourceRange.baseMipLevel = 0;
		//depthStencilView.subresourceRange.levelCount = 1;
		//depthStencilView.subresourceRange.baseArrayLayer = 0;
		//depthStencilView.subresourceRange.layerCount = 1;
		//depthStencilView.image = offscreenPass.depth.image;
		//VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &offscreenPass.depth.view));

		// Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering

		std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
		// Color attachment
		attchmentDescriptions[0].format = VK_FORMAT_R8G8B8A8_UNORM;
		attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		// Depth attachment
		attchmentDescriptions[1].format = fbDepthFormat;
		attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attchmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.pDepthStencilAttachment = &depthReference;

		// Use subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Create the actual renderpass
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
		renderPassInfo.pAttachments = attchmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		vkCreateRenderPass(device.Device(), &renderPassInfo, nullptr, &offscreenPass.renderPass);

		renderPasses["Offscreen"] = offscreenPass.renderPass;

		VkImageView attachments[2];
		attachments[0] = offscreenPass.color.view;
		attachments[1] = offscreenPass.depth.view;

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = offscreenPass.renderPass;
		framebufferInfo.attachmentCount = 2;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = offscreenPass.width;
		framebufferInfo.height = offscreenPass.height;
		framebufferInfo.layers = 1;

		vkCreateFramebuffer(device.Device(), &framebufferInfo, nullptr, &offscreenPass.frameBuffer);

		// Fill a descriptor for later use in a descriptor set
		offscreenPass.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		offscreenPass.descriptor.imageView = offscreenPass.color.view;
		offscreenPass.descriptor.sampler = offscreenPass.sampler;
	}

	// TODO: very gross; abstract this
	int LightingScene::Render(VkCommandBuffer buf, DefaultSystem& test, FrameInfo& f)
	{
		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = offscreenPass.renderPass;
		renderPassInfo.framebuffer = offscreenPass.frameBuffer;
		renderPassInfo.renderArea.extent.width = offscreenPass.width;
		renderPassInfo.renderArea.extent.height = offscreenPass.height;
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(commandBuffers[currFrameIdx], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(offscreenPass.width);
		viewport.height = static_cast<float>(offscreenPass.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(buf, 0, 1, &viewport);

		VkExtent2D extent{ offscreenPass.width, offscreenPass.height };
		VkRect2D scissor{ {0, 0}, extent };
		vkCmdSetScissor(buf, 0, 1, &scissor);

		std::vector<VkDescriptorSet> testSets;
		testSets.resize(3);

		testSets[0] = globalDescriptorSets[2];
		testSets[1] = globalDescriptorSets[1];
		testSets[2] = globalDescriptorSets[0];

		FrameInfo newFrame(f.frameIdx, f.frameTime, f.commandBuffer, 
			f.cam, testSets, f.gameObjects);

		test.Render(newFrame);

		vkCmdEndRenderPass(buf);
		return 0;
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

		lightUBO = std::make_unique<UniformBuffer<LightsUBO>>(
			device,
			sizeof(LightsUBO),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		lightUBO->Map();

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

		globalSetLayout = DescriptorSetLayout::Builder(device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build();

		auto bufInfo = worldUBO->DescriptorInfo();
		auto bufInfo2 = lightUBO->DescriptorInfo();
		auto texInfo = textures[0]->DescriptorInfo();
		auto texInfo2 = textures[1]->DescriptorInfo();
		auto texInfo3 = textures[2]->DescriptorInfo();
		auto texInfo4 = VkDescriptorImageInfo{
			offscreenPass.descriptor.sampler, 
			offscreenPass.descriptor.imageView,
			offscreenPass.descriptor.imageLayout };

		globalDescriptorSets.resize(3);

		// Object set
		DescriptorWriter(*globalSetLayout, *globalPool)
			.WriteBuffer(0, &bufInfo)
			.WriteBuffer(1, &bufInfo2)
			.WriteImage(2, &texInfo4)
			.Build(globalDescriptorSets[0]);

		// Skybox set
		DescriptorWriter(*globalSetLayout, *globalPool)
			.WriteBuffer(0, &bufInfo)
			//.WriteBuffer(1, &bufInfo2)
			.WriteImage(2, &texInfo2)
			.WriteImage(3, &texInfo3)
			.Build(globalDescriptorSets[1]);

		// Offscreen set
		DescriptorWriter(*globalSetLayout, *globalPool)
			.WriteBuffer(0, &bufInfo)
			.WriteBuffer(1, &bufInfo2)
			.WriteImage(2, &texInfo2)
			.Build(globalDescriptorSets[2]);

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
			if (obj.GetTag() == "Sphere")
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

	void LightingScene::LoadGameObjects()
	{
		std::shared_ptr<Model> model = Model::CreateModelFromFile(device, Model::Type::OBJ,
			"Materials/Models/smooth_vase.obj");

		auto whiteFang = GameObject::CreateGameObject("Object");
		whiteFang.SetModel(model);
		whiteFang.GetTransform().SetTranslation(glm::vec3(0.f));
		whiteFang.GetTransform().SetRotation(glm::vec3(0.0f, 0.5f, 0.0f));
		whiteFang.GetTransform().SetScale(glm::vec3(5.5f));

		gameObjects.emplace(whiteFang.GetID(), std::move(whiteFang));


		model = Model::CreateModelFromFile(device, Model::Type::OBJ,
			"Materials/Models/sphere.obj", std::string(), true);

		for (unsigned i = 0; i < MAX_LIGHTS; ++i)
		{
			auto sphere = GameObject::CreateGameObject("Sphere");
			sphere.SetModel(model);
			sphere.GetTransform().SetTranslation(glm::vec3(0.0f, 0.0f, editorVars.sphereLineRad));
			sphere.GetTransform().SetRotation(glm::vec3(0.0f, 1.0f, 0.0f));
			sphere.GetTransform().SetScale(glm::vec3(0.08f));

			gameObjects.emplace(sphere.GetID(), std::move(sphere));
		}

		model = Model::CreateModelFromFile(device, Model::Type::OBJ,
			"Materials/Models/cube.obj", std::string(), true);

		auto skybox = GameObject::CreateGameObject("Skybox");
		skybox.SetModel(model);
		skybox.GetTransform().SetTranslation(glm::vec3(0.f));
		//skybox.GetTransform().SetRotation(glm::vec3(0.0f, 0.5f, 0.0f));
		skybox.GetTransform().SetScale(glm::vec3(50.0f));

		gameObjects.emplace(skybox.GetID(), std::move(skybox));
	}
}