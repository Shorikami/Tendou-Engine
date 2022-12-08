#include "Texture.h"
#include "Buffer.h"

#include <cassert>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


namespace Drevis
{
	Texture::Texture(DrevisDevice& d, std::string filePath)
		: device_(d)
	{
		assert(!filePath.empty() && "Texture error: File path is empty!");
		CreateTextureImage(filePath);
		CreateTextureImageView();
		CreateTextureSampler();
	}

	Texture::~Texture()
	{
		vkDestroySampler(device_.Device(), textureSampler, nullptr);
		vkDestroyImageView(device_.Device(), textureImageView, nullptr);
		vkDestroyImage(device_.Device(), textureImage, nullptr);
		vkFreeMemory(device_.Device(), textureImageMemory, nullptr);
	}

	void Texture::CreateTextureImage(std::string f)
	{
		int width, height, channels;
		stbi_uc* res = stbi_load(f.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		VkDeviceSize imageSize = width * height * 4;

		if (!res)
		{
			throw std::runtime_error("Failed to load texture image!");
		}

		Buffer stagingBuf
		{
			device_,
			imageSize,
			1,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuf.Map();
		stagingBuf.WriteToBuffer(res, static_cast<size_t>(imageSize));
		stagingBuf.Unmap();
		//VkBuffer stagingBuf;
		//VkDeviceMemory stagingBufMemory;
		//device_.CreateBuffer(imageSize,
		//	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		//	stagingBuf, stagingBufMemory);
		//
		//void* data;
		//vkMapMemory(device_.Device(), stagingBufMemory, 0, imageSize, 0, &data);
		//memcpy(data, res, static_cast<size_t>(imageSize));
		//vkUnmapMemory(device_.Device(), stagingBufMemory);

		stbi_image_free(res);

		device_.CreateImage(width, height, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
		
		device_.TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		device_.CopyBufferToImage(stagingBuf.GetBuffer(), textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

		device_.TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		//vkDestroyBuffer(device_.Device(), stagingBuf, nullptr);
		//vkFreeMemory(device_.Device(), stagingBufMemory, nullptr);
	}

	void Texture::CreateTextureImageView()
	{
		textureImageView = device_.CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
	}

	void Texture::CreateTextureSampler()
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(device_.PhysicalDevice(), &properties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		if (vkCreateSampler(device_.Device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	VkDescriptorImageInfo Texture::DescriptorInfo(VkImageLayout layout)
	{
		return VkDescriptorImageInfo
		{
			textureSampler,
			textureImageView,
			layout,
		};
	}
}
