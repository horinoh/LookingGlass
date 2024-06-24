#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloDX.h"

#define USE_CV
//#define USE_CUDA
#include "../CVDX.h"

#define USE_LEAP
#include "../Leap.h"

class DisplacementLeapDX : public DisplacementStereoCVDX, public Leap
{
private:
	using Super = DisplacementStereoCVDX;
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
		//cv::imshow("Disparity", Disparity);

		cv::cvtColor(L, L, cv::COLOR_BGR2RGBA);

		//!< カラー (CVデータをテクスチャへ)
		Create(Textures.emplace_back(), L, DXGI_FORMAT_R8G8B8A8_UNORM);

		//!< デプス (CVデータをテクスチャへ)
		Create(Textures.emplace_back(), Disparity, DXGI_FORMAT_R8_UNORM);

		//!< テクスチャ更新
		Update2(Textures[0], L, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			Textures[1], Disparity, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

	virtual const Texture& GetColorMap() const override { return Textures[0]; };
	virtual const Texture& GetDepthMap() const override { return Textures[1]; };
};