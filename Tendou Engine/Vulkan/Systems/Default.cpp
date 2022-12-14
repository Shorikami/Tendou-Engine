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

		pipeline.push_back(std::make_unique<Pipeline>(device,
			"Materials/Shaders/SimpleShader.vert.spv",
			"Materials/Shaders/SimpleShader.frag.spv",
			pipelineConfig));

		pipeline.push_back(std::make_unique<Pipeline>(device,
			"Materials/Shaders/Test.vert.spv",
			"Materials/Shaders/Test.frag.spv",
			pipelineConfig));
	}

	void DefaultSystem::RenderGameObjects(FrameInfo& frame)
	{
		pipeline[0]->Bind(frame.commandBuffer);
		int count = 0;

		for (auto& kv : frame.gameObjects)
		{
			int ii = 0;
			auto& obj = kv.second;
			if (obj.GetModel() == nullptr)
			{
				continue;
			}

			// TODO: Move this out to a separate update loop
			obj.GetTransform().Update();

			PushConstantData push{};
			push.modelMatrix = obj.GetTransform().ModelMat();
			push.normalMatrix = obj.GetTransform().NormalMatrix();

			vkCmdPushConstants(frame.commandBuffer,
				layout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PushConstantData),
				&push);

			vkCmdBindDescriptorSets(frame.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				layout, 0, 1, &frame.descriptorSets[count],
				0, nullptr);

			if (count < frame.descriptorSets.size() - 1)
			{
				++count;
			}

			obj.GetModel()->Bind(frame.commandBuffer);
			obj.GetModel()->Draw(frame.commandBuffer);
		}
	}
}