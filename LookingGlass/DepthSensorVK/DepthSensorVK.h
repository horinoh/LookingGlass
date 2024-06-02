#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloVK.h"

#define USE_CV
#include "../CVVK.h"

#define USE_DEPTH_SENSOR
#include "../DepthSensor.h"

#ifdef USE_DEPTH_SENSOR
class DepthSensorVK : public DisplacementVK, public DepthSensorA010
{
private:
	using Super = DisplacementVK;
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

		Textures.emplace_back().Create(Device, CurrentPhysicalDeviceMemoryProperties, VK_FORMAT_R8_UNORM, VkExtent3D({ .width = Frame.Header.Cols, .height = Frame.Header.Rows, .depth = 1 }));
	}
	virtual const Texture& GetColorMap() const override { return Textures[0]; };
	virtual const Texture& GetDepthMap() const override { return Textures[0]; };
	virtual bool DrawGrayScale() const override { return true; }

	virtual void DrawFrame(const UINT i) override {
		//Update();
		//UpdateCV();
	}

	virtual void PopulateSecondaryCommandBuffer_Pass0() override {
		Mutex.lock(); {
			VK::UpdateTexture(Textures[0], Frame.Header.Cols, Frame.Header.Rows, 1, std::data(Frame.Payload), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
		} Mutex.unlock();

		Super::PopulateSecondaryCommandBuffer_Pass0();
	}
};
#endif