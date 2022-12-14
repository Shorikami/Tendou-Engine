#ifndef DEFAULT_H
#define DEFAULT_H

#include "RenderSystem.h"

namespace Tendou
{
	class DefaultSystem : public RenderSystem
	{
	public:
		DefaultSystem(TendouDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);

		DefaultSystem(const DefaultSystem&) = delete;
		DefaultSystem& operator=(const DefaultSystem&) = delete;

		void RenderGameObjects(FrameInfo& frame) override;

	protected:
		void CreatePipelineLayout(VkDescriptorSetLayout v) override;
		void CreatePipeline(VkRenderPass pass) override;
	};
}

#endif