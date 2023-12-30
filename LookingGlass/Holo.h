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
					LOG(data(std::format("\thpc_GetDeviceHDMIName = {}\n", data(Buf)))); //!< 参考値) LKG-PO3996
				}
				{
					std::vector<char> Buf(hpc_GetDeviceSerial(i, nullptr, 0));
					hpc_GetDeviceSerial(i, data(Buf), size(Buf));
					LOG(data(std::format("\thpc_GetDeviceSerial = {}\n", data(Buf)))); //!< 参考値) LKG-PORT-03996
				}
				{
					std::vector<char> Buf(hpc_GetDeviceType(i, nullptr, 0));
					hpc_GetDeviceType(i, data(Buf), size(Buf));
					LOG(data(std::format("\thpc_GetDeviceType = {}\n", data(Buf)))); //!< 参考値) portrait
				}

				//!< ウインドウ位置
				LOG(data(std::format("\thpc_GetDevicePropertyWinX, Y = {}, {}\n", hpc_GetDevicePropertyWinX(i), hpc_GetDevicePropertyWinY(i)))); //!< 参考値) 1920, 0

				//!< ウインドウ幅高さ
				LOG(data(std::format("\thpc_GetDevicePropertyScreenW, H = {}, {}\n", hpc_GetDevicePropertyScreenW(i), hpc_GetDevicePropertyScreenH(i)))); //!< 参考値) 1536, 2048

				//!< Display fringe correction uniform (currently only applicable to 15.6" Developer/Pro units)
				LOG(data(std::format("\thpc_GetDevicePropertyFringe = {}\n", hpc_GetDevicePropertyFringe(i))));//!< 参考値) 0

				//!< 基本的に libHoloPlayCore.h の内容は理解する必要はないと書いてあるが、サンプルでは使っている…
				//!< ビューの取りうる範囲 [-viewCone * 0.5f, viewCone * 0.5f]
				LOG(data(std::format("\thpc_GetDevicePropertyFloat(viewCone) = {}\n", hpc_GetDevicePropertyFloat(i, "/calibration/viewCone/value")))); //!< 参考値) 40
			}
		}

		if (0 < hpc_GetNumDevices()) {
			//!< ここでは最初のデバイスを選択 (Select 1st device here)
			DeviceIndex = 0;
			LOG(data(std::format("Selected DeviceIndex = {}\n", DeviceIndex)));

			//!< キルト
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
			//!< [ DX, VK ] 一度に描画できるビュー数
			//!<	DX : D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE 16
			//!<	VK : VkPhysicalDeviceProperties.limits.maxViewports = 16
			//!<	トータルのビュー数が 45 や 48 だったりするので、3 回くらいは描画することになる
			//!<	もしくは、歯抜けに 16 パターン分描画して間は補完する
		} else {
			LOG("[ Holo ] Device not found\n");
		}

		//!< レンチキュラー
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
		//!< ウインドウ位置、サイズを Looking Glass から取得し、反映する
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

		//!< DX では Y が上であり、(ここでは)VK も DX に合わせて Y が上にしている為
		//!< Tilt の値を正にすることで辻褄を合わせている
		LenticularBuffer->Tilt = abs(LenticularBuffer->Tilt);
	}

protected:
	int DeviceIndex = -1;

	float ViewCone = TO_RADIAN(40.0f);

	//!< ジオメトリシェーダパラメータ
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

	//!< ピクセルシェーダパラメータ
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
						//QuiltSize = 2048; //!< レンダーターゲットサイズ 2048x2048 = 512x256 x 4 x 8
					}
					else if ("8k" == TypeStr) {
						Column = 5;
						Row = 9;
						//QuiltSize = 8192; //!< レンダーターゲットサイズ 8192x8192 = 1638x910 x 5 x 9
					}
					else if ("portrait" == TypeStr) {
						Column = 8;
						Row = 6;
						//QuiltSize = 3360; //!< レンダーターゲットサイズ 3360x3360 = 420x560 x 8 x 6
					}
					else {
						Column = 5;
						Row = 9;
						//QuiltSize = 4096; //!< レンダーターゲットサイズ 4096x4096 = 819x455 x 5 x 9
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
		//!< Gi == 1 は確定 (RGB or BGR ということ)
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
