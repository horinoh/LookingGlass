#pragma once

#ifdef USE_CV
#include "CV.h"

class DisplacementCVVK : public DisplacementVK, public CV
{
public:
	virtual Texture& Create(Texture& Tex, const cv::Mat CVImage, const VkFormat Format) {
		return Tex.Create(Device, CurrentPhysicalDeviceMemoryProperties, Format, VkExtent3D({ .width = static_cast<uint32_t>(CVImage.cols), .height = static_cast<uint32_t>(CVImage.rows), .depth = 1 }));
	}
	virtual Texture& Update(Texture& Tex, const cv::Mat CVImage, const VkPipelineStageFlagBits Stage) {
		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;
		const auto CB = CommandBuffers[0];

		VK::Scoped<BufferMemory> StagingBuffer(Device);
		StagingBuffer.Create(Device, PDMP, CVImage.cols * CVImage.rows * CVImage.channels(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, CVImage.ptr());
		const std::vector BICs = {
			VkBufferImageCopy({
				.bufferOffset = 0, .bufferRowLength = 0, .bufferImageHeight = 0,
				.imageSubresource = VkImageSubresourceLayers({.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 }),
				.imageOffset = VkOffset3D({.x = 0, .y = 0, .z = 0 }),
				.imageExtent = VkExtent3D({.width = static_cast<uint32_t>(CVImage.cols), .height = static_cast<uint32_t>(CVImage.rows), .depth = 1 })
			}),
		};
		constexpr VkCommandBufferBeginInfo CBBI = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr
		};
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CB, &CBBI)); {
			PopulateCopyBufferToImageCommand(CB, StagingBuffer.Buffer, Tex.Image, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, BICs, 1, 1);
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CB));
		VK::SubmitAndWait(GraphicsQueue, CB);

		return Tex;
	}
};
#endif