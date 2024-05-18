#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloVK.h"

class DisplacementRGBDSepVK : public DisplacementVK 
{
private:
	using Super = DisplacementVK;
public:
	virtual void CreateTexture() override {
		Super::CreateTexture();
	
		//!< デプス、カラーテクスチャ
		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;
		const auto CB = CommandBuffers[0];
#if 1
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "Rocks007_2K_Color.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "Rocks007_2K_Displacement.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
#elif 1
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "14322_1.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "14322_2.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
#elif 1
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "begger_rgbd_s_1.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "begger_rgbd_s_2.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
#else
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "color.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "depth.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
#endif
	}
};