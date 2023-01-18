#include "Geometry.h"

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

	GeometrySystem::GeometrySystem(TendouDevice& device, VkRenderPass pass, VkDescriptorSetLayout set)
		: RenderSystem(device)
	{
		CreatePipelineLayout(set);
		CreatePipeline(pass);
	}

	void GeometrySystem::CreatePipelineLayout(VkDescriptorSetLayout v)
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

	void GeometrySystem::CreatePipeline(VkRenderPass pass)
	{
		assert(layout != nullptr && "Cannot create pipeline before layout!");
		VkPipelineColorBlendAttachmentState state{};
		state.colorWriteMask = 0xf;
		state.blendEnable = false;

		std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates = 
		{
			state,
			state,
			state
		};

		PipelineConfigInfo pipelineConfig{};
		Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = pass;
		pipelineConfig.pipelineLayout = layout;
		pipelineConfig.colorBlendInfo.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
		pipelineConfig.colorBlendInfo.pAttachments = blendAttachmentStates.data();

		pipeline.push_back(std::make_shared<Pipeline>(device,
			"Materials/Shaders/GeometryPass.vert.spv",
			"Materials/Shaders/GeometryPass.frag.spv",
			pipelineConfig));
	}

	void GeometrySystem::Render(FrameInfo& frame, SceneInfo& scene)
	{
		vkCmdBindDescriptorSets(frame.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			layout, 0, 1, &scene.descriptorSets[0],
			0, nullptr);

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

			// NOTE: RenderDoc push constant calls are coming from
			// the unrenderable lights
			vkCmdPushConstants(frame.commandBuffer,
				layout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PushConstantData),
				&push);

			pipeline[0]->Bind(frame.commandBuffer);

			obj.GetModel()->Bind(frame.commandBuffer);
			obj.GetModel()->Draw(frame.commandBuffer);
		}
	}
}