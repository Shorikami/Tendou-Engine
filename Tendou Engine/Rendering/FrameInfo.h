#ifndef FRAMEINFO_H
#define FRAMEINFO_H

#include "../Components/GameObject.h"
#include "Camera.h"
#include <vulkan/vulkan.h>

namespace Tendou
{
	struct FrameInfo
	{
		FrameInfo(int a, float b, 
			VkCommandBuffer c, uint32_t d = 0)
			: frameIdx(a)
			, frameTime(b)
			, commandBuffer(c)
			, dynamicOffset(d)
		{
		}

		int frameIdx;
		float frameTime;
		VkCommandBuffer commandBuffer;
		uint32_t dynamicOffset;
	};

	struct SceneInfo
	{
		SceneInfo(std::vector<VkDescriptorSet> a, GameObject::Map& b)
			: descriptorSets(a)
			, gameObjects(b)
		{
		}

		std::vector<VkDescriptorSet> descriptorSets;
		GameObject::Map& gameObjects;
	};
}

#endif