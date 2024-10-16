#pragma once

#include "resource.h"

#define USE_DDS_TEXTURE
#include "../HoloDX.h"

#define USE_CV
#include "../CVDX.h"

#ifdef USE_DDS_TEXTURE
//!< カラーとデプスにテクスチャ (DDS) が分かれているケース
class DisplacementRGB_DDX : public DisplacementDX
{
private:
	using Super = DisplacementDX;
public:
	virtual void CreateTexture() override {
		Super::CreateTexture();

		const auto DCA = DirectCommandAllocators[0];
		const auto DCL = DirectCommandLists[0];

		//!< カラー、デプステクスチャ (DDS) を読み込む
#if 1
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "RGB_D" / "Rocks007_2K_Color.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "RGB_D" / "Rocks007_2K_Displacement.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#elif 1
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "RGB_D" / "14322_1.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "RGB_D" / "14322_2.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#elif 1
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "RGB_D" / "begger_rgbd_s_1.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "RGB_D" / "begger_rgbd_s_2.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#else
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "RGB_D" / "color.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "RGB_D" / "depth.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#endif
	}

	virtual const Texture& GetColorMap() const override { return XTKTextures[0]; }
	virtual const Texture& GetDepthMap() const override { return XTKTextures[1]; }
};
#endif

#ifdef USE_CV
//!< 1枚のテクスチャにカラーとデプスが左右に共存してるケース (要 OpenCV)
class DisplacementRGBDDX : public DisplacementCVDX
{
private:
	using Super = DisplacementCVDX;
public:
	virtual void CreateTexture() override {
		Super::CreateTexture();

		bool InvDepth = false;
#if 1
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "Bricks076C_1K.png").string());
#elif 1
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "kame.jpg").string());
		InvDepth = true;
#elif 1
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "begger_rgbd_s.png").string());
#elif 1
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "rgbd.png").string());
#elif 1
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "14295.png").string());
#elif 1
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "14297.png").string());
#elif 1
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "14298.png").string());
#elif 1
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "14299.png").string());
#elif 1
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "elephant1_rgbd_s.png").string());
#else
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "elephant2_rgbd_s.png").string());
#endif

		//!< カラー (右)
		const auto Cols = RGBD.cols / 2;
		auto RGB = cv::Mat(RGBD, cv::Rect(0, 0, Cols, RGBD.rows));
		cv::cvtColor(RGB, RGB, cv::COLOR_BGR2RGBA);

		//!< デプス (左)
		auto D = cv::Mat(RGBD, cv::Rect(Cols, 0, Cols, RGBD.rows));
		cv::cvtColor(D, D, cv::COLOR_BGR2GRAY);
		//!< 黒白凹凸で無い場合は反転
		if (InvDepth) {
			D = ~D; 
		}

		//!< カラー (CVデータをテクスチャへ)
		Create(Textures.emplace_back(), RGB, DXGI_FORMAT_R8G8B8A8_UNORM);

		//!< デプス (CVデータをテクスチャへ)
		Create(Textures.emplace_back(), D, DXGI_FORMAT_R8_UNORM);

		//!< テクスチャ更新
		Update2(Textures[0], RGB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			Textures[1], D, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

	virtual const Texture& GetColorMap() const override { return Textures[0]; };
	virtual const Texture& GetDepthMap() const override { return Textures[1]; };
};
#endif