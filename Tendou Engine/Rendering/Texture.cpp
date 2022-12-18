#include "Texture.h"
#include "Buffer.h"

#include <cassert>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


namespace Tendou
{
	Texture::Texture(TendouDevice& d, std::string filePath)
		: device_(d)
	{
		assert(!filePath.empty() && "Texture error: File path is empty!");
		CreateTextureImage(filePath);
		CreateTextureImageView();
		CreateTextureSampler();
	}

	Texture::Texture(TendouDevice& d, std::vector<std::string> faces)
		: device_(d)
	{
		assert(faces.size() == 6 && "Cubemap error: Container must have exactly 6 faces!");
		CreateCubemap(faces);
		CreateTextureImageView(6);
		CreateTextureSampler(VK_FILTER_NEAREST);
	}

	void Texture::CreateCubemap(std::vector<std::string> faces)
	{
		int width, height, channels;
		stbi_uc* imageBuffers[6];

		for (unsigned i = 0; i < 6; ++i)
		{
			imageBuffers[i] = stbi_load(faces[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
			if (!imageBuffers[i])
			{
				throw std::runtime_error("Failed to load cubemap face!");
			}
		}

		VkDeviceSize layerSize = width * height * 4;
		VkDeviceSize imageSize = layerSize * 6;

		Buffer stagingBuf
		{
			device_,
			imageSize,
			1,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuf.Map();

		for (unsigned i = 0; i < 6; ++i)
		{
			stagingBuf.WriteToBuffer(imageBuffers[i], static_cast<size_t>(layerSize), layerSize * i);
		}

		stagingBuf.Unmap();

		for (unsigned i = 0; i < 6; ++i)
		{
			stbi_image_free(imageBuffers[i]);
		}
		
		// TODO: Fix pls (4/3 channels - RGBA/RGB)
		VkFormat bitFormat = channels == 4 ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_SRGB;

		device_.CreateImage(width, height, bitFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, 6);

		device_.TransitionImageLayout(textureImage, bitFormat,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);

		device_.CopyBufferToImage(stagingBuf.GetBuffer(), textureImage, 
			static_cast<uint32_t>(width), static_cast<uint32_t>(height), 6);

		device_.TransitionImageLayout(textureImage, bitFormat,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);
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

	void Texture::CreateTextureImageView(uint32_t layers)
	{
		textureImageView = device_.CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, layers);
	}

	void Texture::CreateTextureSampler(VkFilter filter)
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(device_.PhysicalDevice(), &properties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = filter;
		samplerInfo.minFilter = filter;
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
