#ifndef UNIFORMBUFFER_HPP
#define UNIFORMBUFFER_HPP

#include "../Rendering/Buffer.h"

namespace Tendou
{
	template <typename T>
	class UniformBuffer : public Buffer
	{
	public:
		UniformBuffer(TendouDevice& device, VkDeviceSize instanceSize, uint32_t instanceCount,
			VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags,
			VkDeviceSize minOffsetAlignment = 1)
		: Buffer(device, instanceSize, instanceCount,
			usageFlags, memoryPropertyFlags, minOffsetAlignment)
		{
		}

		T& GetData()
		{
			return data;
		}

		static void* AlignedAlloc(size_t size, size_t alignment)
		{
			void* data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
			data = _aligned_malloc(size, alignment);
#else
			int res = posix_memalign(&data, alignment, size);
			if (res != 0)
				data = nullptr;
#endif
			return data;
		}

		static void AlignedFree(void* data)
		{
#if	defined(_MSC_VER) || defined(__MINGW32__)
			_aligned_free(data);
#else
			free(data);
#endif
		}

	private:
		T data;
	};

	class WorldUBO
	{
	public:
		glm::mat4 proj = glm::mat4(1.0f);
		glm::mat4 view = glm::mat4(1.0f);
		glm::vec2 nearFar = glm::vec2(0.0f);
	};

	class RenderUBO
	{
	public:
		glm::mat4* view = nullptr;
	};

	class LightsUBO
	{
	public:
		glm::vec4 lightPos[16] = {};
		glm::vec4 lightColor[16] = {};
		glm::vec4 lightDir[16] = {};

		glm::vec4 eyePos = {};
		glm::vec4 emissive = {};
		glm::vec4 globalAmbient = {};
		glm::vec4 coefficients = {}; // x = kA, y = kD, z = kS, w = ns

		glm::vec4 fogColor = glm::vec4(1.0f);

		glm::vec4 specular[16] = {};
		glm::vec4 ambient[16] = {};
		glm::vec4 diffuse[16] = {};

		glm::vec4 lightInfo[16] = {}; // x = inner, y = outer, z = falloff, w = type

		glm::vec4 modes = {}; // x = use gpu, y = use normals, z = UV calculation type

		glm::vec3 attenuation = glm::vec3(0.5f, 0.37f, 0.2f); // x = c1, y = c2, z = c3
		int numLights;
		//float _pad; // std140 requires padding - vec4 = 16 bytes, vec3 + float == 12 + 4 = 16 bytes
	};
}

#endif