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
		if (-1 != DeviceIndex) {
			ViewCone = TO_RADIAN(hpc_GetDevicePropertyFloat(DeviceIndex, "/calibration/viewCone/value"));

			std::vector<char> Buf(hpc_GetDeviceType(DeviceIndex, nullptr, 0));
			hpc_GetDeviceType(DeviceIndex, data(Buf), size(Buf));
			std::string_view TypeStr(data(Buf));
			if ("standard" == TypeStr) {
				QuiltWidth = QuiltHeight = 2048;
			}
			else if ("8k" == TypeStr) {
				QuiltWidth = QuiltHeight = 8192;
			}
			else if ("portrait" == TypeStr) {
				QuiltWidth = QuiltHeight = 3360;
			}
			else {
				QuiltWidth = QuiltHeight = 4096;
			}
		}
	}
	virtual ~Holo() {
		if (nullptr != LenticularBuffer) {
			delete LenticularBuffer;
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
	
	virtual void UpdateLenticularBuffer(const float Column, const float Row, const uint32_t Width, const uint32_t Height) {
		LenticularBuffer->Column = Column;
		LenticularBuffer->Row = Row;
		
		QuiltWidth = Width;
		QuiltHeight = Height;
		const auto ViewWidth = static_cast<float>(QuiltWidth) / LenticularBuffer->Column;
		const auto ViewHeight = static_cast<float>(QuiltHeight) / LenticularBuffer->Row;
		LenticularBuffer->QuiltAspect = ViewWidth / ViewHeight;
		LOG(data(std::format("QuiltAspect = {}\n", ViewWidth / ViewHeight)));
		LOG(data(std::format("ViewPortion = {} x {}\n", static_cast<float>(ViewWidth) * LenticularBuffer->Column / static_cast<float>(QuiltWidth), static_cast<float>(ViewHeight) * LenticularBuffer->Row / static_cast<float>(QuiltHeight))));

		//!< DX では Y が上であり、(ここでは)VK も DX に合わせて Y が上にしている為
		//!< Tilt の値を正にすることで辻褄を合わせている
		LenticularBuffer->Tilt = abs(LenticularBuffer->Tilt);
	}

	virtual uint32_t GetViewportMax() const { return 16; }
	uint32_t GetViewportDrawCount() const {
		const auto ColRow = static_cast<uint32_t>(LenticularBuffer->Column * LenticularBuffer->Row);
		const auto ViewportCount = GetViewportMax();
		return ColRow / ViewportCount + ((ColRow % ViewportCount) ? 1 : 0);
	}
	uint32_t GetViewportSetOffset(const uint32_t i) const { return GetViewportMax() * i; }
	uint32_t GetViewportSetCount(const uint32_t i) const {
		return (std::min)(static_cast<int32_t>(LenticularBuffer->Column * LenticularBuffer->Row) - GetViewportSetOffset(i), GetViewportMax());
	}

	virtual void CreateProjectionMatrix(const int i) {}
	void CreateProjectionMatrices() {
		CreateProjectionMatrix(-1);
		const auto ColRow = static_cast<int>(LenticularBuffer->Column * LenticularBuffer->Row);
		for (auto i = 0; i < ColRow; ++i) {
			CreateProjectionMatrix(i);
		}
	}
	virtual void CreateViewMatrix(const int i) {}
	void CreateViewMatrices() {
		CreateViewMatrix(-1);
		const auto ColRow = static_cast<int>(LenticularBuffer->Column * LenticularBuffer->Row);
		for (auto i = 0; i < ColRow; ++i) {
			CreateViewMatrix(i);
		}
	}
	virtual void updateViewProjectionBuffer() {}

protected:
	int DeviceIndex = -1;

	float ViewCone = TO_RADIAN(40.0f);

	uint32_t QuiltWidth = 3360;
	uint32_t QuiltHeight = 3360;

	const float CameraSize = 5.0f; //!< 焦点面の垂直半径
	const float Fov = TO_RADIAN(14.0f);
	const float CameraDistance = -CameraSize / std::tan(Fov / 2.0f); //!< 焦点面の垂直半径と、視野角からカメラ距離を算出

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
					}
					else if ("8k" == TypeStr) {
						Column = 5;
						Row = 9;
					}
					else if ("portrait" == TypeStr) {
						Column = 8;
						Row = 6;
					}
					else {
						Column = 5;
						Row = 9;
					}
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

		float QuiltAspect = Column / Row;
	};
	LENTICULAR_BUFFER* LenticularBuffer = nullptr;
};
