#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "RenderSystem.h"

namespace Tendou
{
	class GeometrySystem : public RenderSystem
	{
	public:
		GeometrySystem(TendouDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);

		GeometrySystem(const GeometrySystem&) = delete;
		GeometrySystem& operator=(const GeometrySystem&) = delete;

		void Render(FrameInfo& frame, SceneInfo& scene) override;

		uint32_t offset;

	protected:
		void CreatePipelineLayout(VkDescriptorSetLayout v) override;
		void CreatePipeline(VkRenderPass pass) override;
	};
}

#endif