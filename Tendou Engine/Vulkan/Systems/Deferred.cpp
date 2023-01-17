#include "Deferred.h"

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

	DeferredSystem::DeferredSystem(TendouDevice& device, VkRenderPass pass, VkDescriptorSetLayout set)
		: RenderSystem(device)
	{
		CreatePipelineLayout(set);
		CreatePipeline(pass);
	}

	void DeferredSystem::CreatePipelineLayout(VkDescriptorSetLayout v)
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

	void DeferredSystem::CreatePipeline(VkRenderPass pass)
	{
		assert(layout != nullptr && "Cannot create pipeline before layout!");

		PipelineConfigInfo pipelineConfig{};
		Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = pass;
		pipelineConfig.pipelineLayout = layout;

		pipeline.push_back(std::make_shared<Pipeline>(device,
			"Materials/Shaders/Test2.vert.spv",
			"Materials/Shaders/Test2.frag.spv",
			pipelineConfig));

		pipeline.push_back(std::make_shared<Pipeline>(device,
			"Materials/Shaders/Skybox2.vert.spv",
			"Materials/Shaders/Skybox2.frag.spv",
			pipelineConfig));
	}

	void DeferredSystem::Render(FrameInfo& frame, SceneInfo& scene)
	{
		int count = 0;
		vkCmdBindDescriptorSets(frame.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			layout, 0, 1, &scene.descriptorSets[0],
			1, &frame.dynamicOffset);


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

			// TODO: Make this a switch statement (tagging optimizations)
			// NOTE: Do not render the object from which we are doing
			// dynamic reflections
			if (obj.GetTag() == "Light" && obj.GetRender())
			{
				RenderSpheres(obj, frame.commandBuffer);
			}
			else if (obj.GetTag() == "Skybox")
			{
				RenderSkybox(obj, frame.commandBuffer);
			}
		}
	}

	void DeferredSystem::RenderSpheres(GameObject& obj, VkCommandBuffer buf)
	{
		pipeline[0]->Bind(buf);

		obj.GetModel()->Bind(buf);
		obj.GetModel()->Draw(buf);
	}

	void DeferredSystem::RenderSkybox(GameObject& obj, VkCommandBuffer buf)
	{
		pipeline[1]->Bind(buf);

		obj.GetModel()->Bind(buf);
		obj.GetModel()->Draw(buf);
	}
}