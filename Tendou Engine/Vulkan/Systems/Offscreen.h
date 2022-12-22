#ifndef OFFSCREEN_H
#define OFFSCREEN_H

#include "RenderSystem.h"

namespace Tendou
{
	class OffscreenSystem : public RenderSystem
	{
	public:
		OffscreenSystem(TendouDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);

		OffscreenSystem(const OffscreenSystem&) = delete;
		OffscreenSystem& operator=(const OffscreenSystem&) = delete;

		void Render(FrameInfo& frame, SceneInfo& scene) override;

		uint32_t offset;

	protected:
		void CreatePipelineLayout(VkDescriptorSetLayout v) override;
		void CreatePipeline(VkRenderPass pass) override;
	
	private:
		void RenderSpheres(GameObject& obj, VkCommandBuffer buf);
		void RenderSkybox(GameObject& obj, VkCommandBuffer buf);
	};
}

#endif