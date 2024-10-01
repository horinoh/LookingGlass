#pragma once

#define USE_BRIDGE_SDK

//!< bridege.h �� Windows.h �����O�ɒu������
#include <bridge.h>
#include <HoloPlayCore.h>
#include <HoloPlayShaders.h>

#include <Windows.h>

#include <iostream>
#include <vector>
#include <numbers>
#include <format>
#include <cassert>

//!< �O����� (Prerequisites)
//!<	Looking Glass �� PC �ɐڑ����AOS �̃f�B�X�v���C�ݒ肩��A�u�\����ʂ��g������v�� Looking Glass ���g����ʂƂ��Ă�������
//!<	[Connect Looking Glass to PC, from OS display settings "extend screen", select Looking Glass as extended screen]
#include "Common.h"

//#define DISPLAY_QUILT
#ifdef DISPLAY_QUILT
//!< �L���g�����O�Ń����_�����O���Ă���ꍇ�A�ō��E�݂̂�\������ [Display most left, right quilt, only in case self rendering quilt]
//#define DISPLAY_MOST_LR_QUILT
#endif

#define CHECKDIMENSION(_TileXY) if (_TileXY > TileDimensionMax) { LOG(std::data(std::format("TileX * TileY ({}) > TileDimensionMax ({})\n", _TileXY, TileDimensionMax))); __debugbreak(); }

class Holo
{
public:
	Holo() {
#ifdef USE_BRIDGE_SDK
		//!< ������ [Initialize]
		if (!LKGCtrl.Initialize(TEXT("Holo"))) {
			LOG(std::data(std::format("Initialize failed\n")));
		}
		std::wcout << std::data(LKGCtrl.SettingsPath()) << std::endl;
		std::wcout << std::data(LKGCtrl.BridgeInstallLocation(BridgeVersion)) << std::endl;
		//LOG(std::data(std::format("SettingsPath = {}\n", std::data(LKGCtrl.SettingsPath()))));
		//LOG(std::data(std::format("BridgeInstallLocation = {}\n", std::data(LKGCtrl.BridgeInstallLocation(BridgeVersion)))));

		//!< �f�B�X�v���C�擾 [Get displays]
		int Count = 0;
		if (LKGCtrl.GetDisplays(&Count, nullptr)) {
			Displays.resize(Count);
			LOG(std::data(std::format("Displays\n")));
			if (LKGCtrl.GetDisplays(&Count, std::data(Displays)) && !std::empty(Displays)) {
				//!< �Ƃ肠�����ŏ��̂��I�� [Select first one]
				DisplayIndex = Displays[0];
				//!< �f�B�X�v���C��� [Enumerate displays]
				for (auto i : Displays) {
					LOG(std::data(std::format("\t{}{}\n", (i == DisplayIndex ? "> " : "  "), i)));
				}

				{
					auto StrCount = 0;
					//!< �f�o�C�X�� [Device name]
					if (LKGCtrl.GetDeviceNameForDisplay(DisplayIndex, &StrCount, nullptr)) {
						std::wstring DevName;
						DevName.resize(StrCount + 1);
						if (LKGCtrl.GetDeviceNameForDisplay(DisplayIndex, &StrCount, std::data(DevName))) {
							std::wcout << std::data(DevName) << std::endl;
							//LOG(std::data(std::format("DeviceName = {}\n", std::data(DevName))));
						}
					}
					//!< �V���A�� [Serial]
					if (LKGCtrl.GetDeviceSerialForDisplay(DisplayIndex, &StrCount, nullptr)) {
						std::wstring Serial;
						Serial.resize(StrCount + 1);
						if (LKGCtrl.GetDeviceSerialForDisplay(DisplayIndex, &StrCount, std::data(Serial))) {
							std::wcout << std::data(Serial) << std::endl;
							//LOG(std::data(std::format("Serial = {}\n", std::data(Serial))));
						}
					}
					//!< �n�[�h�E�G�A Enum
					int HardWareEnum;
					if (LKGCtrl.GetDeviceTypeForDisplay(DisplayIndex, &HardWareEnum)) {
						LOG(std::data(std::format("HardWareEnum = {}\n", HardWareEnum)));
					}
				}
#if false
				//!< �L�����u���[�V�����p�����[�^(�s�v) [Calibration parameters]
				auto Slope = 0.0f;
				auto Width = 0, Height = 0;
				auto Dpi = 0.0f, FlipX = 0.0f, Fringe = 0.0f;
				auto CellPatternMode = 0, NumberOfCells = 0;
				std::vector<CalibrationSubpixelCell> CSCs;
				if (LKGCtrl.GetCalibrationForDisplay(DisplayIndex, &LenticularBuffer.Center, &LenticularBuffer.Pitch, &Slope, &Width, &Height, &Dpi, &FlipX, &LenticularBuffer.InvView, &HalfViewCone, &Fringe, &CellPatternMode, &NumberOfCells, nullptr)) {
					WinWidth = Width;
					WinHeight = Height;
					CSCs.resize(NumberOfCells);
					if (LKGCtrl.GetCalibrationForDisplay(DisplayIndex, &LenticularBuffer.Center, &LenticularBuffer.Pitch, &Slope, &Width, &Height, &Dpi, &FlipX, &LenticularBuffer.InvView, &HalfViewCone, &Fringe, &CellPatternMode, &NumberOfCells, std::data(CSCs))) {
					}
				}
#endif
				if (!LKGCtrl.GetPitchForDisplay(DisplayIndex, &LenticularBuffer.Pitch)) {
					LOG(std::data(std::format("GetPitchForDisplay [NG]\n")));
				}
				if (!LKGCtrl.GetTiltForDisplay(DisplayIndex, &LenticularBuffer.Tilt)) {
					LOG(std::data(std::format("GetTiltForDisplay [NG]\n")));
				}
				if (!LKGCtrl.GetCenterForDisplay(DisplayIndex, &LenticularBuffer.Center)) {
					LOG(std::data(std::format("GetCenterForDisplay [NG]\n")));
				}
				if (!LKGCtrl.GetSubpForDisplay(DisplayIndex, &LenticularBuffer.Subp)) {
					LOG(std::data(std::format("GetSubpForDisplay [NG]\n")));
				}

				if (!LKGCtrl.GetDisplayAspectForDisplay(DisplayIndex, &LenticularBuffer.DisplayAspect)) {
					LOG(std::data(std::format("GetDisplayAspectForDisplay [NG]\n")));
				}
				if (!LKGCtrl.GetInvViewForDisplay(DisplayIndex, &LenticularBuffer.InvView)) {
					LOG(std::data(std::format("GetInvViewForDisplay [NG]\n")));
				}
				if (!LKGCtrl.GetRiForDisplay(DisplayIndex, &LenticularBuffer.Ri)) {
					LOG(std::data(std::format("GetRiForDisplay [NG]\n")));
				}
				if (!LKGCtrl.GetBiForDisplay(DisplayIndex, &LenticularBuffer.Bi)) {
					LOG(std::data(std::format("GetBiForDisplay [NG]\n")));
				}
				//!< �L���g�p�����[�^ [Quilt parameters]
				if (!LKGCtrl.GetDefaultQuiltSettingsForDisplay(DisplayIndex, &LenticularBuffer.QuiltAspect, &QuiltX, &QuiltY, &LenticularBuffer.TileX, &LenticularBuffer.TileY)) {
					LOG(std::data(std::format("GetDefaultQuiltSettingsForDisplay [NG]\n")));
				}

				//!< �r���[�R�[�� [View cone]
				if (!LKGCtrl.GetViewConeForDisplay(DisplayIndex, &HalfViewCone)) {
					LOG(std::data(std::format("GetViewConeForDisplay [NG]\n")));
				}

				//if (!LKGCtrl.GetFringeForDisplay(DisplayIndex, &Fringe)) {
				//	LOG(std::data(std::format("GetFringeForDisplay [NG]\n")));
				//}

				//!< �E�C���h�E�p�����[�^ [Window parameters]
				if (!LKGCtrl.GetWindowPositionForDisplay(DisplayIndex, &WinX, &WinY)) {
					LOG(std::data(std::format("GetWindowPositionForDisplay [NG]\n")));
				}
				if (!LKGCtrl.GetDimensionsForDisplay(DisplayIndex, &WinWidth, &WinHeight)) {
					LOG(std::data(std::format("GetDimensionsForDisplay [NG]\n")));
				}
			}
		}
#if true
		//!< Tilt �� CoreSDK �� BridgeSDK �ňقȂ� [Tilt value is different on BridegeSDK and CoreSDK...]
		//!< Tilt �� CoreSDK �̂��̂��̗p [Adopt tilt value from CoreSDK]
		if (hpc_CLIERR_NOERROR == hpc_InitializeApp("Holo", hpc_LICENSE_NONCOMMERCIAL)) {
			if (0 < hpc_GetNumDevices()) {
				//!< �Ƃ肠�����ŏ��̂��I�� [Select first one]
				const auto DeviceIndex = 0;
				const auto Tilt = hpc_GetDevicePropertyTilt(DeviceIndex);
				if (!std::isnan(Tilt)) {
					LenticularBuffer.Tilt = Tilt;
				}
			}
			hpc_CloseApp();
		}
#endif
#else
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
					LOG(std::data(std::format("\thpc_GetDeviceHDMIName = {}\n", std::data(Buf))));
				}
				{
					std::vector<char> Buf(hpc_GetDeviceSerial(i, nullptr, 0));
					hpc_GetDeviceSerial(i, std::data(Buf), std::size(Buf));
					LOG(std::data(std::format("\thpc_GetDeviceSerial = {}\n", std::data(Buf))));
				}
				{
					std::vector<char> Buf(hpc_GetDeviceType(i, nullptr, 0));
					hpc_GetDeviceType(i, std::data(Buf), std::size(Buf));
					LOG(std::data(std::format("\thpc_GetDeviceType = {}\n", std::data(Buf))));
				}
				//!< Display fringe correction uniform (currently only applicable to 15.6" Developer/Pro units)
				LOG(std::data(std::format("\thpc_GetDevicePropertyFringe = {}\n", hpc_GetDevicePropertyFringe(i))));
			}
			if (0 < hpc_GetNumDevices()) {
				//!< �����ł͍ŏ��̃f�o�C�X��I�� (Select 1st device here)
				const auto DeviceIndex = 0;

				//!< �����`�L�����[�p�����[�^ [Lenticular parameters]
				LenticularBuffer.Pitch = hpc_GetDevicePropertyPitch(DeviceIndex);
				LenticularBuffer.Tilt = hpc_GetDevicePropertyTilt(DeviceIndex);
				LenticularBuffer.Center = hpc_GetDevicePropertyCenter(DeviceIndex);
				LenticularBuffer.Subp = hpc_GetDevicePropertySubp(DeviceIndex);

				LenticularBuffer.DisplayAspect = hpc_GetDevicePropertyDisplayAspect(DeviceIndex);
				LenticularBuffer.InvView = hpc_GetDevicePropertyInvView(DeviceIndex);
				LenticularBuffer.Ri = hpc_GetDevicePropertyRi(DeviceIndex);
				LenticularBuffer.Bi = hpc_GetDevicePropertyBi(DeviceIndex);

				//!< �L���g�p�����[�^ [Quilt parameters]
				LenticularBuffer.TileX = hpc_GetDevicePropertyTileX(DeviceIndex);
				LenticularBuffer.TileY = hpc_GetDevicePropertyTileY(DeviceIndex);
				LenticularBuffer.QuiltAspect = hpc_GetDevicePropertyQuiltAspect(DeviceIndex);
				QuiltX = hpc_GetDevicePropertyQuiltX(DeviceIndex);
				QuiltY = hpc_GetDevicePropertyQuiltY(DeviceIndex);

				//!< �r���[�R�[�� [View cone]
				HalfViewCone = hpc_GetDevicePropertyFloat(DeviceIndex, "/calibration/viewCone/value");

				//!< �E�C���h�E�p�����[�^ [Window parameters]
				WinX = hpc_GetDevicePropertyWinX(DeviceIndex);
				WinY = hpc_GetDevicePropertyWinY(DeviceIndex);
				WinWidth = hpc_GetDevicePropertyScreenW(DeviceIndex);
				WinHeight = hpc_GetDevicePropertyScreenH(DeviceIndex);
			}
		}
#endif
		//!< �f�o�C�X���̃p�����[�^ (�f�o�b�O�p) [Each device parameter for debug]
		//SetStandardParam();
		//SetPortraitParam();
		//SetGoParam();

		//!< �f�o�b�O�o�� [Debug output]
		{
			//!< �����`�L�����[�p�����[�^ [Lenticular parameters]
			LOG(std::data(std::format("Pitch = {}\n", LenticularBuffer.Pitch)));
			LOG(std::data(std::format("Tilt = {}\n", LenticularBuffer.Tilt)));
			LOG(std::data(std::format("Center = {}\n", LenticularBuffer.Center)));
			LOG(std::data(std::format("Subp = {}\n", LenticularBuffer.Subp)));

			LOG(std::data(std::format("DisplayAspect = {}\n", LenticularBuffer.DisplayAspect)));
			LOG(std::data(std::format("InvView = {}\n", LenticularBuffer.InvView)));
			LOG(std::data(std::format("Ri = {}\n", LenticularBuffer.Ri)));
			LOG(std::data(std::format("Bi = {}\n", LenticularBuffer.Bi)));

			//!< �L���g�p�����[�^ [Quilt parameters]
			LOG(std::data(std::format("TileX x Y = {} x {}\n", LenticularBuffer.TileX, LenticularBuffer.TileY)));
			LOG(std::data(std::format("QuiltAspect = {}\n", LenticularBuffer.QuiltAspect)));
			LOG(std::data(std::format("QuiltX x Y = {} x {}\n", QuiltX, QuiltY)));

			//!< �r���[�R�[�� [View cone]
			LOG(std::data(std::format("ViewCone = {}\n", HalfViewCone)));

			//LOG(std::data(std::format("Fringe = {}\n", Fringe)));

			//!< �E�C���h�E�p�����[�^ [Window parameters]
			LOG(std::data(std::format("WinX, Y = {}, {}\n", WinX, WinY)));
			LOG(std::data(std::format("WinWidth x Height = {} x {}\n", WinWidth, WinHeight)));
		}

#ifdef DISPLAY_MOST_LR_QUILT
		//!< �f�o�b�O�\���p (�L���g���������炵�đ傫���\���A�ō��ƍŉE��2�p�^�[��) [For debug]
		LenticularBuffer.TileX = 2; LenticularBuffer.TileY = 1;
#endif

		//!< ���� [Adjustment]
		{
			//!< DX �ł� Y ����ł���A(�����ł�) VK �� DX �ɍ��킹�� Y ����ɂ��Ă���ׁATilt �̒l�𐳂ɂ��邱�ƂŒ�������킹�Ă��� [Here Y is up, so changing tilt value to positive]
			LenticularBuffer.Tilt *= -1.0f;
			LenticularBuffer.Center *= -1.0f;

			//!< InvView �͐^�U�𕄍��ɂ��ĕێ����Ă��� [Convert truth or falsehood to sign]
			LenticularBuffer.InvView = LenticularBuffer.InvView ? -1 : 1;
			//!< ���̎��_�ł͊p�x�������Ă���̂Ń��W�A���ϊ����� [Convert degree to radian]
			//!< �s�����悢�̂Ŕ��p�ɂ��Ċo���Ă��� [Save half radian for convenience]
			HalfViewCone = TO_RADIAN(HalfViewCone) * 0.5f;
		}

		TileXY = LenticularBuffer.TileX * LenticularBuffer.TileY;
		CHECKDIMENSION(TileXY);
		OffsetAngleCoef = 2.0f * HalfViewCone / (TileXY - 1.0f);
	}

	virtual ~Holo() {
#ifdef USE_BRIDGE_SDK
		LKGCtrl.Uninitialize();
#else
		hpc_CloseApp();
#endif
	}

	void SetHoloWindow(HWND hWnd, HINSTANCE hInstance) {
		::SetWindowPos(hWnd, nullptr, WinX, WinY, WinWidth, WinHeight, SWP_FRAMECHANGED);
		::ShowWindow(hWnd, SW_SHOW);
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

		TileXY = LenticularBuffer.TileX * LenticularBuffer.TileY;
		CHECKDIMENSION(TileXY);
	}

	virtual uint32_t GetMaxViewports() const { return 16; }
	uint32_t GetViewportDrawCount() const {
		const auto XY = static_cast<uint32_t>(TileXY);
		const auto ViewportCount = GetMaxViewports();
		return XY / ViewportCount + ((XY % ViewportCount) ? 1 : 0);
	}
	uint32_t GetViewportSetOffset(const uint32_t i) const { return GetMaxViewports() * i; }
	uint32_t GetViewportSetCount(const uint32_t i) const {
		return (std::min)(static_cast<uint32_t>(TileXY) - GetViewportSetOffset(i), GetMaxViewports());
	}

	virtual void CreateProjectionMatrix(const int i) {}
	void CreateProjectionMatrices() {
		for (auto i = 0; i < TileXY; ++i) {
			CreateProjectionMatrix(i);
		}
	}
	virtual void CreateViewMatrix(const int i) {}
	void CreateViewMatrices() {
		for (auto i = 0; i < TileXY; ++i) {
			CreateViewMatrix(i);
		}
	}
	virtual void UpdateViewProjectionBuffer() {}

	virtual bool GetXYScaleForDevice(float& X, float& Y) {
		constexpr auto Scale = 0.65f;
		X = LenticularBuffer.TileY * Scale;
		Y = LenticularBuffer.TileX * Scale;
		return true;
	}
	float GetOffsetAngle(const int i) const { return static_cast<const float>(i) * OffsetAngleCoef - HalfViewCone; }

	//!< HardWareEnum = 0, Serial = L"LKG02gh1ce45f"
	void SetStandardParam() {
		LenticularBuffer.Pitch = 354.5659f;
		LenticularBuffer.Tilt = -13.943652f; // BridgeSDK : -13.9436522f
		LenticularBuffer.Center = -0.19972825f;
		LenticularBuffer.Subp = 0.00013020834f;

		LenticularBuffer.DisplayAspect = 1.6f;
		LenticularBuffer.InvView = 1;
		LenticularBuffer.Ri = 0;
		LenticularBuffer.Bi = 2;
		LenticularBuffer.TileX = 5;
		LenticularBuffer.TileY = 9;
		LenticularBuffer.QuiltAspect = 1.6f;

		QuiltX = 4096;
		QuiltY = 4096;
		HalfViewCone = 40.0f;
		WinX = 0; //1920
		WinY = 0;
		WinWidth = 2560;
		WinHeight = 1600;
	}
	//!< HardWareEnum = 4, Serial = L"LKG-P03996"
	void SetPortraitParam() {
		LenticularBuffer.Pitch = 246.866f;
		LenticularBuffer.Tilt = -0.185377f; //!< BridgeSDK : -4.04581785f
		LenticularBuffer.Center = 0.565845f;
		LenticularBuffer.Subp = 0.000217014f;

		LenticularBuffer.DisplayAspect = 0.75f;
		LenticularBuffer.InvView = 1;
		LenticularBuffer.Ri = 0;
		LenticularBuffer.Bi = 2;
		LenticularBuffer.TileX = 8;
		LenticularBuffer.TileY = 6;
		LenticularBuffer.QuiltAspect = 0.75f;

		QuiltX = 3360; 
		QuiltY = 3360;
		HalfViewCone = 40.0f;
		WinX = 0; //1920
		WinY = 0;
		WinWidth = 1536; 
		WinHeight = 2048;
	}
	//!< HardWareEnum = 10, Serial = L"LKG-E05304"
	void SetGoParam() {
		LenticularBuffer.Pitch = 234.218f;
		LenticularBuffer.Tilt = -0.26678094f; //!< BridgeSDK : -2.10847139f
		LenticularBuffer.Center = 0.131987f;
		LenticularBuffer.Subp = 0.000231481f;

		LenticularBuffer.DisplayAspect = 0.5625f;
		LenticularBuffer.InvView = 1;
		LenticularBuffer.Ri = 0;
		LenticularBuffer.Bi = 2;
		LenticularBuffer.TileX = 11;
		LenticularBuffer.TileY = 6;
		LenticularBuffer.QuiltAspect = 0.5625f;

		QuiltX = 4092;
		QuiltY = 4092; 
		HalfViewCone = 54.0f;
		WinX = 0; //1920
		WinY = 0;
		WinWidth = 1440;
		WinHeight = 2560;
	}

protected:
	//!< �ύX������V�F�[�_���ɂ��Ή����K�v�ɂȂ�̂Œ��� [If change here, need to change shaders too]
	static constexpr int TileDimensionMax = 16 * 5; //!< 16 Views * 5 Draw call
	
	Controller LKGCtrl;
	std::vector<unsigned long> Displays;
	unsigned long DisplayIndex = 0;

	struct alignas(16) LENTICULAR_BUFFER {
		float Pitch = 246.866f;
		float Tilt = -0.185377f;
		float Center = 0.565845f;
		float Subp = 0.000217014f;

		float DisplayAspect = 0.75f;
		int InvView = 1;
		int Ri = 0;
		int Bi = 2;
		int TileX = 8, TileY = 6;
		float QuiltAspect = 0.75f; //!< static_cast<float>(TileX) / TileY;
	};
	LENTICULAR_BUFFER LenticularBuffer;

	int QuiltX = 3360, QuiltY = 3360;
	float HalfViewCone = 40.0f; //!< ���̎��_�ł͑f�̊p�x���w�� [For now save as degree]
	long WinX = 0, WinY = 0;
	unsigned long WinWidth = 1536, WinHeight = 2048;

	int TileXY = 0;
	float OffsetAngleCoef = 0.0f;
	const float Fov = TO_RADIAN(14.0f);
	const float CameraSize = 5.0f; //!< �œ_�ʂ̐������a [Focal plane vertical radius]
	const float CameraDistance = -CameraSize / std::tan(Fov / 2.0f); //!< �œ_�ʂ̐������a�ƁA����p����J�����������Z�o
};