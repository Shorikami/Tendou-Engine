#ifndef FRAMEINFO_H
#define FRAMEINFO_H

#include "../Components/GameObject.h"
#include "Camera.h"
#include <vulkan/vulkan.h>

namespace Drevis
{
	struct FrameInfo
	{
		FrameInfo(int a, float b, VkCommandBuffer c, Camera& d, std::vector<VkDescriptorSet> e, GameObject::Map& f)
			: frameIdx(a)
			, frameTime(b)
			, commandBuffer(c)
			, cam(d)
			, descriptorSets(e)
			, gameObjects(f)
		{
		}

		int frameIdx;
		float frameTime;
		VkCommandBuffer commandBuffer;
		Camera& cam;
		std::vector<VkDescriptorSet> descriptorSets;
		GameObject::Map& gameObjects;
	};
}

#endif