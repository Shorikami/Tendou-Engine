#include "RenderSystem.h"

namespace Tendou
{
	RenderSystem::RenderSystem(TendouDevice& device_)
		: device(device_)
		, layout(nullptr)
		, count(1)
	{
	}

	RenderSystem::~RenderSystem()
	{
		--count;
		if (count <= 0)
		{
			vkDestroyPipelineLayout(device.Device(), layout, nullptr);
		}
	}

	//RenderSystem::RenderSystem(const RenderSystem& other)
	//	: device(other.device)
	//	, pipeline(other.pipeline)
	//	, layout(other.layout)
	//	, count(other.count + 1)
	//{
	//}

	void RenderSystem::Render(FrameInfo& frame, SceneInfo& scene)
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