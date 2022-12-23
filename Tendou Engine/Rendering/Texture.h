#ifndef TEXTURE_H
#define TEXTURE_H

#include "../Vulkan/TendouDevice.h"

namespace Tendou
{
	class Texture
	{
	public:
		
		// empty texture; specify if cubemap or not
		Texture(TendouDevice& device, int w, int h, bool cubemap = false);

		// Single textures
		Texture(TendouDevice& device, std::string filePath = std::string());

		// Cubemaps
		// --------
		Texture(TendouDevice& device, std::vector<std::string> faces); // strings to file paths
		

		~Texture();

		VkImage TextureImage() { return textureImage; }
		VkImageView TextureImageView() { return textureImageView; }
		VkSampler TextureSampler() { return textureSampler; }

		VkDescriptorImageInfo DescriptorInfo(VkImageLayout out = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	private:
		void CreateEmptyTexture(int width, int height, bool cubemap);

		void CreateTextureImage(std::string f);
		void CreateCubemap(std::vector<std::string> faces);

		void CreateTextureImageView(uint32_t layers = 1, VkImageViewType t = VK_IMAGE_VIEW_TYPE_2D);
		void CreateTextureSampler(VkFilter filter = VK_FILTER_LINEAR);
		TendouDevice& device_;

		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;
	};
}


#endif