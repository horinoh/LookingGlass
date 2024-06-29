#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloDX.h"

#define USE_CV
#define USE_CUDA
#include "../CVDX.h"

#define USE_LEAP
#include "../Leap.h"

class DisplacementLeapDX : public DisplacementWldDX, public LeapCV
{
private:
	using Super = DisplacementWldDX;
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		CreateUI();
		UpdateAsyncStart();
		Super::OnCreate(hWnd, hInstance, Title);
	}
	virtual void OnDestroy(HWND hWnd, HINSTANCE hInstance) override {
		Super::OnDestroy(hWnd, hInstance);
		ExitThread();
	}

	virtual void CreateTexture() override {
		Super::CreateTexture();

		constexpr auto Layers = 1;
		AnimatedTextures.emplace_back().Create(COM_PTR_GET(Device), DisparityImage.cols, DisparityImage.rows, static_cast<UINT>(DisparityImage.elemSize()), Layers, DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		
		//const auto& ColorMap = StereoImages[0];
		//AnimatedTextures.emplace_back().Create(COM_PTR_GET(Device), ColorMap.cols, ColorMap.rows, static_cast<UINT>(ColorMap.elemSize()), Layers, DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}

	virtual const Texture& GetColorMap() const override { return AnimatedTextures[0]; };
	virtual const Texture& GetDepthMap() const override { return AnimatedTextures[0]; };
	virtual bool DrawGrayScale() const override { return true; }

	virtual void DrawFrame(const UINT i) override {
		if (0 == i) {
			Mutex.lock();
			{
				constexpr auto Layers = 1;
				AnimatedTextures[0].UpdateUploadBuffer(DisparityImage.cols, DisparityImage.rows, static_cast<UINT>(DisparityImage.elemSize()), Layers, DisparityImage.ptr());

				//const auto& ColorMap = StereoImages[0];
				//AnimatedTextures[1].UpdateUploadBuffer(ColorMap.cols, ColorMap.rows, static_cast<UINT>(ColorMap.elemSize()), Layers, ColorMap.ptr());
			}
			Mutex.unlock();
		}
	}
	virtual void PopulateAnimatedTextureCommand(const size_t i) override {
		const auto DCL = COM_PTR_GET(DirectCommandLists[i]);

		constexpr auto Bpp = 1;
		AnimatedTextures[0].PopulateUploadToTextureCommand(DCL, Bpp);
		//AnimatedTextures[1].PopulateUploadToTextureCommand(DCL, Bpp);
	}
	virtual void UpdateWorldBuffer() override {
		float X, Y;
		GetXYScaleForDevice(X, Y);
		DirectX::XMStoreFloat4x4(&WorldBuffer.World[0], DirectX::XMMatrixScaling(X, Y, 5.0f));
	}
};