#pragma once

#include "resource.h"

#include "../VK.h"
#include "../Holo.h"

class QuiltVK : public VK, public Holo
{
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		//!< Looking Glass ウインドウのサイズを取得してから
		Holo::SetHoloWindow(hWnd, hInstance);
		//!< VK の準備を行う
		VK::OnCreate(hWnd, hInstance, Title);
	}

	virtual void Camera(const int i)
	{
	}
};
