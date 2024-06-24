#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloVK.h"

#define USE_CV
//#define USE_CUDA
#include "../CVVK.h"

#define USE_LEAP
#include "../Leap.h"

class DisplacementLeapVK : public DisplacementStereoCVVK, public Leap
{
private:
	using Super = DisplacementStereoCVVK;
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
		StereoCV::StereoMatching(L, R, Disparity);
		cv::imshow("Disparity", Disparity);

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