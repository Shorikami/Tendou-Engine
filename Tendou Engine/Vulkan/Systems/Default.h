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

		void Render(FrameInfo& frame, SceneInfo& scene) override;

	protected:
		void CreatePipelineLayout(VkDescriptorSetLayout v) override;
		void CreatePipeline(VkRenderPass pass) override;
	
	private:
		void RenderObject(GameObject& obj, SceneInfo& s, VkCommandBuffer buf);
		void RenderSpheres(GameObject& obj, SceneInfo& s, VkCommandBuffer buf);
		void RenderSkybox(GameObject& obj, SceneInfo& s, VkCommandBuffer buf);
	};
}

#endif