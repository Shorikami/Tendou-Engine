#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H

#include "../Vulkan/Pipeline.h"
#include "../Vulkan/DrevisDevice.h"

#include "../Components/GameObject.h"

#include "FrameInfo.h"
#include "Camera.h"

#include <memory>
#include <vector>

namespace Drevis
{
	class RenderSystem
	{
	public:

		RenderSystem(DrevisDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~RenderSystem();

		RenderSystem(const RenderSystem&) = delete;
		RenderSystem& operator=(const RenderSystem&) = delete;

		void RenderGameObjects(FrameInfo& frame);

	private:
		void CreatePipelineLayout(VkDescriptorSetLayout v);
		void CreatePipeline(VkRenderPass pass);

		DrevisDevice& device;
		std::vector<std::unique_ptr<Pipeline>> pipeline;
		VkPipelineLayout layout;

	};
}

#endif