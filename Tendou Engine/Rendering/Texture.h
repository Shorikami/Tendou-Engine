#ifndef TEXTURE_H
#define TEXTURE_H

#include "../Vulkan/TendouDevice.h"

namespace Tendou
{
	class Texture
	{
	public:
		
		// Single textures
		Texture(TendouDevice& device, std::string filePath = std::string());

		// Cubemap
		Texture(TendouDevice& device, std::vector<std::string> faces);

		~Texture();

		VkImageView TextureImageView() { return textureImageView; }
		VkSampler TextureSampler() { return textureSampler; }

		VkDescriptorImageInfo DescriptorInfo(VkImageLayout out = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	private:
		void CreateTextureImage(std::string f);
		void CreateCubemap(std::vector<std::string> faces);

		void CreateTextureImageView(uint32_t layers = 1);
		void CreateTextureSampler(VkFilter filter = VK_FILTER_LINEAR);
		TendouDevice& device_;

		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;
	};
}


#endif