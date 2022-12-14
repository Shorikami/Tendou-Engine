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
}