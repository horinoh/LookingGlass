#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloVK.h"

#define USE_CV
#include "../CVVK.h"

//#define USE_DEPTH_SENSOR
#include "../DepthSensor.h"

#ifdef USE_DEPTH_SENSOR
class DisplacementDepthSensorA010VK : public DisplacementVK, public DepthSensorA010
{
private:
	using Super = DisplacementVK;
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		Super::OnCreate(hWnd, hInstance, Title);

		//!< デプスセンサーオープン
		Open(COM::COM3);
#ifdef USE_CV
		if (SerialPort.is_open()) {
			cv::imshow("Preview", Preview);
			cv::createTrackbar("Unit", "Preview", &Unit, static_cast<int>(UNIT::Max), [](int Value, void* Userdata) {
				//static_cast<DepthSensorA010*>(Userdata)->SetCmdUNIT(Value);
				}, this);
			cv::createTrackbar("FPS", "Preview", &FPS, static_cast<int>(FPS::Max), [](int Value, void* Userdata) {
				//static_cast<DepthSensorA010*>(Userdata)->SetCmdFPS(Value);
				}, this);
		}
#endif
	}
	virtual void CreateTexture() override {
		Super::CreateTexture();

		Textures.emplace_back().Create(Device, CurrentPhysicalDeviceMemoryProperties, VK_FORMAT_R8_UNORM, VkExtent3D({ .width = 100, .height = 100, .depth = 1 }));	
	}
	virtual void DrawFrame(const UINT i) override {
		Super::DrawFrame(i);

		//!< デプスセンサーの更新
		Update();
	}
	virtual const Texture& GetColorMap() const override { return Textures[0]; };
	virtual const Texture& GetDepthMap() const override { return Textures[0]; };

	virtual void Update() override {
		DepthSensorA010::Update();

		if (SerialPort.is_open()) {
#ifdef USE_CV
			auto Depth = cv::Mat(cv::Size(Frame.Header.Cols, Frame.Header.Rows), CV_8UC1, std::data(Frame.Payload), cv::Mat::AUTO_STEP);
			//!< 黒白が凹凸となるように反転
			Depth = ~Depth;

			//!< #TODO
			//VK::UpdateTexture(Textures[0], Frame.Header.Cols, Frame.Header.Rows, 1, std::data(Frame.Payload), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);

			//!< プレビューする為の処理
			cv::resize(Depth, Depth, cv::Size(420, 560));
			Depth.copyTo(Preview);
			cv::imshow("Preview", Preview);
#endif
		}
	}

#ifdef USE_CV
	cv::Mat Preview = cv::Mat(cv::Size(420, 560), CV_8UC1);
	int Unit;
	int FPS;
#endif
};
#endif

//!< カラーとデプスにテクスチャ (DDS) が分かれているケース
class DisplacementRGB_DVK : public DisplacementVK
{
private:
	using Super = DisplacementVK;
public:
	virtual void CreateTexture() override {
		Super::CreateTexture();

		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;
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

#ifdef USE_CV
//!< 1枚のテクスチャにカラーとデプスが左右に共存してるケース (要 OpenCV)
class DisplacementRGBDVK : public DisplacementCVVK
{
private:
	using Super = DisplacementCVVK;
public:
	virtual void CreateTexture() override {
		Super::CreateTexture();

#if 1
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "Bricks076C_1K.png").string());
#elif 1
		const auto RGBD = cv::imread((std::filesystem::path("..") / "Asset" / "RGBD" / "kame.jpg").string());
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
		Create(Textures.emplace_back(), L, VK_FORMAT_R8G8B8A8_UNORM);
	
		//!< デプス (CVデータをテクスチャへ)
		Create(Textures.emplace_back(), Disparity, VK_FORMAT_R8_UNORM);

		//!< テクスチャ更新
		Update2(Textures[0], L, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			Textures[1], Disparity, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
	}
	virtual const Texture& GetColorMap() const override { return Textures[0]; };
	virtual const Texture& GetDepthMap() const override { return Textures[1]; };
};
#endif