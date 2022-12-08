#include "RenderSystem.h"

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

	RenderSystem::RenderSystem(TendouDevice& device_, VkRenderPass pass, VkDescriptorSetLayout g)
		: device(device_)
	{
		CreatePipelineLayout(g);
		CreatePipeline(pass);
	}

	RenderSystem::~RenderSystem()
	{
		vkDestroyPipelineLayout(device.Device(), layout, nullptr);
	}

	void RenderSystem::CreatePipelineLayout(VkDescriptorSetLayout v)
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

	void RenderSystem::CreatePipeline(VkRenderPass pass)
	{
		assert(layout != nullptr && "Cannot create pipeline before layout!");

		PipelineConfigInfo pipelineConfig{};
		Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = pass;
		pipelineConfig.pipelineLayout = layout;

		pipeline.push_back(std::make_unique<Pipeline>(device,
			"Materials/Shaders/SimpleShader.vert.spv",
			"Materials/Shaders/SimpleShader.frag.spv",
			pipelineConfig));

		pipeline.push_back(std::make_unique<Pipeline>(device,
			"Materials/Shaders/Test.vert.spv",
			"Materials/Shaders/Test.frag.spv",
			pipelineConfig));
	}

	void RenderSystem::RenderGameObjects(FrameInfo& frame)
	{
		pipeline[0]->Bind(frame.commandBuffer);
		int count = 0;

		for (auto& kv : frame.gameObjects)
		{
			auto& obj = kv.second;
			if (obj.model == nullptr)
			{
				continue;
			}

			PushConstantData push{};
			push.modelMatrix = obj.Transform().Mat4();
			push.normalMatrix = obj.Transform().NormalMatrix();

			vkCmdPushConstants(frame.commandBuffer,
				layout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PushConstantData),
				&push);

			vkCmdBindDescriptorSets(frame.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				layout, 0, 1, &frame.descriptorSets[count++],
				0, nullptr);

			obj.model->Bind(frame.commandBuffer);
			obj.model->Draw(frame.commandBuffer);
		}
	}

}