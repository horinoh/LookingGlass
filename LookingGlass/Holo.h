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
				LOG(data(std::format("\thpc_GetDevicePropertyFloat = {}\n", hpc_GetDevicePropertyFloat(i, "/calibration/viewCone/value")))); //!< 参考値) 40
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
					QuiltColumn = 4;
					QuiltRow = 8;
					//!< 2048x2048
				}
				else if ("8k" == TypeStr) {
					QuiltColumn = 5;
					QuiltRow = 9;
					//!< 8192x8192
				}
				else if ("portrait" == TypeStr) {
					QuiltColumn = 8;
					QuiltRow = 6;
					//!< 3360x3360
				}
				else {
					QuiltColumn = 5;
					QuiltRow = 9;
					//!< 4096x4096
				}
				//!< QuiltTotalは hpc_GetDevicePropertyInvView(DeviceIndex) ? 1 : -1 を乗算して、符号込みにしてからシェーダに渡す #TODO
				QuiltTotal = QuiltColumn * QuiltRow;

				LOG(data(std::format("Type = {}, Quilt = {} x {} = {}\n", TypeStr, QuiltColumn, QuiltRow, QuiltTotal)));
			}

			//!< レンチキュラー
			{
				Pitch = hpc_GetDevicePropertyPitch(DeviceIndex);
				Tilt = hpc_GetDevicePropertyTilt(DeviceIndex);
				Center = hpc_GetDevicePropertyCenter(DeviceIndex);
				Subp = hpc_GetDevicePropertySubp(DeviceIndex);
				DisplayAspect = hpc_GetDevicePropertyDisplayAspect(DeviceIndex);
				//hpc_GetDevicePropertyInvView(DeviceIndex);
				Ri = hpc_GetDevicePropertyRi(DeviceIndex);
				Bi = hpc_GetDevicePropertyBi(DeviceIndex);
			
				LOG(data(std::format("Pitch = {}\n", Pitch)));
				LOG(data(std::format("Tilt = {}\n", Tilt)));
				LOG(data(std::format("Center = {}\n", Center)));
				LOG(data(std::format("Subp = {}\n", Subp)));
				LOG(data(std::format("DisplayAspect = {}\n", DisplayAspect)));
				LOG(data(std::format("Ri, Bi = {}, {} ({})\n", Ri, Bi, (Ri == 0 && Bi == 2) ? "RGB" : "BGR")));
			}
			{
				ViewCone = TO_RADIAN(hpc_GetDevicePropertyFloat(DeviceIndex, "/calibration/viewCone/value"));
			}
			//!< [ DX, VK ] 一度に描画できるビュー数
			//!<	DX : D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE 16
			//!<	VK : VkPhysicalDeviceProperties.limits.maxViewports = 16
			//!<	トータルのビュー数が 45 や 48 だったりするので、3 回くらいは描画することになる
		} else {
			LOG("[ Holo ] Device not found\n");
		}
	}
	virtual ~Holo() {
		hpc_CloseApp();
	}

	void SetHoloWindow(HWND hWnd, HINSTANCE hInstance)  
	{
		//!< ウインドウ位置、サイズを Looking Glass から取得し、反映する
		const auto Index = GetDeviceIndex();
		if (-1 != Index) {
			::SetWindowPos(hWnd, nullptr, hpc_GetDevicePropertyWinX(Index), hpc_GetDevicePropertyWinY(Index), hpc_GetDevicePropertyScreenW(Index), hpc_GetDevicePropertyScreenH(Index), SWP_FRAMECHANGED);
			::ShowWindow(hWnd, SW_SHOW);
		}
	}

	int GetDeviceIndex() const { return DeviceIndex; }
	//virtual int GetViewportMax() const { return 1; }
	//struct QUILT_SETTING {
	//	QUILT_SETTING() {}
	//	QUILT_SETTING(const int Index) {
	//		std::vector<char> Buf(hpc_GetDeviceType(Index, nullptr, 0));
	//		hpc_GetDeviceType(Index, data(Buf), size(Buf));
	//		std::string_view TypeStr(data(Buf));

	//		if ("standard" == TypeStr) {
	//			Size = 2048; Column = 4; Row = 8;
	//		}
	//		else if ("8k" == TypeStr) {
	//			//Looking Glass 8K : 5 columns by 9 rows, 8192 by 8192
	//			Size = 8192; Column = 5; Row = 9;
	//		}
	//		else if ("portrait" == TypeStr) {
	//			//Looking Glass Portrait : 8 columns by 6 rows, 3360 by 3360
	//			Size = 3360; Column = 8; Row = 6;
	//		}
	//		else {
	//			//Looking Glass 15.6": 5 columns by 9 rows, 4096 by 4096
	//			Size = 4096; Column = 5; Row = 9;
	//		}

	//		LOG(data(std::format("Type = {}\n", TypeStr)));
	//		LOG(data(std::format("\tQuiltSettings =\n")));
	//		LOG(data(std::format("\t\tWidth, Height = {}, {}\n", GetWidth(), GetHeight())));
	//		LOG(data(std::format("\t\tColumn, Row = {}, {}\n", GetViewColumn(), GetViewRow())));
	//		//OUTPUT(data(std::format("\t\tViewTotal = {}\n", GetViewTotal())));
	//	}

	//	int GetSize() const { return Size; }
	//	int GetWidth() const { return GetSize(); }
	//	int GetHeight() const { return GetSize(); }
	//	int GetViewColumn() const { return Column; }
	//	int GetViewRow() const { return Row; }
	//	int GetViewWidth() const { return GetWidth() / GetViewColumn(); }
	//	int GetViewHeight() const { return GetHeight() / GetViewRow(); }
	//	int GetViewTotal() const { return GetViewColumn() * GetViewRow(); }

	//	int Size = 3360;
	//	int Column = 8, Row = 6;
	//};
	//const QUILT_SETTING& GetQuiltSetting() const { return QuiltSetting; }

protected:
	int DeviceIndex = -1;

	int QuiltColumn = 5;
	int QuiltRow = 9;
	int QuiltTotal = QuiltColumn * QuiltRow;

	float Pitch = 246.866f;
	float Tilt = -0.185377f;
	float Center = 0.565845f;
	float Subp = 0.000217014f;
	float DisplayAspect = 0.75f;
	int Ri = 0, Bi = 2;

	float ViewCone = TO_RADIAN(40.0f);

	// 
	//QUILT_SETTING QuiltSetting;

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
	//struct HOLO_BUFFER_PS {
	//	HOLO_BUFFER_PS() {}
	//	HOLO_BUFFER_PS(const int Index) {
	//		Pitch = hpc_GetDevicePropertyPitch(Index);
	//		Tilt = hpc_GetDevicePropertyTilt(Index);
	//		Center = hpc_GetDevicePropertyCenter(Index);
	//		Subp = hpc_GetDevicePropertySubp(Index);
	//		//!<  W / H
	//		DisplayAspect = hpc_GetDevicePropertyDisplayAspect(Index);
	//		InvView = hpc_GetDevicePropertyInvView(Index);
	//		//!< 恐らく G == 1 は確定で、RGB or BGR ということ
	//		Ri = hpc_GetDevicePropertyRi(Index);
	//		Bi = hpc_GetDevicePropertyBi(Index);

	//		LOG(data(std::format("\thpc_GetDevicePropertyPitch = {}\n", Pitch)));
	//		LOG(data(std::format("\thpc_GetDevicePropertyTilt = {}\n", Tilt)));
	//		LOG(data(std::format("\thpc_GetDevicePropertyCenter = {}\n", Center)));
	//		LOG(data(std::format("\thpc_GetDevicePropertySubp = {}\n", Subp)));
	//		LOG(data(std::format("\thpc_GetDevicePropertyDisplayAspect = {}\n", DisplayAspect)));
	//		LOG(data(std::format("\thpc_GetDevicePropertyInvView = {}\n", InvView)));
	//		LOG(data(std::format("\thpc_GetDevicePropertyRi, Bi = {}, {} ({})\n", Ri, Bi, (Ri == 0 && Bi == 2) ? "RGB" : "BGR")));
	//	}
	//	float Pitch = 246.866f;
	//	float Tilt = -0.185377f;
	//	float Center = 0.565845f;
	//	float Subp = 0.000217014f;
	//	float DisplayAspect = 0.75f;
	//	int InvView = 1;
	//	int Ri = 0, Bi = 2;

	//	float QuiltColumn;
	//	float QuiltRow;
	//	float QuiltTotal;
	//};
	//HOLO_BUFFER_PS BufferPS;
};
