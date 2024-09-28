#pragma once

#include "resource.h"

#define USE_DDS_TEXTURE
#include "../HoloDX.h"

#define USE_CV
#define USE_CUDA
#include "../CVDX.h"

#define USE_LEAP
#include "../Leap.h"

class DisplacementLeapDX : public DisplacementDX, public LeapCV
{
private:
	using Super = DisplacementDX;
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

		constexpr auto HasColorMap = true;

		constexpr auto Layers = 1;
		AnimatedTextures.emplace_back().Create(COM_PTR_GET(Device), DisparityImage.cols, DisparityImage.rows, static_cast<UINT>(DisparityImage.elemSize()), Layers, DXGI_FORMAT_R8_UNORM, 
			//!< カラーマップ専用テクスチャが無い場合は、ピクセルシェーダでも再利用する
			HasColorMap ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		
		if (HasColorMap) {
			const auto& ColorMap = StereoImages[0];
			AnimatedTextures.emplace_back().Create(COM_PTR_GET(Device), ColorMap.cols, ColorMap.rows, static_cast<UINT>(ColorMap.elemSize()), Layers, DrawGrayScale() ? DXGI_FORMAT_R8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
	}

	virtual const Texture& GetColorMap() const override { return std::size(AnimatedTextures) > 1 ? AnimatedTextures[1]:AnimatedTextures[0]; };
	virtual const Texture& GetDepthMap() const override { return AnimatedTextures[0]; };
	virtual bool DrawGrayScale() const override { return true; }

	virtual void DrawFrame(const UINT i) override {
		if (0 == i) {
			Mutex.lock();
			{
				constexpr auto Layers = 1;
				AnimatedTextures[0].UpdateUploadBuffer(DisparityImage.cols, DisparityImage.rows, static_cast<UINT>(DisparityImage.elemSize()), Layers, DisparityImage.ptr());

				if (std::size(AnimatedTextures) > 1) {
					const auto& ColorMap = StereoImages[0];
					AnimatedTextures[1].UpdateUploadBuffer(ColorMap.cols, ColorMap.rows, static_cast<UINT>(ColorMap.elemSize()), Layers, ColorMap.ptr());
				}
			}
			Mutex.unlock();
		}
	}
	virtual void PopulateAnimatedTextureCommand(const size_t i) override {
		const auto DCL = COM_PTR_GET(DirectCommandLists[i]);

		constexpr auto Bpp = 1;
		AnimatedTextures[0].PopulateUploadToTextureCommand(DCL, Bpp);
		if (std::size(AnimatedTextures) > 1) {
			AnimatedTextures[1].PopulateUploadToTextureCommand(DCL, Bpp);
		}
	}
	virtual void UpdateWorldBuffer() override {
		auto X = 1.0f, Y = 1.0f;
		GetXYScaleForDevice(X, Y);
		DirectX::XMStoreFloat4x4(&WorldBuffer.World[0], DirectX::XMMatrixScaling(X, Y, 5.0f));
	}
};