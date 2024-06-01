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
	virtual void CreateTexture() override {
		Super::CreateTexture();

		Textures.emplace_back().Create(COM_PTR_GET(Device), Frame.Header.Cols, Frame.Header.Rows, 1, DXGI_FORMAT_R8_UNORM);
	}

	virtual const Texture& GetColorMap() const override { return Textures[0]; };
	virtual const Texture& GetDepthMap() const override { return Textures[0]; };

	virtual void PopulateBundleCommandList_Pass0() {
		Mutex.lock(); {
			DX::UpdateTexture(Textures[0], 1, std::data(Frame.Payload), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		} Mutex.unlock();

		Super::PopulateBundleCommandList_Pass0();
	}
};
#endif