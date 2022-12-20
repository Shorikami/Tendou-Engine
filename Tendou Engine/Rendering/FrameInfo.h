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
			VkCommandBuffer c, Camera& d, std::vector<VkDescriptorSet> e, 
			GameObject::Map& f, uint32_t g = 0)
			: frameIdx(a)
			, frameTime(b)
			, commandBuffer(c)
			, cam(d)
			, descriptorSets(e)
			, gameObjects(f)
			, dynamicOffset(g)
		{
		}

		int frameIdx;
		float frameTime;
		VkCommandBuffer commandBuffer;
		Camera& cam;
		std::vector<VkDescriptorSet> descriptorSets;
		GameObject::Map& gameObjects;
		uint32_t dynamicOffset;
	};
}

#endif