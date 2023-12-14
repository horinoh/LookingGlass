#pragma once

#include "resource.h"

#include "../DX.h"
#include "../Holo.h"

class QuiltDX : public DX, public Holo
{
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		//!< Looking Glass ウインドウのサイズを取得してから
		Holo::SetHoloWindow(hWnd, hInstance);
		//!< DX の準備を行う
		DX::OnCreate(hWnd, hInstance, Title);
	}

	virtual void Camera(const int i) 
	{
		constexpr float CameraSize = 5.0f;
		constexpr float Fov = DirectX::XMConvertToRadians(14.0f);
		const float CameraDistance = -CameraSize / tan(Fov * 0.5f);

		//!< [-0.5f * ViewCone, 0.5f * ViewCone]
		const float OffsetAngle = (i / (QuiltTotal - 1.0f) - 0.5f) * ViewCone;
		const float OffsetX = CameraDistance * tan(OffsetAngle);

		//const auto View = DirectX::XMMatrixLookAtRH(CamPos, CamTag, CamUp);
		//viewMatrix = glm::translate(currentViewMatrix, glm::vec3(OffsetX, 0.0f, CameraDistance));

		auto Projection = DirectX::XMMatrixPerspectiveFovLH(Fov, DisplayAspect, 0.1f, 100.0f);
		//Projection[2][0] += OffsetX / (cameraSize * DisplayAspect);
	}
};
