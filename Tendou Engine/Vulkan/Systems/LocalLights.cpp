#include "LocalLights.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace Tendou
{
	struct LocalLightData
	{
		glm::mat4 modelMatrix{ 1.0f };
		glm::vec4 position{ 1.0f };
		glm::vec3 color{ 1.0f };
		float range = 0.0f;
	};

	LocalLightSystem::LocalLightSystem(TendouDevice& device, VkRenderPass pass, VkDescriptorSetLayout set)
		: RenderSystem(device)
	{
		CreatePipelineLayout(set);
		CreatePipeline(pass);
	}

	void LocalLightSystem::CreatePipelineLayout(VkDescriptorSetLayout v)
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(LocalLightData);

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

	void LocalLightSystem::CreatePipeline(VkRenderPass pass)
	{
		assert(layout != nullptr && "Cannot create pipeline before layout!");

		PipelineConfigInfo pipelineConfig{};
		Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
		Pipeline::EnableAlphaBlending(pipelineConfig);
		pipelineConfig.renderPass = pass;
		pipelineConfig.pipelineLayout = layout;

		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
		//pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;

		pipeline.push_back(std::make_shared<Pipeline>(device,
			"Materials/Shaders/LightingPassLight.vert.spv",
			"Materials/Shaders/LightingPassLight.frag.spv",
			pipelineConfig));
	}

	void LocalLightSystem::Render(FrameInfo& frame, SceneInfo& scene)
	{
		vkCmdBindDescriptorSets(frame.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			layout, 0, 1, &scene.descriptorSets[0],
			0, nullptr);
		int i = 1;
		for (auto& kv : scene.gameObjects)
		{
			auto& obj = kv.second;
			if (obj.GetModel() == nullptr)
			{
				continue;
			}

			LocalLightData push{};
			push.modelMatrix = obj.GetTransform().ModelMat();
			push.position = glm::vec4(1.0f * i);
			push.color = glm::vec3(0.1f * i);
			push.range = 10.0f;
			i += 1;
			// NOTE: RenderDoc push constant calls are coming from
			// the unrenderable lights
			vkCmdPushConstants(frame.commandBuffer,
				layout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(LocalLightData),
				&push);

			pipeline[0]->Bind(frame.commandBuffer);

			obj.GetModel()->Bind(frame.commandBuffer);
			obj.GetModel()->Draw(frame.commandBuffer);
		}
	}
}