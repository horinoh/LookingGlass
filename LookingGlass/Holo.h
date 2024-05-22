#pragma once

#include <Windows.h>

#include <iostream>
#include <vector>
#include <numbers>
#include <format>

#include <HoloPlayCore.h>
#include <HoloPlayShaders.h>

#include "Common.h"

#define DISPLAY_QUILT

class Holo
{
public:
	Holo() {
		LOG(std::data(std::format("hpc_LightfieldVertShaderGLSL =\n{}\n", hpc_LightfieldVertShaderGLSL)));
		LOG(std::data(std::format("hpc_LightfieldFragShaderGLSL =\n{}\n", hpc_LightfieldFragShaderGLSL)));

		if (hpc_CLIERR_NOERROR == hpc_InitializeApp("Holo", hpc_LICENSE_NONCOMMERCIAL)) {
			{
				std::vector<char> Buf(hpc_GetStateAsJSON(nullptr, 0));
				hpc_GetStateAsJSON(std::data(Buf), std::size(Buf));
				LOG(std::data(std::format("hpc_GetStateAsJSON = {}\n", std::data(Buf))));
			}
			{
				std::vector<char> Buf(hpc_GetHoloPlayCoreVersion(nullptr, 0));
				hpc_GetHoloPlayCoreVersion(std::data(Buf), std::size(Buf));
				LOG(std::data(std::format("hpc_GetHoloPlayCoreVersion = {}\n", std::data(Buf))));
			}
			{
				std::vector<char> Buf(hpc_GetHoloPlayServiceVersion(nullptr, 0));
				hpc_GetHoloPlayServiceVersion(std::data(Buf), std::size(Buf));
				LOG(std::data(std::format("hpc_GetHoloPlayServiceVersion = {}\n", std::data(Buf))));
			}

			LOG(std::data(std::format("hpc_GetNumDevices = {}\n", hpc_GetNumDevices())));
			for (auto i = 0; i < hpc_GetNumDevices(); ++i) {
				LOG(std::data(std::format("[{}]\n", i)));
				{
					std::vector<char> Buf(hpc_GetDeviceHDMIName(i, nullptr, 0));
					hpc_GetDeviceHDMIName(i, std::data(Buf), std::size(Buf));
					LOG(std::data(std::format("\thpc_GetDeviceHDMIName = {}\n", std::data(Buf)))); //!< 参考値) LKG-PO3996
				}
				{
					std::vector<char> Buf(hpc_GetDeviceSerial(i, nullptr, 0));
					hpc_GetDeviceSerial(i, std::data(Buf), std::size(Buf));
					LOG(std::data(std::format("\thpc_GetDeviceSerial = {}\n", std::data(Buf)))); //!< 参考値) LKG-PORT-03996
				}
				{
					std::vector<char> Buf(hpc_GetDeviceType(i, nullptr, 0));
					hpc_GetDeviceType(i, std::data(Buf), std::size(Buf));
					LOG(std::data(std::format("\thpc_GetDeviceType = {}\n", std::data(Buf)))); //!< 参考値) portrait
					//!< "standard"	4x8	2048x2048
					//!< "8k"		5x9	8192x8192
					//!< "portrait"	8x6	3360x3360
					//!<			5x9	4096x4096
				}
				//!< Display fringe correction uniform (currently only applicable to 15.6" Developer/Pro units)
				LOG(std::data(std::format("\thpc_GetDevicePropertyFringe = {}\n", hpc_GetDevicePropertyFringe(i))));//!< 参考値) 0
			}
		}

		if (0 < hpc_GetNumDevices()) {
			//!< ここでは最初のデバイスを選択 (Select 1st device here)
			DeviceIndex = 0;
			LOG(std::data(std::format("Selected DeviceIndex = {}\n", DeviceIndex)));
		} else {
			LOG("Device not found\n");
		}

		LenticularBuffer = LENTICULAR_BUFFER(DeviceIndex);
		LOG(std::data(std::format("Pitch = {}\n", LenticularBuffer.Pitch)));
		LOG(std::data(std::format("Tilt = {}\n", LenticularBuffer.Tilt)));
		LOG(std::data(std::format("Center = {}\n", LenticularBuffer.Center)));
		LOG(std::data(std::format("Subp = {}\n", LenticularBuffer.Subp)));
		LOG(std::data(std::format("DisplayAspect = {}\n", LenticularBuffer.DisplayAspect)));
		LOG(std::data(std::format("InvView = {}\n", LenticularBuffer.InvView)));
		LOG(std::data(std::format("Ri, Bi = {}, {} ({})\n", LenticularBuffer.Ri, LenticularBuffer.Bi, (LenticularBuffer.Ri == 0 && LenticularBuffer.Bi == 2) ? "RGB" : "BGR")));
		LOG(std::data(std::format("TileX, TileY = {}, {}\n", LenticularBuffer.TileX, LenticularBuffer.TileY)));
		LOG(std::data(std::format("QuiltAspect = {}\n", LenticularBuffer.QuiltAspect)));

		if (-1 != DeviceIndex) {
			QuiltX = hpc_GetDevicePropertyQuiltX(DeviceIndex);
			QuiltY = hpc_GetDevicePropertyQuiltY(DeviceIndex);
			ViewCone = TO_RADIAN(hpc_GetDevicePropertyFloat(DeviceIndex, "/calibration/viewCone/value"));
		}
		LOG(std::data(std::format("QuiltX, QuiltY = {}, {}\n", QuiltX, QuiltY)));
		LOG(std::data(std::format("ViewCone = {}\n", ViewCone)));
	}
	virtual ~Holo() {
		hpc_CloseApp();
	}

	void SetHoloWindow(HWND hWnd, HINSTANCE hInstance)  
	{
		//!< ウインドウ位置、サイズを Looking Glass から取得し、反映する [Get window position, size from Looking Glass and apply]
		if (-1 != DeviceIndex) {
			LOG(std::data(std::format("Win = ({}, {}) {} x {}\n", hpc_GetDevicePropertyWinX(DeviceIndex), hpc_GetDevicePropertyWinY(DeviceIndex), hpc_GetDevicePropertyScreenW(DeviceIndex), hpc_GetDevicePropertyScreenH(DeviceIndex))));
			::SetWindowPos(hWnd, nullptr, hpc_GetDevicePropertyWinX(DeviceIndex), hpc_GetDevicePropertyWinY(DeviceIndex), hpc_GetDevicePropertyScreenW(DeviceIndex), hpc_GetDevicePropertyScreenH(DeviceIndex), SWP_FRAMECHANGED);	
		} else {
			::SetWindowPos(hWnd, nullptr, 0, 0, 1536, 2048, SWP_FRAMECHANGED);
		}
		::ShowWindow(hWnd, SW_SHOW);
		//::ShowWindow(hWnd, SW_SHOWMAXIMIZED);
	}
	
	virtual void UpdateLenticularBuffer(const int Column, const int Row, const int Width, const int Height) {
		LenticularBuffer.TileX = Column;
		LenticularBuffer.TileY = Row;
		QuiltX = Width;
		QuiltY = Height;
		const auto ViewWidth = static_cast<float>(QuiltX) / LenticularBuffer.TileX;
		const auto ViewHeight = static_cast<float>(QuiltY) / LenticularBuffer.TileY;
		LenticularBuffer.QuiltAspect = ViewWidth / ViewHeight;
		LOG(std::data(std::format("QuiltAspect = {}\n", ViewWidth / ViewHeight)));
		LOG(std::data(std::format("ViewPortion = {} x {}\n", static_cast<float>(ViewWidth) * LenticularBuffer.TileX / static_cast<float>(QuiltX), static_cast<float>(ViewHeight) * LenticularBuffer.TileY / static_cast<float>(QuiltY))));
	}

	virtual uint32_t GetViewportMax() const { return 16; }
	uint32_t GetViewportDrawCount() const {
		const auto XY = static_cast<uint32_t>(LenticularBuffer.TileX * LenticularBuffer.TileY);
		const auto ViewportCount = GetViewportMax();
		return XY / ViewportCount + ((XY % ViewportCount) ? 1 : 0);
	}
	uint32_t GetViewportSetOffset(const uint32_t i) const { return GetViewportMax() * i; }
	uint32_t GetViewportSetCount(const uint32_t i) const {
		return (std::min)(static_cast<int32_t>(LenticularBuffer.TileX * LenticularBuffer.TileY) - GetViewportSetOffset(i), GetViewportMax());
	}

	virtual void CreateProjectionMatrix(const int i) {}
	void CreateProjectionMatrices() {
		CreateProjectionMatrix(-1);
		const auto XY = static_cast<int>(LenticularBuffer.TileX * LenticularBuffer.TileY);
		for (auto i = 0; i < XY; ++i) {
			CreateProjectionMatrix(i);
		}
	}
	virtual void CreateViewMatrix(const int i) {}
	void CreateViewMatrices() {
		CreateViewMatrix(-1);
		const auto XY = static_cast<int>(LenticularBuffer.TileX * LenticularBuffer.TileY);
		for (auto i = 0; i < XY; ++i) {
			CreateViewMatrix(i);
		}
	}
	virtual void UpdateViewProjectionBuffer() {}

protected:
	int DeviceIndex = -1;

	int QuiltX = 3360, QuiltY = 3360;
	float ViewCone = TO_RADIAN(40.0f);

	const float Fov = TO_RADIAN(14.0f);
	const float CameraSize = 5.0f; //!< 焦点面の垂直半径 [Focal plane vertical radius]
	const float CameraDistance = -CameraSize / std::tan(Fov / 2.0f); //!< 焦点面の垂直半径と、視野角からカメラ距離を算出

	struct LENTICULAR_BUFFER {
		LENTICULAR_BUFFER() {
			//!< DX では Y が上であり、(ここでは) VK も DX に合わせて Y が上にしている為、Tilt の値を正にすることで辻褄を合わせている [Here Y is up, so changing tilt value to positive]
			Tilt = abs(Tilt);
		}
		LENTICULAR_BUFFER(const int Index) {
			if (-1 != Index) {
				Pitch = hpc_GetDevicePropertyPitch(Index);
				Tilt = hpc_GetDevicePropertyTilt(Index);
				Center = hpc_GetDevicePropertyCenter(Index);
				Subp = hpc_GetDevicePropertySubp(Index);

				DisplayAspect = hpc_GetDevicePropertyDisplayAspect(Index);
				InvView = hpc_GetDevicePropertyInvView(Index) ? -1 : 1;
				Ri = hpc_GetDevicePropertyRi(Index);
				Bi = hpc_GetDevicePropertyBi(Index);

				TileX = hpc_GetDevicePropertyTileX(Index);
				TileY = hpc_GetDevicePropertyTileY(Index);
				QuiltAspect = hpc_GetDevicePropertyQuiltAspect(Index);
			}
			Tilt = abs(Tilt);
		}
		float Pitch = 246.866f;
		float Tilt = -0.185377f;
		float Center = 0.565845f;
		float Subp = 0.000217014f;

		float DisplayAspect = 0.75f;
		int InvView = 1;
		int Ri = 0;
		int Bi = 2;

#if 0
		int TileX = 8, TileY = 6;
#else
		int TileX = 2, TileY = 1; //!< デバッグ表示用 (キルト分割を減らして大きく表示、最左と最右の2パターン) [For debug]
#endif
		float QuiltAspect = static_cast<float>(TileX) / TileY;
		float Padding;
	};
	LENTICULAR_BUFFER LenticularBuffer;
};
