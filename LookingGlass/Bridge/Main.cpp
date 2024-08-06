#include <bridge.h>

#include <GLFW/glfw3.h>

#pragma comment(lib, "glfw3dll.lib")

/*
[ Glfw �� OpenGL �̐ݒ�ɂ��āALookingGlass �̕K�v�ȏ�񂾂��擾���Ċo���Ă����A��U�j������ Vulkan �p�ɍ�蒼���Ƃ����Ȃ��ƃ_������ ]
 
[ Looking Glass Portrait �̏ꍇ�̎Q�l�o�� ]
DeviceName=Looking Glass Portrait
Serial=LKG-P03996
Center=0.565845, Pitch=52.5741, Slope=-7.19256, Width=1536, Height=2048, Dpi=324, FlipX=0, CellPatternMode=0, NumberOfCells=0
QuiltAspect=0.75, QuiltXY=3360, 3360, TileXY=8, 6

[ Core SDK �Ƃ̈Ⴂ ]
Pitch �̒l���Ⴄ
	52.5741 vs 246.86568
Tilt ������
	0.18537661 vs ----
Subp ������
	0.00021701389 vs ----
InvView ������
	-1 vs ----
Ri, Bi ������
	0, 2 vs ----
ViewCone ������
	0.69813174 vs ----
WinPos ������
	1920, 0 vs ----
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

	//!< Glfw �� OpenGL �̐ݒ�ɂ���
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	auto GlfwWnd = glfwCreateWindow(800, 800, "Bridge SDK example", nullptr, nullptr);
	glfwMakeContextCurrent(GlfwWnd);

	//!< Glfw �� OpenGL �̐ݒ�ŏ���������ŁA�f�o�C�X��ڑ����Ă��Ȃ��Ǝ��s����
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
	//!< �f�o�C�X��
	if (LKGCtrl.GetDeviceName(LKGWnd, &StrCount, nullptr)) {
		std::wstring DevName;
		DevName.resize(StrCount + 1);
		LKGCtrl.GetDeviceName(LKGWnd, &StrCount, std::data(DevName));
		std::wcout << "DeviceName=" << std::data(DevName) << std::endl;
	}
	//!< �V���A��
	if (LKGCtrl.GetDeviceSerial(LKGWnd, &StrCount, nullptr)) {
		std::wstring Serial;
		Serial.resize(StrCount + 1);
		LKGCtrl.GetDeviceSerial(LKGWnd, &StrCount, std::data(Serial));
		std::wcout << "Serial=" << std::data(Serial) << std::endl;
	}
	//!< �L�����u���[�V����
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
	//!< �L���g�ݒ�
	auto QuiltAspect = 0.0f;
	auto QuiltX = 0, QuiltY = 0, TileX = 0, TileY = 0;
	if (LKGCtrl.GetDefaultQuiltSettings(LKGWnd, &QuiltAspect, &QuiltX, &QuiltY, &TileX, &TileY)) {
		std::cout << "QuiltAspect=" << QuiltAspect << ", QuiltXY=" << QuiltX << ", " << QuiltY << ", TileXY=" << TileX << ", " << TileY << std::endl;
	}

	//!< ------------------------------------
	//!< �e���v���[�g
	int TmplCount = 0;
	if (LKGCtrl.GetCalibrationTemplateCount(&TmplCount)) {
		std::cout << "TemplateCount=" << TmplCount << std::endl;
		for (auto i = 0; i < TmplCount; ++i) {
			std::cout << "\t[" << i << "]";
	
			auto StrCount = 0;
			//!< �f�o�C�X��
			if (LKGCtrl.GetCalibrationTemplateDeviceName(i, &StrCount, nullptr)) {
				std::wstring DevName;
				DevName.resize(StrCount + 1);
				LKGCtrl.GetCalibrationTemplateDeviceName(i, &StrCount, std::data(DevName));
				std::wcout << " " << std::data(DevName);
			}
			//!< �V���A��
			if (LKGCtrl.GetCalibrationTemplateSerial(i, &StrCount, nullptr)) {
				std::wstring Serial;
				Serial.resize(StrCount + 1);
				if (!LKGCtrl.GetCalibrationTemplateSerial(i, &StrCount, std::data(Serial))) {}
				std::wcout << ", " << std::data(Serial);
			}
			//!< �L�����u���[�V����
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