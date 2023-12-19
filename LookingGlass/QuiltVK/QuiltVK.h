#pragma once

#include "resource.h"

#include "../VK.h"
#include "../Holo.h"

class QuiltVK : public VK, public Holo
{
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		//!< Looking Glass �E�C���h�E�̃T�C�Y���擾���Ă���
		Holo::SetHoloWindow(hWnd, hInstance);
		//!< VK �̏������s��
		VK::OnCreate(hWnd, hInstance, Title);
	}

	virtual void Camera(const int i)
	{
	}
};
