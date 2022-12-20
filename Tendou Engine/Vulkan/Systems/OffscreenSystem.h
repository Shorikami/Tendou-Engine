#ifndef OFFSCREENSYSTEM_H
#define OFFSCREENSYSTEM_H

#include "RenderSystem.h"

namespace Tendou
{
	class OffscreenSystem : public RenderSystem
	{
	public:
		OffscreenSystem(TendouDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);

		OffscreenSystem(const OffscreenSystem&) = delete;
		OffscreenSystem& operator=(const OffscreenSystem&) = delete;

		void Render(FrameInfo& frame) override;

	protected:
		void CreatePipelineLayout(VkDescriptorSetLayout v) override;
		void CreatePipeline(VkRenderPass pass) override;
	
	private:
		void RenderSpheres(GameObject& obj, FrameInfo& f);
		void RenderSkybox(GameObject& obj, FrameInfo& f);
	};
}

#endif