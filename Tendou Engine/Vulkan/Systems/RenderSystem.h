#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H

#include "../Pipeline.h"
#include "../TendouDevice.h"

#include "../../Components/GameObject.h"
#include "../../Rendering/FrameInfo.h"
#include "../../Rendering/Camera.h"

#include <memory>
#include <vector>

namespace Tendou
{
	class RenderSystem
	{
	public:

		RenderSystem(TendouDevice& device);
		~RenderSystem();

		RenderSystem(const RenderSystem&) = delete;
		RenderSystem& operator=(const RenderSystem&) = delete;

		virtual void Render(FrameInfo& frame);

	protected:
		virtual void CreatePipelineLayout(VkDescriptorSetLayout v);
		virtual void CreatePipeline(VkRenderPass pass);

		TendouDevice& device;
		std::vector<std::unique_ptr<Pipeline>> pipeline;
		VkPipelineLayout layout;
	};
}

#endif