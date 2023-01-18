#ifndef LOCALLIGHTS_H
#define LOCALLIGHTS_H

#include "RenderSystem.h"

namespace Tendou
{
	class LocalLightSystem : public RenderSystem
	{
	public:
		LocalLightSystem(TendouDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);

		LocalLightSystem(const LocalLightSystem&) = delete;
		LocalLightSystem& operator=(const LocalLightSystem&) = delete;

		void Render(FrameInfo& frame, SceneInfo& scene) override;

	protected:
		void CreatePipelineLayout(VkDescriptorSetLayout v) override;
		void CreatePipeline(VkRenderPass pass) override;
	};
}

#endif