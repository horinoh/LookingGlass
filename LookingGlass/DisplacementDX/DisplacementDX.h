#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloDX.h"

class DisplacementRGBDSepDX : public DisplacementDX 
{
private:
	using Super = DisplacementDX;
public:
	virtual void CreateTexture() override {
		Super::CreateTexture();

		//!< デプス、カラーテクスチャ
		const auto DCA = DirectCommandAllocators[0];
		const auto DCL = DirectCommandLists[0];
#if 1
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "Rocks007_2K_Color.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "Rocks007_2K_Displacement.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#elif 1
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "14322_1.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "14322_2.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#elif 1
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "begger_rgbd_s_1.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "begger_rgbd_s_2.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#else
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "color.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "depth.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#endif
	}
};