#ifndef TEXTURE_H
#define TEXTURE_H

#include "../Vulkan/DrevisDevice.h"

namespace Drevis
{
	class Texture
	{
	public:
		Texture(DrevisDevice& device, std::string filePath = std::string());
		~Texture();

		VkImageView TextureImageView() { return textureImageView; }
		VkSampler TextureSampler() { return textureSampler; }

		VkDescriptorImageInfo DescriptorInfo(VkImageLayout out = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	private:
		void CreateTextureImage(std::string f);
		void CreateTextureImageView();
		void CreateTextureSampler();
		DrevisDevice& device_;

		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;
	};
}


#endif