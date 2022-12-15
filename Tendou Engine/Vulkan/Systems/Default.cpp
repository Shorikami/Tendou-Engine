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

	void DefaultSystem::Render(FrameInfo& frame)
	{
		static float angle = 0.0f;
		int count = 0, ii = 0;

		for (auto& kv : frame.gameObjects)
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
			
			// TODO: Move this out to a separate update loop?
			if (obj.GetTag() == "Sphere")
			{
				RenderSpheres(obj, frame, angle, ii++);
			}
			else
			{
				RenderObject(obj, frame);
			}
		}

		angle += 0.01f;
		if (angle > glm::pi<float>())
		{
			angle = 0.0f;
		}
	}

	void DefaultSystem::RenderObject(GameObject& obj, FrameInfo& f)
	{
		pipeline[0]->Bind(f.commandBuffer);
		obj.GetTransform().Update();

		vkCmdBindDescriptorSets(f.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			layout, 0, 1, &f.descriptorSets[0],
			0, nullptr);

		obj.GetModel()->Bind(f.commandBuffer);
		obj.GetModel()->Draw(f.commandBuffer);
	}

	void DefaultSystem::RenderSpheres(GameObject& obj, FrameInfo& f, float angle, int idx)
	{
		pipeline[1]->Bind(f.commandBuffer);

		float res = angle + (glm::pi<float>() / 8.0f) * idx;
		obj.GetTransform().SetRotationAngle(res);
		obj.GetTransform().Update(true);

		obj.GetModel()->Bind(f.commandBuffer);
		obj.GetModel()->Draw(f.commandBuffer);
	}
}