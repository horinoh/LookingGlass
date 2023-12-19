#pragma once

#include <Windows.h>

#include <vector>
#include <array>
#include <cassert>
#include <iterator>
#include <numeric>
#include <bitset>
#include <algorithm>

#include <d3d12.h>
#include <DXGI1_6.h>
#include <DirectXColors.h>

#include <winrt/base.h>
#define COM_PTR winrt::com_ptr
#define COM_PTR_GET(_x) _x.get()
#define COM_PTR_PUT(_x) _x.put()
#define COM_PTR_PUTVOID(_x) _x.put_void()
#define COM_PTR_UUIDOF_PUTVOID(_x) __uuidof(_x), COM_PTR_PUTVOID(_x)
#define COM_PTR_RESET(_x) _x = nullptr
#define COM_PTR_AS(_x, _y) winrt::copy_to_abi(_x, *_y.put_void());
#define COM_PTR_COPY(_x, _y) _x.copy_from(COM_PTR_GET(_y))

#define VERIFY_SUCCEEDED(x) (x)

#ifdef _DEBUG
#define LOG(x) OutputDebugStringA((x))
#else
#define LOG(x)
#endif

class DX
{
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) {
		GetClientRect(hWnd, &Rect);
		const auto W = Rect.right - Rect.left, H = Rect.bottom - Rect.top;
		LOG(data(std::format("Rect = {} x {}\n", W, H)));

		CreateDevice(hWnd);
		CreateCommandQueue();
		CreateFence();
		CreateSwapChain(hWnd, DXGI_FORMAT_R8G8B8A8_UNORM);
		CreateCommandList();
		CreateGeometry();
		CreateConstantBuffer();
		CreateTexture();
		CreateStaticSampler();
		CreateRootSignature();
		CreatePipelineState();
		CreateDescriptor();
		CreateShaderTable();
		
		OnExitSizeMove(hWnd, hInstance);
	}
	virtual void OnExitSizeMove(HWND hWnd, HINSTANCE hInstance) {
#define TIMER_ID 1000 //!< 何でも良い
		KillTimer(hWnd, TIMER_ID);
		{
			GetClientRect(hWnd, &Rect);

			//SwapChain->ResizeBuffers() でリサイズ
			//その他のリソースはほぼ作り直し
			LOG("OnExitSizeMove\n");

			const auto W = Rect.right - Rect.left, H = Rect.bottom - Rect.top;
			CreateViewport(static_cast<const FLOAT>(W), static_cast<const FLOAT>(H));

			for (auto i = 0; i < size(DirectCommandLists); ++i) {
				PopulateCommandList(i);
			}
		}
		SetTimer(hWnd, TIMER_ID, 1000 / 60, nullptr);
	}
	virtual void OnTimer(HWND hWnd, HINSTANCE hInstance) { SendMessage(hWnd, WM_PAINT, 0, 0); }
	virtual void OnPaint(HWND hWnd, HINSTANCE hInstance)  { Draw(); }
	virtual void OnDestroy(HWND hWnd, HINSTANCE hInstance)  { }

public:
	virtual void CreateDevice([[maybe_unused]] HWND hWnd);
	virtual void CreateCommandQueue();
	virtual void CreateFence();
	
	virtual void CreateSwapChain(HWND hWnd, const DXGI_FORMAT ColorFormat, const UINT Width, const UINT Height);
	virtual void GetSwapChainResource();
	virtual void CreateSwapChain(HWND hWnd, const DXGI_FORMAT ColorFormat) {
		CreateSwapChain(hWnd, ColorFormat, Rect.right - Rect.left, Rect.bottom - Rect.top);
		GetSwapChainResource();
	}
	//virtual void ResizeSwapChain(const UINT Width, const UINT Height);

	virtual void CreateDirectCommandList();
	virtual void CreateBundleCommandList();
	virtual void CreateCommandList() {
		CreateDirectCommandList();
		CreateBundleCommandList();
	}

	virtual void CreateGeometry() {}
	virtual void CreateConstantBuffer() {}
	virtual void CreateTexture() {}
	virtual void CreateStaticSampler() {}
	virtual void CreateRootSignature() {}
	virtual void CreatePipelineState() {}
	virtual void CreateDescriptor() {}
	virtual void CreateShaderTable() {}

	virtual void CreateViewport(const FLOAT Width, const FLOAT Height, const FLOAT MinDepth = 0.0f, const FLOAT MaxDepth = 1.0f);
	static void ResourceBarrier(ID3D12GraphicsCommandList* GCL, ID3D12Resource* Resource, const D3D12_RESOURCE_STATES Before, const D3D12_RESOURCE_STATES After) {
		const std::array RBs = {
			D3D12_RESOURCE_BARRIER({
				.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
				.Transition = D3D12_RESOURCE_TRANSITION_BARRIER({
					.pResource = Resource,
					.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					.StateBefore = Before, .StateAfter = After
				})
			})
		};
		GCL->ResourceBarrier(static_cast<UINT>(size(RBs)), data(RBs));
	}
	virtual void PopulateCommandList(const size_t i) {
		const auto CL = COM_PTR_GET(DirectCommandLists[i]);
		const auto CA = COM_PTR_GET(DirectCommandAllocators[0]);
		VERIFY_SUCCEEDED(CL->Reset(CA, nullptr)); {
			CL->RSSetViewports(static_cast<UINT>(size(Viewports)), data(Viewports));
			CL->RSSetScissorRects(static_cast<UINT>(size(ScissorRects)), data(ScissorRects));
			const auto SCR = COM_PTR_GET(SwapChainResources[i]);
			ResourceBarrier(CL, SCR, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			{
				constexpr std::array<D3D12_RECT, 0> Rects = {};
				CL->ClearRenderTargetView(SwapChainCPUHandles[i], DirectX::Colors::SkyBlue, static_cast<UINT>(size(Rects)), data(Rects));
			}
			ResourceBarrier(CL, SCR, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		} VERIFY_SUCCEEDED(CL->Close());
	}

	static void WaitForFence(ID3D12CommandQueue* CQ, ID3D12Fence* Fence);
	virtual void DrawFrame([[maybe_unused]] const UINT i) {}
	virtual void SubmitGraphics(const UINT i);
	virtual void Present();
	virtual void Draw();

	
protected:
	RECT Rect;

	COM_PTR<IDXGIFactory4> Factory;
	COM_PTR<IDXGIAdapter> Adapter;
	COM_PTR<IDXGIOutput> Output;
	COM_PTR<ID3D12Device> Device;

	COM_PTR<ID3D12CommandQueue> GraphicsCommandQueue;
	
	COM_PTR<ID3D12Fence> GraphicsFence;

	COM_PTR<IDXGISwapChain4> SwapChain;
	COM_PTR<ID3D12DescriptorHeap> SwapChainDescriptorHeap;				
	std::vector<COM_PTR<ID3D12Resource>> SwapChainResources;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> SwapChainCPUHandles;

	std::vector<COM_PTR<ID3D12CommandAllocator>> DirectCommandAllocators;
	std::vector<COM_PTR<ID3D12GraphicsCommandList>> DirectCommandLists;
	std::vector<COM_PTR<ID3D12CommandAllocator>> BundleCommandAllocators;
	std::vector<COM_PTR<ID3D12GraphicsCommandList>> BundleCommandLists;

	std::vector<D3D12_VIEWPORT> Viewports;
	std::vector<D3D12_RECT> ScissorRects;
};
