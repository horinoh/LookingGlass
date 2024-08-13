#pragma once

#include "resource.h"

#define USE_DDS_TEXTURE
#include "../HoloVK.h"

#define USE_CV
#include "../CVVK.h"

#ifdef USE_DDS_TEXTURE
//!< カラーとデプスにテクスチャ (DDS) が分かれているケース
class DisplacementRGB_DVK : public DisplacementVK
{
private:
	using Super = DisplacementVK;
public:
	virtual void CreateTexture() override {
		Super::CreateTexture();

		const auto& PDMP = SelectedPhysDevice.second.PDMP;
		const auto CB = CommandBuffers[0];

		//!< カラー、デプステクスチャ (DDS) を読み込む
#if 1
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "RGB_D" / "Rocks007_2K_Color.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "RGB_D" / "Rocks007_2K_Displacement.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
#elif 1
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "RGB_D" / "14322_1.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "RGB_D" / "14322_2.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
#elif 1
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "RGB_D" / "begger_rgbd_s_1.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "RGB_D" / "begger_rgbd_s_2.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
#else
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "RGB_D" / "color.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "RGB_D" / "depth.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
#endif
	}

	virtual const Texture& GetColorMap() const override { return GLITextures[0]; };
	virtual const Texture& GetDepthMap() const override { return GLITextures[1]; };
};
#endif

#ifdef USE_CV
//!< 1枚のテクスチャにカラーとデプスが左右に共存してるケース (要 OpenCV)
class DisplacementRGBDVK : public DisplacementCVVK
{
private:
	using Super = DisplacementCVVK;
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
		Create(Textures.emplace_back(), RGB, VK_FORMAT_R8G8B8A8_UNORM);

		//!< デプス (CVデータをテクスチャへ)
		Create(Textures.emplace_back(), D, VK_FORMAT_R8_UNORM);

		//!< テクスチャ更新
		Update2(Textures[0], RGB, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			Textures[1], D, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
	}

	virtual const Texture& GetColorMap() const override { return Textures[0]; };
	virtual const Texture& GetDepthMap() const override { return Textures[1]; };
};
#endif