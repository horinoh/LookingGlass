#pragma once

#include <dwmapi.h>
#include <windowsx.h>

class BorderlessWin
{
public:
	static bool IsBorderless(HWND hWnd) { return ::GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_POPUP; }
	static void ToggleBorderless(HWND hWnd);
	static bool AdjustBorderlessRect(HWND hWnd, RECT& Rect);
	static LRESULT GetBorderlessHit(HWND hWnd, const POINT& Cursor, const bool IsDrag = false, const bool IsResize = false);
};