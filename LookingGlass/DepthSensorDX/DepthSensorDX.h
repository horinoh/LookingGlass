#pragma once

#include "resource.h"

#define USE_DDS_TEXTURE
#include "../HoloDX.h"

#define USE_CV
#include "../CVDX.h"

#define USE_DEPTH_SENSOR
#include "../DepthSensor.h"

#ifdef USE_DEPTH_SENSOR
using DepthSensorDX = DisplacementDX;
//!< 要深度センサ MaixSense A010 (Need depth sensor MaixSense A010)
class DepthSensorA010DX : public DepthSensorDX, public DepthSensorA010
{
private:
	using Super = DepthSensorDX;
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		//!< 深度センサオープン (COM番号は調べて、適切に指定する必要がある) (Select appropriate com no)
		Open(COM_NO::COM3);

		//!< 非同期更新開始 (Start async update)
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
		constexpr auto Layers = 1;
		AnimatedTextures.emplace_back().Create(COM_PTR_GET(Device), Frame.Header.Cols, Frame.Header.Rows, Bpp, Layers, DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}

	virtual const Texture& GetColorMap() const override { return AnimatedTextures[0]; };
	virtual const Texture& GetDepthMap() const override { return AnimatedTextures[0]; };
	virtual bool DrawGrayScale() const override { return true; }

	virtual void DrawFrame(const UINT i) override {
		if (0 == i) {
			Mutex.lock(); {
				constexpr auto Bpp = 1;
				constexpr auto Layers = 1;
				AnimatedTextures[0].UpdateUploadBuffer(Frame.Header.Cols, Frame.Header.Rows, Bpp, Layers, std::data(Frame.Payload));
			} Mutex.unlock();
		}
	}
	virtual void PopulateAnimatedTextureCommand(const size_t i) override {
		const auto DCL = COM_PTR_GET(DirectCommandLists[i]);

		constexpr auto Bpp = 1;
		AnimatedTextures[0].PopulateUploadToTextureCommand(DCL, Bpp);
	}
	virtual void UpdateWorldBuffer() override {
		auto X = 1.0f, Y = 1.0f;
		GetXYScaleForDevice(X, Y);
		//!< ディスプレースメント (Z) を顕著にしてみる (More remarkable displacement Z)
		DirectX::XMStoreFloat4x4(&WorldBuffer.World[0], DirectX::XMMatrixScaling(X, Y, 5.0f));
	}
};
class DepthSensorA075DX : public DepthSensorDX, public DepthSensorA075
{
private:
	using Super = DepthSensorDX;
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		//!< 非同期更新開始 (Start async update)
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
		constexpr auto Layers = 1;
		AnimatedTextures.emplace_back().Create(COM_PTR_GET(Device), 320, 240, Bpp, Layers, DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}

	virtual const Texture& GetColorMap() const override { return AnimatedTextures[0]; };
	virtual const Texture& GetDepthMap() const override { return AnimatedTextures[0]; };
	virtual bool DrawGrayScale() const override { return true; }

	virtual void DrawFrame(const UINT i) override {
		if (0 == i) {
			constexpr auto Bpp = 1;
			constexpr auto Layers = 1;
			//AnimatedTextures[0].UpdateUploadBuffer(320, 240, Bpp, Layers, std::data(...));
		}
	}
	virtual void PopulateAnimatedTextureCommand(const size_t i) override {
		const auto DCL = COM_PTR_GET(DirectCommandLists[i]);

		constexpr auto Bpp = 1;
		AnimatedTextures[0].PopulateUploadToTextureCommand(DCL, Bpp);
	}
	virtual void UpdateWorldBuffer() override {
		auto X = 1.0f, Y = 1.0f;
		GetXYScaleForDevice(X, Y);
		//!< ディスプレースメント (Z) を顕著にしてみる (More remarkable displacement Z)
		DirectX::XMStoreFloat4x4(&WorldBuffer.World[0], DirectX::XMMatrixScaling(X, Y, 5.0f));
	}
};
#endif