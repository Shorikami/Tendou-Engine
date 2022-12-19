#include "RenderSystem.h"

namespace Tendou
{
	RenderSystem::RenderSystem(TendouDevice& device_)
		: device(device_)
		, layout(nullptr)
	{
	}

	RenderSystem::~RenderSystem()
	{
		vkDestroyPipelineLayout(device.Device(), layout, nullptr);
	}

	void RenderSystem::Render(FrameInfo& frame)
	{
		return;
	}

	void RenderSystem::CreatePipelineLayout(VkDescriptorSetLayout v)
	{
		return;
	}

	void RenderSystem::CreatePipeline(VkRenderPass pass)
	{
		return;
	}
}