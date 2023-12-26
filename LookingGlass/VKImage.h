#pragma once

#include <fstream>
#include <filesystem>

#pragma warning(push)
#pragma warning(disable : 4100)
//#pragma warning(disable : 4201)
#pragma warning(disable : 4244)
#pragma warning(disable : 4458)
#pragma warning(disable : 5054)
#pragma warning(disable : 4189)
#include <gli/gli.hpp>
#pragma warning(pop)

#include "VK.h"

class VKImage : public VK
{
private:
	using Super = VK;
public:
	[[nodiscard]] static VkFormat ToVkFormat(const gli::format GLIFormat) {
#define GLI_FORMAT_TO_VK_FORMAT_ENTRY(glientry, vkentry) case FORMAT_ ## glientry: return VK_FORMAT_ ## vkentry;
		switch (GLIFormat)
		{
			using enum gli::format;
#include "VKGLIFormat.h"
		}
#undef GLI_FORMAT_TO_VK_FORMAT_ENTRY
		assert(false && "Not supported");
		return VK_FORMAT_UNDEFINED;
	}
	[[nodiscard]] static VkImageViewType ToVkImageViewType(const gli::target GLITarget) {
#define GLI_TARGET_TO_VK_IMAGE_VIEW_TYPE_ENTRY(entry) case TARGET_ ## entry: return VK_IMAGE_VIEW_TYPE_ ## entry
		switch (GLITarget)
		{
			using enum gli::target;
			GLI_TARGET_TO_VK_IMAGE_VIEW_TYPE_ENTRY(1D);
			GLI_TARGET_TO_VK_IMAGE_VIEW_TYPE_ENTRY(1D_ARRAY);
			GLI_TARGET_TO_VK_IMAGE_VIEW_TYPE_ENTRY(2D);
			GLI_TARGET_TO_VK_IMAGE_VIEW_TYPE_ENTRY(2D_ARRAY);
			GLI_TARGET_TO_VK_IMAGE_VIEW_TYPE_ENTRY(3D);
			GLI_TARGET_TO_VK_IMAGE_VIEW_TYPE_ENTRY(CUBE);
			GLI_TARGET_TO_VK_IMAGE_VIEW_TYPE_ENTRY(CUBE_ARRAY);
		}
#undef GLI_TARGET_TO_VK_IMAGE_VIEW_TYPE_ENTRY
		assert(false && "Not supported");
		return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	}
	[[nodiscard]] static VkImageType ToVkImageType(const gli::target GLITarget) {
		assert(gli::TARGET_INVALID != static_cast<int>(GLITarget) && "TARGET_INVALID");
		switch (GLITarget)
		{
			using enum gli::target;
		case TARGET_1D:
		case TARGET_1D_ARRAY:
			return VK_IMAGE_TYPE_1D;
		case TARGET_2D:
		case TARGET_2D_ARRAY:
		case TARGET_CUBE:
		case TARGET_CUBE_ARRAY:
			return VK_IMAGE_TYPE_2D;
		case TARGET_3D:
			return VK_IMAGE_TYPE_3D;
		}
		assert(false && "Not supported");
		return VK_IMAGE_TYPE_MAX_ENUM;
	}
	[[nodiscard]] static VkComponentSwizzle ToVkComponentSwizzle(const gli::swizzle GLISwizzle) {
		switch (GLISwizzle)
		{
		case gli::SWIZZLE_ZERO: return VK_COMPONENT_SWIZZLE_ZERO;
		case gli::SWIZZLE_ONE: return VK_COMPONENT_SWIZZLE_ONE;
		case gli::SWIZZLE_RED: return VK_COMPONENT_SWIZZLE_R;
		case gli::SWIZZLE_GREEN: return VK_COMPONENT_SWIZZLE_G;
		case gli::SWIZZLE_BLUE: return VK_COMPONENT_SWIZZLE_B;
		case gli::SWIZZLE_ALPHA: return VK_COMPONENT_SWIZZLE_A;
		}
		return VK_COMPONENT_SWIZZLE_IDENTITY;
	}
	[[nodiscard]] static VkComponentMapping ToVkComponentMapping(const gli::texture::swizzles_type GLISwizzleType) {
		return VkComponentMapping({ .r = ToVkComponentSwizzle(GLISwizzleType.r), .g = ToVkComponentSwizzle(GLISwizzleType.g), .b = ToVkComponentSwizzle(GLISwizzleType.b), .a = ToVkComponentSwizzle(GLISwizzleType.a) });
	}

public:
	virtual void OnDestroy(HWND hWnd, HINSTANCE hInstance) override {
		for (auto& i : GLITextures) { i.Destroy(Device); } GLITextures.clear();
		Super::OnDestroy(hWnd, hInstance);
	}

	class GLITexture : public Texture
	{
	private:
		using Super = Texture;
		gli::texture GliTexture;

	public:
		GLITexture& Create(const VkDevice Dev, const VkPhysicalDeviceMemoryProperties PDMP, const std::filesystem::path& Path) {
			assert(std::filesystem::exists(Path) && "");
			if (IsDDS(Path) || IsKTX(Path)) {
				GliTexture = gli::load(data(Path.string()));
			}
			assert(!GliTexture.empty() && "Load image failed");

			const auto Format = ToVkFormat(GliTexture.format());
			VK::CreateImageMemory(&Image, &DeviceMemory, Dev, PDMP,
				gli::is_target_cube(GliTexture.target()) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
				ToVkImageType(GliTexture.target()),
				Format,
				VkExtent3D({ .width = static_cast<const uint32_t>(GliTexture.extent(0).x), .height = static_cast<const uint32_t>(GliTexture.extent(0).y), .depth = static_cast<const uint32_t>(GliTexture.extent(0).z) }),
				static_cast<const uint32_t>(GliTexture.levels()),
				static_cast<const uint32_t>(GliTexture.layers()) * static_cast<const uint32_t>(GliTexture.faces()),
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

			const VkImageViewCreateInfo IVCI = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = Image,
				.viewType = ToVkImageViewType(GliTexture.target()),
				.format = Format,
				.components = ToVkComponentMapping(GliTexture.swizzles()),
				.subresourceRange = VkImageSubresourceRange({.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = VK_REMAINING_MIP_LEVELS, .baseArrayLayer = 0, .layerCount = VK_REMAINING_ARRAY_LAYERS })
			};
			VERIFY_SUCCEEDED(vkCreateImageView(Dev, &IVCI, GetAllocationCallbacks(), &View));
			return *this;
		}
		void PopulateCopyCommand(const VkCommandBuffer CB, const VkPipelineStageFlags PSF, const VkBuffer Staging) {
			//!< キューブマップの場合は、複数レイヤのイメージとして作成する。(When cubemap, create as layered image)
			//!< イメージビューを介して、レイヤをフェイスとして扱うようハードウエアへ伝える (Tell the hardware that it should interpret its layers as faces)
			//!< キューブマップの場合フェイスの順序は +X-X+Y-Y+Z-Z (When cubemap, faces order is +X-X+Y-Y+Z-Z)
			const auto Layers = static_cast<const uint32_t>(GliTexture.layers()) * static_cast<const uint32_t>(GliTexture.faces());
			const auto Levels = static_cast<const uint32_t>(GliTexture.levels());
			std::vector<VkBufferImageCopy> BICs; BICs.reserve(Layers * Levels);
			VkDeviceSize Offset = 0;
			for (uint32_t i = 0; i < Layers; ++i) {
				for (uint32_t j = 0; j < Levels; ++j) {
					BICs.emplace_back(VkBufferImageCopy({
						.bufferOffset = Offset, .bufferRowLength = 0, .bufferImageHeight = 0,
						.imageSubresource = VkImageSubresourceLayers({.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = j, .baseArrayLayer = i, .layerCount = 1 }),
						.imageOffset = VkOffset3D({.x = 0, .y = 0, .z = 0 }),
						.imageExtent = VkExtent3D({.width = static_cast<const uint32_t>(GliTexture.extent(j).x), .height = static_cast<const uint32_t>(GliTexture.extent(j).y), .depth = static_cast<const uint32_t>(GliTexture.extent(j).z) }) }));
					Offset += static_cast<const VkDeviceSize>(GliTexture.size(j));
				}
			}
			VK::PopulateCopyBufferToImageCommand(CB, Staging, Image, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, PSF, BICs, Levels, Layers);
		}
		void SubmitCopyCommand(const VkDevice Dev, const VkPhysicalDeviceMemoryProperties PDMP, const VkCommandBuffer CB, const VkQueue Queue, const VkPipelineStageFlags PSF) {
			VK::Scoped<StagingBuffer> Staging(Dev);
			Staging.Create(Dev, PDMP, static_cast<VkDeviceSize>(GliTexture.size()), GliTexture.data());
			constexpr VkCommandBufferBeginInfo CBBI = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .pNext = nullptr, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, .pInheritanceInfo = nullptr };
			VERIFY_SUCCEEDED(vkBeginCommandBuffer(CB, &CBBI)); {
				PopulateCopyCommand(CB, PSF, Staging.Buffer);
			} VERIFY_SUCCEEDED(vkEndCommandBuffer(CB));
			VK::SubmitAndWait(Queue, CB);
		}
	};

	std::vector<GLITexture> GLITextures;
};
