#include <bridge.h>

#include <GLFW/glfw3.h>

#pragma comment(lib, "glfw3dll.lib")

/*
[ Glfw を OpenGL の設定にして、LookingGlass の必要な情報だけ取得して覚えておき、一旦破棄して Vulkan 用に作り直すとかしないとダメかも ]

[ 参考値 ]
SettingsPath=C:\Users\<USERNAME>\AppData\Roaming\Looking Glass\Bridge\settings.json
BridgeInstallLocation=C:\\Program Files\\Looking Glass\\Looking Glass Bridge 2.4.10

[ キャリブレーションテンプレート参考値 ]
TemplateCount=18
	[0] 8.9" Looking Glass, Center=0.131987, Pitch=80.756, Slope=-6.66381, Width=1440, Height=2560, Dpi=491, FlipX=0, CellPatternMode=0, NumberOfCells=0
	[1] 15.6" Looking Glass, Center=0.131987, Pitch=80.756, Slope=-6.66381, Width=1440, Height=2560, Dpi=491, FlipX=0, CellPatternMode=0, NumberOfCells=0
	[2] Looking Glass Pro, Center = 0.131987, Pitch = 80.756, Slope = -6.66381, Width = 1440, Height = 2560, Dpi = 491, FlipX = 0, CellPatternMode = 0, NumberOfCells = 0
	[3] Looking Glass 8K, Center = 0.131987, Pitch = 80.756, Slope = -6.66381, Width = 1440, Height = 2560, Dpi = 491, FlipX = 0, CellPatternMode = 0, NumberOfCells = 0
	[4] Looking Glass Portrait, LKG - PORT - , Center = 0.5, Pitch = 52, Slope = -7, Width = 1536, Height = 2048, Dpi = 324, FlipX = 0, CellPatternMode = 0, NumberOfCells = 0
	[5] Looking Glass 16", LKG-A, Center=0.5, Pitch=50, Slope=-7, Width=3840, Height=2160, Dpi=283, FlipX=0, CellPatternMode=0, NumberOfCells=0
	[6] Looking Glass 32", LKG-B, Center=0.3, Pitch=42, Slope=-6, Width=7680, Height=4320, Dpi=280, FlipX=0, CellPatternMode=0, NumberOfCells=0
	[7] Third - Party Non - Looking Glass, Center = 0.3, Pitch = 42, Slope = -6, Width = 7680, Height = 4320, Dpi = 280, FlipX = 0, CellPatternMode = 0, NumberOfCells = 0
	[8] Looking Glass 65", LKG-D, Center=0.3, Pitch=42, Slope=-6, Width=7680, Height=4320, Dpi=136, FlipX=0, CellPatternMode=0, NumberOfCells=0
	[9] Looking Glass Prototype, LKG - Q, Center = 0.3, Pitch = 42, Slope = -6, Width = 7680, Height = 4320, Dpi = 136.6, FlipX = 0, CellPatternMode = 0, NumberOfCells = 0
	[10] Looking Glass Go, LKG - E, Center = -0.3, Pitch = 80.7, Slope = -6.3, Width = 1440, Height = 2560, Dpi = 491, FlipX = 0, CellPatternMode = 0, NumberOfCells = 0
	[11] Looking Glass Go, LKG - G, Center = 0.2, Pitch = 75, Slope = -7.2, Width = 2560, Height = 1440, Dpi = 491, FlipX = 0, CellPatternMode = 0, NumberOfCells = 1
		BOffsetXY = 0, 0, GOffsetXY = 0, 0, ROffsetXY = 0, 0
	[12] Looking Glass Kiosk, LKG - F, Center = 0.5, Pitch = 52, Slope = -7, Width = 1536, Height = 2048, Dpi = 324, FlipX = 0, CellPatternMode = 0, NumberOfCells = 0
	[13] Looking Glass 16" Spatial Display, LKG-H, Center=0, Pitch=44.72, Slope=-7.45, Width=2160, Height=3840, Dpi=283, FlipX=0, CellPatternMode=4, NumberOfCells=2
		BOffsetXY = 0.25, 0, GOffsetXY = -0.25, 0.28, ROffsetXY = -0.25, -0.31
		BOffsetXY = -0.25, 0, GOffsetXY = 0.25, 0.28, ROffsetXY = 0.25, -0.31
	[14] Looking Glass 16" Spatial Display, LKG-J, Center=0.5, Pitch=44.72, Slope=-6.9, Width=3840, Height=2160, Dpi=283, FlipX=0, CellPatternMode=2, NumberOfCells=2
		BOffsetXY = 0, -0.25, GOffsetXY = 0.28, 0.25, ROffsetXY = -0.31, 0.25
		BOffsetXY = 0, 0.25, GOffsetXY = 0.28, -0.25, ROffsetXY = -0.31, -0.25
	[15] Looking Glass 32" Spatial Display, LKG-K, Center=-0.3, Pitch=80.7, Slope=-6.3, Width=4320, Height=7680, Dpi=280, FlipX=0, CellPatternMode=0, NumberOfCells=1
		BOffsetXY = 0, -0.333, GOffsetXY = 0, 0, ROffsetXY = 0, 0.333
	[16] Looking Glass 32" Spatial Display, LKG-L, Center=0.2, Pitch=75, Slope=-7.2, Width=7680, Height=4320, Dpi=280, FlipX=0, CellPatternMode=0, NumberOfCells=0
	[17] Looking Glass 65" Portrait, Center=0.2, Pitch=75, Slope=-7.2, Width=7680, Height=4320, Dpi=280, FlipX=0, CellPatternMode=0, NumberOfCells=0

[ キャリブレーション参考値 (Looking Glass Go) ]
DeviceName=Looking Glass Go
Serial=LKG-E05304
Center=0.131987, Pitch=80.756, Slope=-6.66381, Width=1440, Height=2560, Dpi=491, FlipX=0, CellPatternMode=0, NumberOfCells=0
QuiltAspect=0.5625, QuiltXY=4092, 4092, TileXY=11, 6

[ キャリブレーション参考値 (Looking Glass Portrait) ]
DeviceName=Looking Glass Portrait
Serial=LKG-P03996
Center=0.565845, Pitch=52.5741, Slope=-7.19256, Width=1536, Height=2048, Dpi=324, FlipX=0, CellPatternMode=0, NumberOfCells=0
QuiltAspect=0.75, QuiltXY=3360, 3360, TileXY=8, 6

[ HoloPlay Core 参考値　(Looking Glass Portrait) ]
Center = 0.565845f
Pitch = 246.866f -> 52.5741
Tilt = -0.185377f -> ----
Subp = 0.000217014f -> ----
DisplayAspect = 0.75f
InvView = 1 -> ----
Ri = 0, Bi = 2 -> ----
QuiltX = 3360, QuiltY = 3360
TileX = 8, TileY = 6
ViewCone = 0.69813174 -> ----
*/
int main() 
{
	if (!glfwInit()) {
		std::cerr << "glfwInit() failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	Controller LKGCtrl;
	if (!LKGCtrl.Initialize()) {
		std::cerr << "Initialize failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	std::wcout << "SettingsPath=" << std::data(LKGCtrl.SettingsPath()) << std::endl;
	std::wcout << "BridgeInstallLocation=" << std::data(LKGCtrl.BridgeInstallLocation(BridgeVersion)) << std::endl;

	//!< Glfw を OpenGL の設定にする
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	auto GlfwWnd = glfwCreateWindow(1440, 2560, "Bridge SDK example", nullptr, nullptr);
	glfwMakeContextCurrent(GlfwWnd);

	//!< Glfw を OpenGL の設定で準備した上で、デバイスを接続していないと失敗する
	WINDOW_HANDLE LKGWnd;
	if (!LKGCtrl.InstanceWindowGL(&LKGWnd)) {
		std::cerr << "InstanceWindowGL failed" << std::endl;
	}

	//ID3D12Device* Device;
	//if (!LKGCtrl.InstanceWindowDX(Device, &LKGWnd)) {
	//	std::cerr << "InstanceWindowDX failed" << std::endl;
	//}
	
	//WINDOW_HANDLE LKGOffWnd;
	//if (!LKGCtrl.InstanceOffscreenWindow(&LKGOffWnd, 1280, 720, L"")) {
	//	std::cerr << "InstanceOffscreenWindow failed" << std::endl;
	//}
	
	auto StrCount = 0;
	//!< デバイス名
	if (LKGCtrl.GetDeviceName(LKGWnd, &StrCount, nullptr)) {
		std::wstring DevName;
		DevName.resize(StrCount + 1);
		LKGCtrl.GetDeviceName(LKGWnd, &StrCount, std::data(DevName));
		std::wcout << "DeviceName=" << std::data(DevName) << std::endl;
	}
	//!< シリアル
	if (LKGCtrl.GetDeviceSerial(LKGWnd, &StrCount, nullptr)) {
		std::wstring Serial;
		Serial.resize(StrCount + 1);
		LKGCtrl.GetDeviceSerial(LKGWnd, &StrCount, std::data(Serial));
	}
	//!< キャリブレーション
	auto Center = 0.0f, Pitch = 0.0f, Slope = 0.0f;
	auto Width = 0, Height = 0;
	auto Dpi = 0.0f, FlipX = 0.0f;
	auto CellPatternMode = 0, NumberOfCells = 0;
	if (LKGCtrl.GetCalibration(LKGWnd, &Center, &Pitch, &Slope, &Width, &Height, &Dpi, &FlipX, &CellPatternMode, &NumberOfCells, nullptr)) {
		std::cout << "Center=" << Center << ", Pitch=" << Pitch << ", Slope=" << Slope << ", Width=" << Width << ", Height=" << Height << ", Dpi=" << Dpi << ", FlipX=" << FlipX << ", CellPatternMode=" << CellPatternMode << ", NumberOfCells=" << NumberOfCells;
		std::vector<CalibrationSubpixelCell> CSCs;
		if (NumberOfCells) {
			CSCs.resize(NumberOfCells);
			LKGCtrl.GetCalibration(LKGWnd, &Center, &Pitch, &Slope, &Width, &Height, &Dpi, &FlipX, &CellPatternMode, &NumberOfCells, std::data(CSCs));
			std::cout << std::endl;
			for (auto j : CSCs) {
				std::cout << "\tBOffsetXY=" << j.BOffsetX << ", " << j.BOffsetY << ", GOffsetXY=" << j.GOffsetX << ", " << j.GOffsetY << ", ROffsetXY=" << j.ROffsetX << ", " << j.ROffsetY << std::endl;
			}
		}
		else {
			std::cout << std::endl;
		}
	}
	//!< キルト設定
	auto QuiltAspect = 0.0f;
	auto QuiltX = 0, QuiltY = 0, TileX = 0, TileY = 0;
	if (LKGCtrl.GetDefaultQuiltSettings(LKGWnd, &QuiltAspect, &QuiltX, &QuiltY, &TileX, &TileY)) {
		std::cout << "QuiltAspect=" << QuiltAspect << ", QuiltXY=" << QuiltX << ", " << QuiltY << ", TileXY=" << TileX << ", " << TileY << std::endl;
	}

	{
		int WinLeft, WinTop, WinRight, WinBottom;
		glfwGetWindowFrameSize(GlfwWnd, &WinLeft, &WinTop, &WinRight, &WinBottom);
		std::cout << "Window frame size (LTRB) = " << WinLeft << ", " << WinTop << ", " << WinRight << " ," << WinBottom << std::endl;

		int WinX, WinY;
		glfwGetWindowPos(GlfwWnd, &WinX, &WinY);
		std::cout << "Window pos = " << WinX << ", " << WinY << std::endl;
		int WinWidth, WinHeight;
		glfwGetWindowSize(GlfwWnd, &WinWidth, &WinHeight);
		std::cout << "Window size = " << WinWidth << "x" << WinHeight << std::endl;

		int FBWidth, GBHeight;
		glfwGetFramebufferSize(GlfwWnd, &FBWidth, &GBHeight);
	}

	//!< ------------------------------------
	//!< テンプレート
	int TmplCount = 0;
	if (LKGCtrl.GetCalibrationTemplateCount(&TmplCount)) {
		std::cout << "TemplateCount=" << TmplCount << std::endl;
		for (auto i = 0; i < TmplCount; ++i) {
			std::cout << "\t[" << i << "]";
	
			auto StrCount = 0;
			//!< デバイス名
			if (LKGCtrl.GetCalibrationTemplateDeviceName(i, &StrCount, nullptr)) {
				std::wstring DevName;
				DevName.resize(StrCount + 1);
				LKGCtrl.GetCalibrationTemplateDeviceName(i, &StrCount, std::data(DevName));
				std::wcout << " " << std::data(DevName);
			}
			//!< シリアル
			if (LKGCtrl.GetCalibrationTemplateSerial(i, &StrCount, nullptr)) {
				std::wstring Serial;
				Serial.resize(StrCount + 1);
				if (!LKGCtrl.GetCalibrationTemplateSerial(i, &StrCount, std::data(Serial))) {}
				std::wcout << ", " << std::data(Serial);
			}
			//!< キャリブレーション
			LKGCtrl.GetCalibrationTemplate(i, &Center, &Pitch, &Slope, &Width, &Height, &Dpi, &FlipX, &CellPatternMode, &NumberOfCells, nullptr);
			std::cout << ", Center=" << Center << ", Pitch=" << Pitch << ", Slope=" << Slope << ", Width=" << Width << ", Height=" << Height << ", Dpi=" << Dpi << ", FlipX=" << FlipX << ", CellPatternMode=" << CellPatternMode << ", NumberOfCells=" << NumberOfCells;
			std::vector<CalibrationSubpixelCell> CSCs;
			if (NumberOfCells) {
				CSCs.resize(NumberOfCells);
				LKGCtrl.GetCalibrationTemplate(i, &Center, &Pitch, &Slope, &Width, &Height, &Dpi, &FlipX, &CellPatternMode, &NumberOfCells, std::data(CSCs));
				std::cout << std::endl;
				for (auto j : CSCs) {
					std::cout << "\t\tBOffsetXY=" << j.BOffsetX << ", " << j.BOffsetY << ", GOffsetXY=" << j.GOffsetX << ", " << j.GOffsetY << ", ROffsetXY=" << j.ROffsetX << ", " << j.ROffsetY << std::endl;
				}
			}
			else {
				std::cout << std::endl;
			}
		}
	}

	LKGCtrl.ShowWindow(LKGWnd, true);

	LKGCtrl.Uninitialize();

	glfwDestroyWindow(GlfwWnd);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}