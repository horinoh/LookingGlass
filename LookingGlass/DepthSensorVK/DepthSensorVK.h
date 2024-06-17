#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloVK.h"

#define USE_CV
#include "../CVVK.h"

#define USE_DEPTH_SENSOR
#include "../DepthSensor.h"

#ifdef USE_DEPTH_SENSOR
class DepthSensorVK : public DisplacementWldVK
{
};

//!< 要深度センサ MaixSense A010 (Need depth sensor MaixSense A010)
class DepthSensorA010VK : public DepthSensorVK, public DepthSensorA010
{
private:
	using Super = DepthSensorVK;
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		//!< 深度センサオープン (COM番号は調べて、適切に指定する必要がある) (Select appropriate com no)
		Open(COM::COM3);

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
	virtual void UpdateWorldBuffer() override {
		float X, Y;
		GetXYScaleForDevice(X, Y);
		//!< ディスプレースメント (Z) を顕著にしてみる (More remarkable displacement Z)
		WorldBuffer.World[0] = glm::scale(glm::mat4(1.0f), glm::vec3(X, Y, 5.0f));
	}
};

class DepthSensorA075VK : public DepthSensorVK, public DepthSensorA075
{
private:
	using Super = DepthSensorVK;
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
		AnimatedTextures.emplace_back().Create(Device, CurrentPhysicalDeviceMemoryProperties, VK_FORMAT_R8_UNORM, Bpp, VkExtent3D({ .width = 320, .height = 240, .depth = 1 }));
	}
	virtual const Texture& GetColorMap() const override { return AnimatedTextures[0]; };
	virtual const Texture& GetDepthMap() const override { return AnimatedTextures[0]; };
	virtual bool DrawGrayScale() const override { return true; }

	virtual void DrawFrame(const UINT i) override {
		if (0 == i) {
			constexpr auto Bpp = 1;
			//AnimatedTextures[0].UpdateStagingBuffer(Device, 320 * 240 * Bpp, std::data(...));
		}
	}
	virtual void PopulateAnimatedTextureCommand(const size_t i) override {
		const auto CB = CommandBuffers[i];
		AnimatedTextures[0].PopulateStagingToImageCommand(CB, 320, 240, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
	}
	virtual void UpdateWorldBuffer() override {
		float X, Y;
		GetXYScaleForDevice(X, Y);
		//!< ディスプレースメント (Z) を顕著にしてみる (More remarkable displacement Z)
		WorldBuffer.World[0] = glm::scale(glm::mat4(1.0f), glm::vec3(X, Y, 5.0f));
	}
};
#endif