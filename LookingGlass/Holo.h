#pragma once

#include <Windows.h>

#include <iostream>
#include <vector>
#include <numbers>
#include <format>

#include <HoloPlayCore.h>
#include <HoloPlayShaders.h>

#include "Common.h"

//#define DISPLAY_QUILT
#ifdef DISPLAY_QUILT
//!< �L���g�����O�Ń����_�����O���Ă���ꍇ�A�ō��E�݂̂�\������ (Display most left, right quilt, only in case self rendering quilt)
#define DISPLAY_MOST_LR_QUILT
#endif

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
					LOG(std::data(std::format("\thpc_GetDeviceHDMIName = {}\n", std::data(Buf)))); //!< �Q�l�l) LKG-PO3996
				}
				{
					std::vector<char> Buf(hpc_GetDeviceSerial(i, nullptr, 0));
					hpc_GetDeviceSerial(i, std::data(Buf), std::size(Buf));
					LOG(std::data(std::format("\thpc_GetDeviceSerial = {}\n", std::data(Buf)))); //!< �Q�l�l) LKG-PORT-03996
				}
				{
					std::vector<char> Buf(hpc_GetDeviceType(i, nullptr, 0));
					hpc_GetDeviceType(i, std::data(Buf), std::size(Buf));
					LOG(std::data(std::format("\thpc_GetDeviceType = {}\n", std::data(Buf)))); //!< �Q�l�l) portrait
					//!< "standard"	4x8	2048x2048
					//!< "8k"		5x9	8192x8192
					//!< "portrait"	8x6	3360x3360
					//!<			5x9	4096x4096
				}
				//!< Display fringe correction uniform (currently only applicable to 15.6" Developer/Pro units)
				LOG(std::data(std::format("\thpc_GetDevicePropertyFringe = {}\n", hpc_GetDevicePropertyFringe(i))));//!< �Q�l�l) 0
			}
		}

		if (0 < hpc_GetNumDevices()) {
			//!< �����ł͍ŏ��̃f�o�C�X��I�� (Select 1st device here)
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
			HalfViewCone = TO_RADIAN(hpc_GetDevicePropertyFloat(DeviceIndex, "/calibration/viewCone/value")) * 0.5f;
		}
		LOG(std::data(std::format("QuiltX, QuiltY = {}, {}\n", QuiltX, QuiltY)));
		LOG(std::data(std::format("ViewCone = {}\n", 2.0f * HalfViewCone)));

		OffsetAngleCoef = 2.0f * HalfViewCone / (LenticularBuffer.TileX * LenticularBuffer.TileY - 1.0f);
	}
	virtual ~Holo() {
		hpc_CloseApp();
	}

	void SetHoloWindow(HWND hWnd, HINSTANCE hInstance)  
	{
		//!< �E�C���h�E�ʒu�A�T�C�Y�� Looking Glass ����擾���A���f���� [Get window position, size from Looking Glass and apply]
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

	virtual uint32_t GetMaxViewports() const { return 16; }
	uint32_t GetViewportDrawCount() const {
		const auto XY = static_cast<uint32_t>(LenticularBuffer.TileX * LenticularBuffer.TileY);
		const auto ViewportCount = GetMaxViewports();
		return XY / ViewportCount + ((XY % ViewportCount) ? 1 : 0);
	}
	uint32_t GetViewportSetOffset(const uint32_t i) const { return GetMaxViewports() * i; }
	uint32_t GetViewportSetCount(const uint32_t i) const {
		return (std::min)(static_cast<uint32_t>(LenticularBuffer.TileX * LenticularBuffer.TileY) - GetViewportSetOffset(i), GetMaxViewports());
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

	virtual bool GetXYScaleForDevice(float& X, float& Y) {
		X = 6.0f * 0.65f; Y = 8.0f * 0.65f;
		if (-1 != DeviceIndex) {
			std::vector<char> Buf(hpc_GetDeviceType(DeviceIndex, nullptr, 0));
			hpc_GetDeviceType(DeviceIndex, std::data(Buf), std::size(Buf));
			if (0 == strncmp(std::data(Buf), "standard", std::size(Buf))) {
				X = 8.0f; Y = 4.0f;
			}
			else if (0 == strncmp(std::data(Buf), "portrait", std::size(Buf))) {
				X = 6.0f * 0.65f; Y = 8.0f * 0.65f;
			}
			else if (0 == strncmp(std::data(Buf), "8k", std::size(Buf))) {
				X = 9.0f; Y = 5.0f;
			}
			else {
				LOG(std::data(std::format("{} is not supported\n", std::data(Buf))));
				//!< #TODO Add support
				__debugbreak();
			}
			return true;
		}
		return false;
	}
	float GetOffsetAngle(const int i) const { return static_cast<const float>(i) * OffsetAngleCoef - HalfViewCone; }

protected:
	int DeviceIndex = -1;

	int QuiltX = 3360, QuiltY = 3360;
	float HalfViewCone = TO_RADIAN(40.0f) * 0.5f;
	float OffsetAngleCoef = 0.0f;

	const float Fov = TO_RADIAN(14.0f);
	const float CameraSize = 5.0f; //!< �œ_�ʂ̐������a [Focal plane vertical radius]
	const float CameraDistance = -CameraSize / std::tan(Fov / 2.0f); //!< �œ_�ʂ̐������a�ƁA����p����J�����������Z�o

	struct LENTICULAR_BUFFER {
		LENTICULAR_BUFFER() {
			//!< DX �ł� Y ����ł���A(�����ł�) VK �� DX �ɍ��킹�� Y ����ɂ��Ă���ׁATilt �̒l�𐳂ɂ��邱�ƂŒ�������킹�Ă��� [Here Y is up, so changing tilt value to positive]
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

#ifdef DISPLAY_MOST_LR_QUILT
		int TileX = 2, TileY = 1; //!< �f�o�b�O�\���p (�L���g���������炵�đ傫���\���A�ō��ƍŉE��2�p�^�[��) [For debug]
#else
		int TileX = 8, TileY = 6;
#endif
		float QuiltAspect = static_cast<float>(TileX) / TileY;
		float Padding;
	};
	LENTICULAR_BUFFER LenticularBuffer;
};
