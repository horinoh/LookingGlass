#pragma once

#include <Windows.h>

#include <iostream>
#include <vector>
#include <numbers>
#include <format>

#include <HoloPlayCore.h>
#include <HoloPlayShaders.h>

#ifdef _DEBUG
#define LOG(x) OutputDebugStringA((x))
#else
#define LOG(x)
#endif

#define TO_RADIAN(x) ((x) * std::numbers::pi_v<float> / 180.0f)

class Holo
{
public:
	Holo() {
		LOG(data(std::format("hpc_LightfieldVertShaderGLSL =\n{}\n", hpc_LightfieldVertShaderGLSL)));
		LOG(data(std::format("hpc_LightfieldFragShaderGLSL =\n{}\n", hpc_LightfieldFragShaderGLSL)));

		if (hpc_CLIERR_NOERROR == hpc_InitializeApp("Holo", hpc_LICENSE_NONCOMMERCIAL)) {
			{
				std::vector<char> Buf(hpc_GetStateAsJSON(nullptr, 0));
				hpc_GetStateAsJSON(data(Buf), size(Buf));
				LOG(data(std::format("hpc_GetStateAsJSON = {}\n", data(Buf))));
			}
			{
				std::vector<char> Buf(hpc_GetHoloPlayCoreVersion(nullptr, 0));
				hpc_GetHoloPlayCoreVersion(data(Buf), size(Buf));
				LOG(data(std::format("hpc_GetHoloPlayCoreVersion = {}\n", data(Buf))));
			}
			{
				std::vector<char> Buf(hpc_GetHoloPlayServiceVersion(nullptr, 0));
				hpc_GetHoloPlayServiceVersion(data(Buf), size(Buf));
				LOG(data(std::format("hpc_GetHoloPlayServiceVersion = {}\n", data(Buf))));
			}

			LOG(data(std::format("hpc_GetNumDevices = {}\n", hpc_GetNumDevices())));
			for (auto i = 0; i < hpc_GetNumDevices(); ++i) {
				LOG(data(std::format("[{}]\n", i)));
				{
					std::vector<char> Buf(hpc_GetDeviceHDMIName(i, nullptr, 0));
					hpc_GetDeviceHDMIName(i, data(Buf), size(Buf));
					LOG(data(std::format("\thpc_GetDeviceHDMIName = {}\n", data(Buf)))); //!< �Q�l�l) LKG-PO3996
				}
				{
					std::vector<char> Buf(hpc_GetDeviceSerial(i, nullptr, 0));
					hpc_GetDeviceSerial(i, data(Buf), size(Buf));
					LOG(data(std::format("\thpc_GetDeviceSerial = {}\n", data(Buf)))); //!< �Q�l�l) LKG-PORT-03996
				}
				{
					std::vector<char> Buf(hpc_GetDeviceType(i, nullptr, 0));
					hpc_GetDeviceType(i, data(Buf), size(Buf));
					LOG(data(std::format("\thpc_GetDeviceType = {}\n", data(Buf)))); //!< �Q�l�l) portrait
				}

				//!< �E�C���h�E�ʒu
				LOG(data(std::format("\thpc_GetDevicePropertyWinX, Y = {}, {}\n", hpc_GetDevicePropertyWinX(i), hpc_GetDevicePropertyWinY(i)))); //!< �Q�l�l) 1920, 0

				//!< �E�C���h�E������
				LOG(data(std::format("\thpc_GetDevicePropertyScreenW, H = {}, {}\n", hpc_GetDevicePropertyScreenW(i), hpc_GetDevicePropertyScreenH(i)))); //!< �Q�l�l) 1536, 2048

				//!< Display fringe correction uniform (currently only applicable to 15.6" Developer/Pro units)
				LOG(data(std::format("\thpc_GetDevicePropertyFringe = {}\n", hpc_GetDevicePropertyFringe(i))));//!< �Q�l�l) 0

				//!< ��{�I�� libHoloPlayCore.h �̓��e�͗�������K�v�͂Ȃ��Ə����Ă��邪�A�T���v���ł͎g���Ă���c
				//!< �r���[�̎�肤��͈� [-viewCone * 0.5f, viewCone * 0.5f]
				LOG(data(std::format("\thpc_GetDevicePropertyFloat(viewCone) = {}\n", hpc_GetDevicePropertyFloat(i, "/calibration/viewCone/value")))); //!< �Q�l�l) 40
			}
		}

		if (0 < hpc_GetNumDevices()) {
			//!< �����ł͍ŏ��̃f�o�C�X��I�� (Select 1st device here)
			DeviceIndex = 0;
			LOG(data(std::format("Selected DeviceIndex = {}\n", DeviceIndex)));

			//!< �L���g
			{
				std::vector<char> Buf(hpc_GetDeviceType(DeviceIndex, nullptr, 0));
				hpc_GetDeviceType(DeviceIndex, data(Buf), size(Buf));
				std::string_view TypeStr(data(Buf));

				if ("standard" == TypeStr) {
					LOG(data(std::format("{} : QuiltDim={}x{}, Render={}x{}\n", TypeStr, 4, 8, 2048, 2048)));
				}
				else if ("8k" == TypeStr) {
					LOG(data(std::format("{} : QuiltDim={}x{}, Render={}x{}\n", TypeStr, 5, 9, 8192, 8192)));
				}
				else if ("portrait" == TypeStr) {
					LOG(data(std::format("{} : QuiltDim={}x{}, Render={}x{}\n", TypeStr, 8, 6, 3360, 3360)));
				}
				else {
					LOG(data(std::format("{} : QuiltDim={}x{}, Render={}x{}\n", TypeStr, 5, 9, 4096, 4096)));
				}
			}

			{
				ViewCone = TO_RADIAN(hpc_GetDevicePropertyFloat(DeviceIndex, "/calibration/viewCone/value"));
			}
			//!< [ DX, VK ] ��x�ɕ`��ł���r���[��
			//!<	DX : D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE 16
			//!<	VK : VkPhysicalDeviceProperties.limits.maxViewports = 16
			//!<	�g�[�^���̃r���[���� 45 �� 48 �������肷��̂ŁA3 �񂭂炢�͕`�悷�邱�ƂɂȂ�
			//!<	�������́A�������� 16 �p�^�[�����`�悵�ĊԂ͕⊮����
		} else {
			LOG("[ Holo ] Device not found\n");
		}

		//!< �����`�L�����[
		LenticularBuffer = new LENTICULAR_BUFFER(DeviceIndex);
		if (nullptr != LenticularBuffer) {
			LOG(data(std::format("Pitch = {}\n", LenticularBuffer->Pitch)));
			LOG(data(std::format("Tilt = {}\n", LenticularBuffer->Tilt)));
			LOG(data(std::format("Center = {}\n", LenticularBuffer->Center)));
			LOG(data(std::format("Subp = {}\n", LenticularBuffer->Subp)));
			LOG(data(std::format("DisplayAspect = {}\n", LenticularBuffer->DisplayAspect)));
			LOG(data(std::format("InvView = {}\n", LenticularBuffer->InvView)));
			LOG(data(std::format("Ri, Bi = {}, {} ({})\n", LenticularBuffer->Ri, LenticularBuffer->Bi, (LenticularBuffer->Ri == 0 && LenticularBuffer->Bi == 2) ? "RGB" : "BGR")));
		}
	}
	virtual ~Holo() {
		if (nullptr != LenticularBuffer) {
			delete LenticularBuffer;
		}
		if (nullptr != HogeBuffer) {
			delete HogeBuffer;
		}
		hpc_CloseApp();
	}

	void SetHoloWindow(HWND hWnd, HINSTANCE hInstance)  
	{
		//!< �E�C���h�E�ʒu�A�T�C�Y�� Looking Glass ����擾���A���f����
		const auto Index = DeviceIndex;
		if (-1 != Index) {
			LOG(data(std::format("Win = ({}, {}) {} x {}\n", hpc_GetDevicePropertyWinX(Index), hpc_GetDevicePropertyWinY(Index), hpc_GetDevicePropertyScreenW(Index), hpc_GetDevicePropertyScreenH(Index))));
			::SetWindowPos(hWnd, nullptr, hpc_GetDevicePropertyWinX(Index), hpc_GetDevicePropertyWinY(Index), hpc_GetDevicePropertyScreenW(Index), hpc_GetDevicePropertyScreenH(Index), SWP_FRAMECHANGED);
			::ShowWindow(hWnd, SW_SHOW);
		} else {
			::SetWindowPos(hWnd, nullptr, /*1920*/0, 0, 1536, 2048, SWP_FRAMECHANGED);
			::ShowWindow(hWnd, SW_SHOW);
		}
	}
	
	virtual void UpdateLenticularBuffer(const float Column, const float Row, const uint64_t Width, const uint32_t Height) {
		LenticularBuffer->Column = Column;
		LenticularBuffer->Row = Row;
		LenticularBuffer->ColRow = LenticularBuffer->Column * LenticularBuffer->Row;

		const auto ViewWidth = Width / LenticularBuffer->Column;
		const auto ViewHeight = Height / LenticularBuffer->Row;
		LenticularBuffer->QuiltAspect = ViewWidth / ViewHeight;
		LenticularBuffer->QuiltAspect = LenticularBuffer->DisplayAspect;
		LOG(data(std::format("QuiltAspect = {}\n", LenticularBuffer->QuiltAspect)));
		LOG(data(std::format("ViewPortion = {} x {}\n", float(ViewWidth) * LenticularBuffer->Column / float(Width), float(ViewHeight) * LenticularBuffer->Row / float(Height))));

		//!< DX �ł� Y ����ł���A(�����ł�)VK �� DX �ɍ��킹�� Y ����ɂ��Ă����
		//!< Tilt �̒l�𐳂ɂ��邱�ƂŒ�������킹�Ă���
		LenticularBuffer->Tilt = abs(LenticularBuffer->Tilt);
	}

protected:
	int DeviceIndex = -1;

	float ViewCone = TO_RADIAN(40.0f);

	//!< �W�I���g���V�F�[�_�p�����[�^
	//struct HOLO_BUFFER_GS {
	//	HOLO_BUFFER_GS() {}
	//	HOLO_BUFFER_GS(const int Index) {
	//		Aspect = hpc_GetDevicePropertyDisplayAspect(Index);
	//		const auto ViewConeDegree = hpc_GetDevicePropertyFloat(Index, "/calibration/viewCone/value");
	//		ViewCone = TO_RADIAN(ViewConeDegree);
	//		
	//		LOG(data(std::format("\thpc_GetDevicePropertyDisplayAspect = {}\n", Aspect)));
	//		LOG(data(std::format("\thpc_GetDevicePropertyFloat(viewCone) = {}\n", ViewConeDegree)));
	//	}
	//	int ViewIndexOffset = 0;
	//	int ViewTotal;//GetQuiltSetting().GetViewTotal();
	//	float Aspect = 0.75f;
	//	float ViewCone = TO_RADIAN(40.0f);
	//};
	//HOLO_BUFFER_GS BufferGS;

	//!< �s�N�Z���V�F�[�_�p�����[�^
	struct LENTICULAR_BUFFER {
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

				{
					std::vector<char> Buf(hpc_GetDeviceType(Index, nullptr, 0));
					hpc_GetDeviceType(Index, data(Buf), size(Buf));
					std::string_view TypeStr(data(Buf));
					if ("standard" == TypeStr) {
						Column = 4;
						Row = 8;
						//QuiltSize = 2048; //!< �����_�[�^�[�Q�b�g�T�C�Y 2048x2048 = 512x256 x 4 x 8
					}
					else if ("8k" == TypeStr) {
						Column = 5;
						Row = 9;
						//QuiltSize = 8192; //!< �����_�[�^�[�Q�b�g�T�C�Y 8192x8192 = 1638x910 x 5 x 9
					}
					else if ("portrait" == TypeStr) {
						Column = 8;
						Row = 6;
						//QuiltSize = 3360; //!< �����_�[�^�[�Q�b�g�T�C�Y 3360x3360 = 420x560 x 8 x 6
					}
					else {
						Column = 5;
						Row = 9;
						//QuiltSize = 4096; //!< �����_�[�^�[�Q�b�g�T�C�Y 4096x4096 = 819x455 x 5 x 9
					}
					ColRow = Column * Row;
				}
			}
		}
		float Pitch = 246.866f;
		float Tilt = -0.185377f;
		float Center = 0.565845f;
		float Subp = 0.000217014f;

		float DisplayAspect = 0.75f;
		int InvView = 1;
		//!< Gi == 1 �͊m�� (RGB or BGR �Ƃ�������)
		int Ri = 0;
		int Bi = 2;

		float Column = 8;
		float Row = 6;
		float ColRow = Column * Row;
		float QuiltAspect = Column / Row;
	};
	LENTICULAR_BUFFER* LenticularBuffer = nullptr;

	struct HOGE_BUFFER {
		HOGE_BUFFER(const int Index) {
			if (-1 != Index) {
			}
		}
		float a = 0.0f;
	};
	HOGE_BUFFER* HogeBuffer = nullptr;
};
