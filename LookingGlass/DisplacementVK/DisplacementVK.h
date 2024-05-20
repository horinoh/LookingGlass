#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloVK.h"

#define USE_CV
#include "../CVVK.h"

//!< �J���[�ƃf�v�X�Ƀe�N�X�`�� (DDS) ��������Ă���P�[�X
class DisplacementRGBD2VK : public DisplacementVK 
{
private:
	using Super = DisplacementVK;
public:
	virtual void CreateTexture() override {
		Super::CreateTexture();

		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;
		const auto CB = CommandBuffers[0];

		//!< �J���[�A�f�v�X�e�N�X�`�� (DDS) ��ǂݍ���
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

	virtual const Texture& GetColorMap() const override { return GLITextures[0]; };
	virtual const Texture& GetDepthMap() const override { return GLITextures[1]; };
};

#ifdef USE_CV
//!< 1���̃e�N�X�`���ɃJ���[�ƃf�v�X�����E�ɋ������Ă�P�[�X (�v OpenCV�ADDS �ȊO����)
class DisplacementRGBDVK : public DisplacementCVVK
{
private:
	using Super = DisplacementCVVK;
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

		//!< �J���[ (�E)
		const auto Cols = RGBD.cols / 2;
		auto RGB = cv::Mat(RGBD, cv::Rect(0, 0, Cols, RGBD.rows));
		cv::cvtColor(RGB, RGB, cv::COLOR_BGR2RGBA);

		//!< �f�v�X (��)
		auto D = cv::Mat(RGBD, cv::Rect(Cols, 0, Cols, RGBD.rows));
		cv::cvtColor(D, D, cv::COLOR_BGR2GRAY);

		//!< �J���[ (CV�f�[�^���e�N�X�`����)
		Update(Create(Textures.emplace_back(), RGB, VK_FORMAT_R8G8B8A8_UNORM), RGB, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		//!< �f�v�X (CV�f�[�^���e�N�X�`����)
		Update(Create(Textures.emplace_back(), D, VK_FORMAT_R8_UNORM), D, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
	}

	virtual const Texture& GetColorMap() const override { return Textures[0]; };
	virtual const Texture& GetDepthMap() const override { return Textures[1]; };
};
class DisplacementStereoVK : public DisplacementCVVK
{
private:
	using Super = DisplacementCVVK;
public:
	virtual void CreateTexture() override {
		Super::CreateTexture();

		const auto CVPath = CV::GetOpenCVPath();
		auto L = cv::imread((CVPath / "sources" / "samples" / "data" / "aloeL.jpg").string());
		auto R = cv::imread((CVPath / "sources" / "samples" / "data" / "aloeR.jpg").string());

		//!< �d���̂ŏk�����čs��
		cv::resize(L, L, cv::Size(320, 240));
		cv::resize(R, R, cv::Size(320, 240));
		//cv::imshow("L", L);
		//cv::imshow("R", R);

		//!< �X�e���I�}�b�`���O
		cv::Mat Disparity;
		CV::StereoMatching(L, R, Disparity);
		//cv::imshow("Disparity", Disparity);

		//!< �J���[ (CV�f�[�^���e�N�X�`����)
		cv::cvtColor(L, L, cv::COLOR_BGR2RGBA);
		Update(Create(Textures.emplace_back(), L, VK_FORMAT_R8G8B8A8_UNORM), L, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		//!< �f�v�X (CV�f�[�^���e�N�X�`����)
		Update(Create(Textures.emplace_back(), Disparity, VK_FORMAT_R8_UNORM), Disparity, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
	}
	virtual const Texture& GetColorMap() const override { return Textures[0]; };
	virtual const Texture& GetDepthMap() const override { return Textures[1]; };
};
#endif