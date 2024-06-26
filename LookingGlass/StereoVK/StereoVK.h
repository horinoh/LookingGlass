#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloVK.h"

#define USE_CV
//#define USE_CUDA
#include "../CVVK.h"

#define USE_LEAP
#include "../Leap.h"

class DisplacementLeapVK : public DisplacementWldVK, public LeapCV
{
private:
	using Super = DisplacementWldVK;
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		UpdateAsyncStart();
		Super::OnCreate(hWnd, hInstance, Title);
	}
	virtual void OnDestroy(HWND hWnd, HINSTANCE hInstance) override {
		Super::OnDestroy(hWnd, hInstance);
		ExitThread();
	}
	virtual void CreateTexture() override {
		Super::CreateTexture();
		
		AnimatedTextures.emplace_back().Create(Device, CurrentPhysicalDeviceMemoryProperties, VK_FORMAT_R8_UNORM, static_cast<uint32_t>(DisparityImage.elemSize()), VkExtent3D({.width = static_cast<uint32_t>(DisparityImage.cols), .height = static_cast<uint32_t>(DisparityImage.rows), .depth = 1}));
	}
	virtual const Texture& GetColorMap() const override { return AnimatedTextures[0]; };
	virtual const Texture& GetDepthMap() const override { return AnimatedTextures[0]; };
	virtual bool DrawGrayScale() const override { return true; }

	virtual void DrawFrame(const UINT i) override {
		if (0 == i) {
			AnimatedTextures[0].UpdateStagingBuffer(Device, DisparityImage.total() * DisparityImage.elemSize(), DisparityImage.ptr());
		}
	}
	virtual void PopulateAnimatedTextureCommand(const size_t i) override {
		const auto CB = CommandBuffers[i];
		AnimatedTextures[0].PopulateStagingToImageCommand(CB, DisparityImage.cols, DisparityImage.rows, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
	}
	virtual void UpdateWorldBuffer() override {
		float X, Y;
		GetXYScaleForDevice(X, Y);
		WorldBuffer.World[0] = glm::scale(glm::mat4(1.0f), glm::vec3(X, Y, 5.0f));
	}
};