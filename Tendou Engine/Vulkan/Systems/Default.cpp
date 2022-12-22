#include "Default.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace Tendou
{
	struct PushConstantData
	{
		glm::mat4 modelMatrix{ 1.0f };
		glm::mat4 normalMatrix{ 1.0f };
	};

	DefaultSystem::DefaultSystem(TendouDevice& device, VkRenderPass pass, VkDescriptorSetLayout set)
		: RenderSystem(device)
	{
		CreatePipelineLayout(set);
		CreatePipeline(pass);
	}

	void DefaultSystem::CreatePipelineLayout(VkDescriptorSetLayout v)
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ v };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device.Device(), &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline layout!");
		}
	}

	void DefaultSystem::CreatePipeline(VkRenderPass pass)
	{
		assert(layout != nullptr && "Cannot create pipeline before layout!");

		PipelineConfigInfo pipelineConfig{};
		Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = pass;
		pipelineConfig.pipelineLayout = layout;

		pipeline.push_back(std::make_shared<Pipeline>(device,
			"Materials/Shaders/Lighting.vert.spv",
			"Materials/Shaders/Lighting.frag.spv",
			pipelineConfig));

		pipeline.push_back(std::make_shared<Pipeline>(device,
			"Materials/Shaders/Test.vert.spv",
			"Materials/Shaders/Test.frag.spv",
			pipelineConfig));

		pipeline.push_back(std::make_shared<Pipeline>(device,
			"Materials/Shaders/Skybox.vert.spv",
			"Materials/Shaders/Skybox.frag.spv",
			pipelineConfig));
	}

	void DefaultSystem::Render(FrameInfo& frame, SceneInfo& scene)
	{
		int count = 0;

		for (auto& kv : scene.gameObjects)
		{
			auto& obj = kv.second;
			if (obj.GetModel() == nullptr)
			{
				continue;
			}

			PushConstantData push{};
			push.modelMatrix = obj.GetTransform().ModelMat();
			push.normalMatrix = obj.GetTransform().NormalMatrix();

			vkCmdPushConstants(frame.commandBuffer,
				layout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PushConstantData),
				&push);

			// TODO: Make this a switch statement (tagging optimizations)
			if (obj.GetTag() == "Light" && obj.GetRender())
			{
				RenderSpheres(obj, scene, frame.commandBuffer);
			}
			else if (obj.GetTag() != "Light")
			{
				//RenderObject(obj, frame);
				if (obj.GetTag() == "Skybox")
				{
					RenderSkybox(obj, scene, frame.commandBuffer);
				}
				else
				{
					RenderObject(obj, scene, frame.commandBuffer);
				}
				
			}
		}
	}

	void DefaultSystem::RenderObject(GameObject& obj, SceneInfo& f, VkCommandBuffer buf)
	{
		pipeline[0]->Bind(buf);

		vkCmdBindDescriptorSets(buf,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			layout, 0, 1, &f.descriptorSets[0],
			0, nullptr);

		obj.GetModel()->Bind(buf);
		obj.GetModel()->Draw(buf);
	}

	void DefaultSystem::RenderSpheres(GameObject& obj, SceneInfo& f, VkCommandBuffer buf)
	{
		pipeline[1]->Bind(buf);

		obj.GetModel()->Bind(buf);
		obj.GetModel()->Draw(buf);
	}

	void DefaultSystem::RenderSkybox(GameObject& obj, SceneInfo& f, VkCommandBuffer buf)
	{
		pipeline[2]->Bind(buf);

		vkCmdBindDescriptorSets(buf,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			layout, 0, 1, &f.descriptorSets[1],
			0, nullptr);

		obj.GetModel()->Bind(buf);
		obj.GetModel()->Draw(buf);
	}
}