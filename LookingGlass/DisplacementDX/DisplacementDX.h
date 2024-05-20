#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloDX.h"

#define USE_CV
#include "../CVDX.h"

//!< カラーとデプスにテクスチャ (DDS) が分かれているケース
class DisplacementRGBD2DX : public DisplacementDX
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

	virtual const Texture& GetColorMap() const override { return XTKTextures[0]; }
	virtual const Texture& GetDepthMap() const override { return XTKTextures[1]; }
};

#ifdef USE_CV
//!< 1枚のテクスチャにカラーとデプスが左右に共存してるケース (要 OpenCV、DDS 以外も可)
class DisplacementRGBDDX : public DisplacementCVDX
{
private:
	using Super = DisplacementCVDX;
public:
	virtual void CreateTexture() override {
		Super::CreateTexture();

#if 1
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "rgbd.png").string());
#elif 1
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "begger_rgbd_s.png").string());
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

		//!< カラー (CVデータをテクスチャへ)
		Update(Create(Textures.emplace_back(), RGB, DXGI_FORMAT_R8G8B8A8_UNORM), RGB);

		//!< デプス (CVデータをテクスチャへ)
		Update(Create(Textures.emplace_back(), D, DXGI_FORMAT_R8_UNORM), D);
	}

	virtual const Texture& GetColorMap() const override { return Textures[0]; };
	virtual const Texture& GetDepthMap() const override { return Textures[1]; };
};
class DisplacementStereoDX : public DisplacementCVDX
{
private:
	using Super = DisplacementCVDX;
public:
	virtual void CreateTexture() override {
		Super::CreateTexture();

		const auto CVPath = CV::GetOpenCVPath();
		auto L = cv::imread((CVPath / "sources" / "samples" / "data" / "aloeL.jpg").string());
		auto R = cv::imread((CVPath / "sources" / "samples" / "data" / "aloeR.jpg").string());

		//!< 重いので縮小して行う
		cv::resize(L, L, cv::Size(320, 240));
		cv::resize(R, R, cv::Size(320, 240));
		//cv::imshow("L", L);
		//cv::imshow("R", R);

		//!< ステレオマッチング
		cv::Mat Disparity;
		CV::StereoMatching(L, R, Disparity);
		//cv::imshow("Disparity", Disparity);

		//!< カラー (CVデータをテクスチャへ)
		cv::cvtColor(L, L, cv::COLOR_BGR2RGBA);
		Update(Create(Textures.emplace_back(), L, DXGI_FORMAT_R8G8B8A8_UNORM), L);

		//!< デプス (CVデータをテクスチャへ)
		Update(Create(Textures.emplace_back(), Disparity, DXGI_FORMAT_R8_UNORM), Disparity);
	}

	virtual const Texture& GetColorMap() const override { return Textures[0]; };
	virtual const Texture& GetDepthMap() const override { return Textures[1]; };
};
#endif