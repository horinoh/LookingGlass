#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloDX.h"

#define USE_CV
#include "../CVDX.h"

#define USE_DEPTH_SENSOR
#include "../DepthSensor.h"

#ifdef USE_DEPTH_SENSOR
class DepthSensorDX : public DisplacementDX, public DepthSensorA010
{
private:
	using Super = DisplacementDX;
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		//!< デプスセンサーオープン
		Open(COM::COM3);

		//!< 非同期更新開始
		UpdateAsyncStart();

		Super::OnCreate(hWnd, hInstance, Title);
	}
	virtual void OnDestroy(HWND hWnd, HINSTANCE hInstance) override {
		Super::OnDestroy(hWnd, hInstance);

		Exit();
	}
	virtual void CreateTexture() override {
		Super::CreateTexture();
		AnimatedTextures.emplace_back().Create(COM_PTR_GET(Device), Frame.Header.Cols, Frame.Header.Rows, 1, DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}

	virtual const Texture& GetColorMap() const override { return AnimatedTextures[0]; };
	virtual const Texture& GetDepthMap() const override { return AnimatedTextures[0]; };
	virtual bool DrawGrayScale() const override { return true; }

	virtual void DrawFrame(const UINT i) override {
		if (0 == i) {
			constexpr auto Bpp = 1;
			constexpr auto Layers = 1;
			AnimatedTextures[0].UpdateUploadBuffer(Frame.Header.Cols, Frame.Header.Rows, Bpp, std::data(Frame.Payload), Layers);
		}
	}
	virtual void PopulateAnimatedTextureCommand(const size_t i) override {
		const auto DCL = COM_PTR_GET(DirectCommandLists[i]);
		constexpr auto Bpp = 1;
		AnimatedTextures[0].PopulateUploadToTextureCommand(DCL, Bpp);
	}
};
#endif