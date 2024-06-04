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
		//!< �[�x�Z���T�I�[�v�� (COM�ԍ��͒��ׂāA�K�؂Ɏw�肷��K�v������)
		Open(COM::COM3);

		//!< �񓯊��X�V�J�n
		UpdateAsyncStart();

		Super::OnCreate(hWnd, hInstance, Title);
	}
	virtual void OnDestroy(HWND hWnd, HINSTANCE hInstance) override {
		Super::OnDestroy(hWnd, hInstance);
		
		Exit();
	}
	virtual void CreateTexture() override {
		Super::CreateTexture();

		constexpr auto Bpp = 1;
		AnimatedTextures.emplace_back().Create(Device, CurrentPhysicalDeviceMemoryProperties, VK_FORMAT_R8_UNORM, Bpp, VkExtent3D({ .width = Frame.Header.Cols, .height = Frame.Header.Rows, .depth = 1 }));
	}
	virtual const Texture& GetColorMap() const override { return AnimatedTextures[0]; };
	virtual const Texture& GetDepthMap() const override { return AnimatedTextures[0]; };
	virtual bool DrawGrayScale() const override { return true; }

	virtual void DrawFrame(const UINT i) override {
		if (0 == i) {
			constexpr auto Bpp = 1;
			AnimatedTextures[0].UpdateStagingBuffer(Device, Frame.Header.Cols * Frame.Header.Rows * Bpp, std::data(Frame.Payload));
		}
	}
	virtual void PopulateAnimatedTextureCommand(const size_t i) override {
		const auto CB = CommandBuffers[i];
		AnimatedTextures[0].PopulateStagingToImageCommand(CB, Frame.Header.Cols, Frame.Header.Rows, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
	}
};
#endif