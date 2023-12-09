#include "BorderlessWin.h"

#pragma comment(lib, "dwmapi.lib")

//!< case WM_CREATE:
//!<		Win::ToggleBorderless(hWnd);
void BorderlessWin::ToggleBorderless(HWND hWnd)
{
	BOOL IsComposition = FALSE;
	::DwmIsCompositionEnabled(&IsComposition);

	//!< ボーダーレス切替え
	constexpr auto Common = WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
	constexpr auto Borderless = Common | WS_POPUP;
	constexpr auto Aero = Borderless | WS_CAPTION;
	constexpr auto Windowed = Common | WS_OVERLAPPEDWINDOW | WS_CAPTION;
	::SetWindowLongPtrW(hWnd, GWL_STYLE, IsBorderless(hWnd) ? Windowed : (IsComposition ? Aero : Borderless));

	//!< シャドウ
	if (IsComposition) {
		constexpr auto WithoutShadow = MARGINS({ .cxLeftWidth = 0, .cxRightWidth = 0, .cyTopHeight = 0, .cyBottomHeight = 0 });
		constexpr auto WithShadow = MARGINS({ .cxLeftWidth = 1, .cxRightWidth = 1, .cyTopHeight = 1, .cyBottomHeight = 1 });
		::DwmExtendFrameIntoClientArea(hWnd, IsBorderless(hWnd) ? &WithShadow : &WithoutShadow);
	}

	//!< 再描画
	::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
	::ShowWindow(hWnd, SW_SHOW);
}

//!< クライアント領域のサイズを再計算する
//!< case WM_NCCALCSIZE:
//!<	if (wParam && Win::IsBorderless(hWnd)) {
//!<		Win::AdjustBorderlessRect(hWnd, reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam)->rgrc[0]);
//!<	}
//!<	else {
//!<		return DefWindowProc(hWnd, message, wParam, lParam);
//!<	}
bool BorderlessWin::AdjustBorderlessRect(HWND hWnd, RECT& Rect)
{
	WINDOWPLACEMENT Placement;
	::GetWindowPlacement(hWnd, &Placement);
	//!< 最大化されている場合
	if (SW_MAXIMIZE == Placement.showCmd) {
		const auto Monitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONULL);
		if (nullptr != Monitor) {
			MONITORINFO MonitorInfo;
			MonitorInfo.cbSize = sizeof(MonitorInfo);
			::GetMonitorInfoW(Monitor, &MonitorInfo);
			//!< クライアント矩形が画面からはみ出さないように調整
			Rect = MonitorInfo.rcWork;
			return true;
		}
	}
	return false;
}

//!< カーソルとのヒットを検出して、ドラッグ、サイズ変更を処理する
//!< case WM_NCHITTEST:
//!<	if (Win::IsBorderless(hWnd)) {
//!<		return Win::GetBorderlessHit(hWnd, POINT({ .x = GET_X_LPARAM(lParam), .y = GET_Y_LPARAM(lParam) }), true, true);
//!<	}
LRESULT BorderlessWin::GetBorderlessHit(HWND hWnd, const POINT& Cursor, const bool IsDrag, const bool IsResize)
{
	RECT Rect;
	//!< カーソルがウインドウ内に無いとダメ
	if (::GetWindowRect(hWnd, &Rect)) {
		//!< リサイズが有効
		if (IsResize) {
			const auto Border = POINT({ .x = ::GetSystemMetrics(SM_CXFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER), .y = ::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER) });
			enum EdgeMask {
				L = 0x1,
				R = 0x2,
				T = 0x4,
				B = 0x8,
				LT = L | T,
				RT = R | T,
				LB = L | B,
				RB = R | B,
			};
			//!< カーソルがエッジ上(上下左右)にいるか判定
			const auto IsOnEdge = (Cursor.x < Rect.left + Border.x ? EdgeMask::L : 0)
				| (Cursor.x >= Rect.right - Border.x ? EdgeMask::R : 0)
				| (Cursor.y < Rect.top + Border.y ? EdgeMask::T : 0)
				| (Cursor.y >= Rect.bottom - Border.y ? EdgeMask::B : 0);
			if (IsOnEdge) {
				//!< カーソルがどのエッジにいるかでリサイズできる方向を選択
				switch (static_cast<EdgeMask>(IsOnEdge)) {
					using enum EdgeMask;
				case L: return HTLEFT;
				case R: return HTRIGHT;
				case T: return HTTOP;
				case B: return HTBOTTOM;
				case LT: return HTTOPLEFT;
				case RT: return HTTOPRIGHT;
				case LB: return HTBOTTOMLEFT;
				case RB: return HTBOTTOMRIGHT;
				}
			}
		}

		//!< ドラッグが有効かどうかにより選択
		return IsDrag ? HTCAPTION : HTCLIENT;
	}
	return HTNOWHERE;
}