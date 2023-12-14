#include "DX.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

void DX::CreateDevice([[maybe_unused]] HWND hWnd)
{
	VERIFY_SUCCEEDED(CreateDXGIFactory1(COM_PTR_UUIDOF_PUTVOID(Factory)));

	//!< アダプター(GPU)の選択
	std::vector<DXGI_ADAPTER_DESC> ADs;
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters(i, COM_PTR_PUT(Adapter)); ++i) {
		VERIFY_SUCCEEDED(Adapter->GetDesc(&ADs.emplace_back()));
		COM_PTR_RESET(Adapter);
	}
	const auto Index = static_cast<UINT>(std::distance(begin(ADs), std::ranges::max_element(ADs, [](const DXGI_ADAPTER_DESC& lhs, const DXGI_ADAPTER_DESC& rhs) { return lhs.DedicatedSystemMemory > rhs.DedicatedSystemMemory; })));
	VERIFY_SUCCEEDED(Factory->EnumAdapters(Index, COM_PTR_PUT(Adapter)));

	//!< アウトプット(Moniter)の選択
	COM_PTR<IDXGIAdapter> DA;
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters(i, COM_PTR_PUT(DA)); ++i) {
		for (UINT j = 0; DXGI_ERROR_NOT_FOUND != DA->EnumOutputs(j, COM_PTR_PUT(Output)); ++j) {
			if (nullptr != Output) { break; }
			COM_PTR_RESET(Output);
		}
		COM_PTR_RESET(DA);
		if (nullptr != Output) { break; }
	}

	//!< 高フィーチャーレベル優先でデバイスを作成
	constexpr std::array FeatureLevels = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};
	for (const auto i : FeatureLevels) {
		if (SUCCEEDED(D3D12CreateDevice(COM_PTR_GET(Adapter), i, COM_PTR_UUIDOF_PUTVOID(Device)))) {
			//!< NumFeatureLevels, pFeatureLevelsRequested は入力、MaxSupportedFeatureLevel は出力となる (NumFeatureLevels, pFeatureLevelsRequested is input, MaxSupportedFeatureLevel is output)
			D3D12_FEATURE_DATA_FEATURE_LEVELS FDFL = { .NumFeatureLevels = static_cast<UINT>(size(FeatureLevels)), .pFeatureLevelsRequested = data(FeatureLevels) };
			VERIFY_SUCCEEDED(Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, reinterpret_cast<void*>(&FDFL), sizeof(FDFL)));
#define D3D_FEATURE_LEVEL_ENTRY(fl) case D3D_FEATURE_LEVEL_##fl: break;
			switch (FDFL.MaxSupportedFeatureLevel) {
			default: assert(0 && "Unknown FeatureLevel"); break;
				D3D_FEATURE_LEVEL_ENTRY(12_2)
				D3D_FEATURE_LEVEL_ENTRY(12_1)
				D3D_FEATURE_LEVEL_ENTRY(12_0)
				D3D_FEATURE_LEVEL_ENTRY(11_1)
				D3D_FEATURE_LEVEL_ENTRY(11_0)
				D3D_FEATURE_LEVEL_ENTRY(10_1)
				D3D_FEATURE_LEVEL_ENTRY(10_0)
				D3D_FEATURE_LEVEL_ENTRY(9_3)
				D3D_FEATURE_LEVEL_ENTRY(9_2)
				D3D_FEATURE_LEVEL_ENTRY(9_1)
			}
#undef D3D_FEATURE_LEVEL_ENTRY
			break;
		}
	}
}
void DX::CreateCommandQueue()
{
	constexpr D3D12_COMMAND_QUEUE_DESC CQD = {
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
		.NodeMask = 0
	};
	VERIFY_SUCCEEDED(Device->CreateCommandQueue(&CQD, COM_PTR_UUIDOF_PUTVOID(GraphicsCommandQueue)));
}
void DX::CreateFence()
{
	VERIFY_SUCCEEDED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, COM_PTR_UUIDOF_PUTVOID(GraphicsFence)));
}
void DX::CreateSwapChain(HWND hWnd, const DXGI_FORMAT ColorFormat, const UINT Width, const UINT Height)
{
	std::vector<DXGI_SAMPLE_DESC> SDs;
	for (UINT i = 1; i <= D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT; ++i) {
		auto FDMSQL = D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS({
			.Format = ColorFormat,
			.SampleCount = i,
			.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,
			.NumQualityLevels = 0
			});
		VERIFY_SUCCEEDED(Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, reinterpret_cast<void*>(&FDMSQL), sizeof(FDMSQL)));
		if (FDMSQL.NumQualityLevels) {
			SDs.emplace_back(DXGI_SAMPLE_DESC({ .Count = FDMSQL.SampleCount, .Quality = FDMSQL.NumQualityLevels - 1 }));
		}
	}

	constexpr UINT BufferCount = 3;

	auto SCD = DXGI_SWAP_CHAIN_DESC({
		.BufferDesc = DXGI_MODE_DESC({
			.Width = Width, .Height = Height,
			.RefreshRate = DXGI_RATIONAL({.Numerator = 60, .Denominator = 1 }),
			.Format = ColorFormat,
			.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
			.Scaling = DXGI_MODE_SCALING_UNSPECIFIED
		}),
		.SampleDesc = SDs[0],
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = BufferCount,
		.OutputWindow = hWnd,
		.Windowed = TRUE,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	});
	COM_PTR_RESET(SwapChain);
	COM_PTR<IDXGISwapChain> NewSwapChain;
	VERIFY_SUCCEEDED(Factory->CreateSwapChain(COM_PTR_GET(GraphicsCommandQueue), &SCD, COM_PTR_PUT(NewSwapChain)));
	COM_PTR_AS(NewSwapChain, SwapChain);

	const auto DHD = D3D12_DESCRIPTOR_HEAP_DESC({
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = SCD.BufferCount,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
	});
	VERIFY_SUCCEEDED(Device->CreateDescriptorHeap(&DHD, COM_PTR_UUIDOF_PUTVOID(SwapChainDescriptorHeap)));
}
void DX::GetSwapChainResource()
{
	DXGI_SWAP_CHAIN_DESC1 SCD;
	SwapChain->GetDesc1(&SCD);

	auto CDH = SwapChainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	const auto IncSize = Device->GetDescriptorHandleIncrementSize(SwapChainDescriptorHeap->GetDesc().Type);
	for (UINT i = 0; i < SCD.BufferCount; ++i) {
		VERIFY_SUCCEEDED(SwapChain->GetBuffer(i, COM_PTR_UUIDOF_PUTVOID(SwapChainResources.emplace_back())));
		Device->CreateRenderTargetView(COM_PTR_GET(SwapChainResources.back()), nullptr, CDH);
		SwapChainCPUHandles.emplace_back(CDH);

		CDH.ptr += IncSize;
	}
}
void DX::CreateSwapchain(HWND hWnd, const DXGI_FORMAT ColorFormat)
{
	CreateSwapChain(hWnd, ColorFormat, Rect.right - Rect.left, Rect.bottom - Rect.top);

	GetSwapChainResource();
}
void DX::CreateDirectCommandList()
{
	VERIFY_SUCCEEDED(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, COM_PTR_UUIDOF_PUTVOID(DirectCommandAllocators.emplace_back())));

	DXGI_SWAP_CHAIN_DESC1 SCD;
	SwapChain->GetDesc1(&SCD);
	for (UINT i = 0; i < SCD.BufferCount; ++i) {
		VERIFY_SUCCEEDED(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, COM_PTR_GET(DirectCommandAllocators[0]), nullptr, COM_PTR_UUIDOF_PUTVOID(DirectCommandLists.emplace_back())));
		VERIFY_SUCCEEDED(DirectCommandLists.back()->Close());
	}
}
void DX::CreateBundleCommandList()
{
	VERIFY_SUCCEEDED(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, COM_PTR_UUIDOF_PUTVOID(BundleCommandAllocators.emplace_back())));

	DXGI_SWAP_CHAIN_DESC1 SCD;
	SwapChain->GetDesc1(&SCD);
	for (UINT i = 0; i < SCD.BufferCount; ++i) {
		VERIFY_SUCCEEDED(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, COM_PTR_GET(BundleCommandAllocators[0]), nullptr, COM_PTR_UUIDOF_PUTVOID(BundleCommandLists.emplace_back())));
		VERIFY_SUCCEEDED(BundleCommandLists.back()->Close());
	}
}

void DX::CreateViewport(const FLOAT Width, const FLOAT Height, const FLOAT MinDepth, const FLOAT MaxDepth)
{
	//!< DirectX、OpenGLは「左下」が原点 (Vulkan はデフォルトで「左上」が原点)
	Viewports = {
		D3D12_VIEWPORT({.TopLeftX = 0.0f, .TopLeftY = 0.0f, .Width = Width, .Height = Height, .MinDepth = MinDepth, .MaxDepth = MaxDepth }),
	};
	//!< left, top, right, bottomで指定 (offset, extentで指定のVKとは異なるので注意)
	ScissorRects = {
		D3D12_RECT({.left = 0, .top = 0, .right = static_cast<LONG>(Width), .bottom = static_cast<LONG>(Height) }),
	};
}

void DX::WaitForFence(ID3D12CommandQueue* CQ, ID3D12Fence* Fence)
{
	auto Value = Fence->GetCompletedValue();
	++Value;
	//!< GPUが到達すれば Value になる
	VERIFY_SUCCEEDED(CQ->Signal(Fence, Value));
	if (Fence->GetCompletedValue() < Value) {
		auto hEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		if (nullptr != hEvent) [[likely]] {
			//!< GetCompletedValue() が FenceValue になったらイベントが発行されるようにする
			VERIFY_SUCCEEDED(Fence->SetEventOnCompletion(Value, hEvent));
			//!< イベント発行まで待つ
			WaitForSingleObject(hEvent, INFINITE);
			CloseHandle(hEvent);
		}
	}
}
void DX::SubmitGraphics(const UINT i)
{
	const std::array CLs = { static_cast<ID3D12CommandList*>(COM_PTR_GET(DirectCommandLists[i])) };
	GraphicsCommandQueue->ExecuteCommandLists(static_cast<UINT>(size(CLs)), data(CLs));
}
void DX::Present()
{
	VERIFY_SUCCEEDED(SwapChain->Present(1/*垂直同期を待つ*/, 0));
}
void DX::Draw()
{
	WaitForFence(COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence));

	const auto Index = SwapChain->GetCurrentBackBufferIndex();
	DrawFrame(Index);
	SubmitGraphics(Index);

	Present();
}