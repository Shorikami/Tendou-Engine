#ifndef DEFERRED_H
#define DEFERRED_H

#include "RenderSystem.h"

namespace Tendou
{
	class DeferredSystem : public RenderSystem
	{
	public:
		DeferredSystem(TendouDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);

		DeferredSystem(const DeferredSystem&) = delete;
		DeferredSystem& operator=(const DeferredSystem&) = delete;

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