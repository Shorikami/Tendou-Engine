#ifndef FRAMEINFO_H
#define FRAMEINFO_H

#include "../Components/GameObject.h"
#include "Camera.h"
#include <vulkan/vulkan.h>

namespace Drevis
{
	struct FrameInfo
	{
		FrameInfo(int a, float b, VkCommandBuffer c, Camera& d, VkDescriptorSet e, GameObject::Map& f)
			: frameIdx(a)
			, frameTime(b)
			, commandBuffer(c)
			, cam(d)
			, globalDescriptorSet(e)
			, gameObjects(f)
		{
		}

		int frameIdx;
		float frameTime;
		VkCommandBuffer commandBuffer;
		Camera& cam;
		VkDescriptorSet globalDescriptorSet;
		GameObject::Map& gameObjects;
	};
}

#endif