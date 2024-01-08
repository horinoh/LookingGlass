#pragma once

#include "resource.h"

#include "../DXImage.h"
#include "../Holo.h"

class QuiltDX : public DXImage, public Holo
{
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		//!< Looking Glass ウインドウのサイズを取得してから
		Holo::SetHoloWindow(hWnd, hInstance);
		//!< DX の準備を行う
		DX::OnCreate(hWnd, hInstance, Title);
	}

	virtual void CreateGeometry() override {
		const auto DCA = DirectCommandAllocators[0];
		const auto DCL = DirectCommandLists[0];
		constexpr D3D12_DRAW_ARGUMENTS DA = {
			.VertexCountPerInstance = 4,
			.InstanceCount = 1,
			.StartVertexLocation = 0, 
			.StartInstanceLocation = 0 
		};
		IndirectBuffers.emplace_back().Create(COM_PTR_GET(Device), DA).ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), sizeof(DA), &DA);
	}
	virtual void CreateConstantBuffer() override {
		//DXGI_SWAP_CHAIN_DESC1 SCD;
		//SwapChain->GetDesc1(&SCD);
		//for (UINT i = 0; i < SCD.BufferCount; ++i) {
		//	ConstantBuffers.emplace_back().Create(COM_PTR_GET(Device), sizeof(*LenticularBuffer));
		//}
		ConstantBuffers.emplace_back().Create(COM_PTR_GET(Device), sizeof(*LenticularBuffer));
	}
	//!< キルト画像は dds 形式にして Asset フォルダ内へ配置しておく
	virtual void CreateTexture() override {
		const auto DCA = DirectCommandAllocators[0];
		const auto DCL = DirectCommandLists[0];
		//example01.dds // 4x8
		//mar1_qs5x8.dds 
		//dedouze_qs5x9.dds
		//inventorbench_jayhowse_qs5x9.dds
		//j_smf_lightfield_qs5x9.dds
		//timestar_40.dds // 8x6
		//soccerballquilt.dds // 8x6
		//Jane_Guan_Space_Nap_qs8x6.dds //https://docs.lookingglassfactory.com/keyconcepts/quilts
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "Jane_Guan_Space_Nap_qs8x6.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		
		if (nullptr != LenticularBuffer) {
			const auto RD = XTKTextures.back().Resource->GetDesc();
			//!< キルト画像の分割に合わせて 引数 Column, Row を指定すること
			Holo::UpdateLenticularBuffer(8, 6, static_cast<uint32_t>(RD.Width), RD.Height);

			//for (auto i : ConstantBuffers) {
			//	CopyToUploadResource(COM_PTR_GET(i.Resource), RoundUp256(sizeof(*LenticularBuffer)), LenticularBuffer);
			//}
			auto& CB = ConstantBuffers[0];
			CopyToUploadResource(COM_PTR_GET(CB.Resource), RoundUp256(sizeof(*LenticularBuffer)), LenticularBuffer);
		}
	}
	virtual void CreateStaticSampler() override {
		CreateStaticSampler_LinearWrap(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	}
	virtual void CreateRootSignature() override {
		COM_PTR<ID3DBlob> Blob;
		constexpr std::array DRs_Srv = {
			D3D12_DESCRIPTOR_RANGE({.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV, .NumDescriptors = 1, .BaseShaderRegister = 0, .RegisterSpace = 0, .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND })
		};
		constexpr std::array DRs_Cbv = {
			D3D12_DESCRIPTOR_RANGE({.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV, .NumDescriptors = 1, .BaseShaderRegister = 0, .RegisterSpace = 0, .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND })
		};
		DX::SerializeRootSignature(Blob, 
			{
				//!< SRV
				D3D12_ROOT_PARAMETER({
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE({.NumDescriptorRanges = static_cast<uint32_t>(size(DRs_Srv)), .pDescriptorRanges = data(DRs_Srv) }),
					.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
				}),
				//!< CBV
				D3D12_ROOT_PARAMETER({
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE({.NumDescriptorRanges = static_cast<UINT>(size(DRs_Cbv)), .pDescriptorRanges = data(DRs_Cbv) }),
					.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
				}),
			}, 
			{
				StaticSamplerDescs[0],
			}, 
			SHADER_ROOT_ACCESS_PS);

		VERIFY_SUCCEEDED(Device->CreateRootSignature(0, Blob->GetBufferPointer(), Blob->GetBufferSize(), COM_PTR_UUIDOF_PUTVOID(RootSignatures.emplace_back())));
	}
	virtual void CreatePipelineState() override {
		PipelineStates.emplace_back();

		std::vector<COM_PTR<ID3DBlob>> SBs = {};
		VERIFY_SUCCEEDED(D3DReadFileToBlob(data((std::filesystem::path(".") / "QuiltDX.vs.cso").wstring()), COM_PTR_PUT(SBs.emplace_back())));
		VERIFY_SUCCEEDED(D3DReadFileToBlob(data((std::filesystem::path(".") / "QuiltDX.ps.cso").wstring()), COM_PTR_PUT(SBs.emplace_back())));
		const std::array SBCs = {
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs[0]->GetBufferPointer(), .BytecodeLength = SBs[0]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs[1]->GetBufferPointer(), .BytecodeLength = SBs[1]->GetBufferSize() }),
		};
		constexpr D3D12_RASTERIZER_DESC RD = {
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_BACK, .FrontCounterClockwise = TRUE,
			.DepthBias = D3D12_DEFAULT_DEPTH_BIAS, .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP, .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
			.DepthClipEnable = TRUE,
			.MultisampleEnable = FALSE, .AntialiasedLineEnable = FALSE, .ForcedSampleCount = 0,
			.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};
		CreatePipelineState_VsPs(PipelineStates[0], COM_PTR_GET(RootSignatures[0]), D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, RD, FALSE, SBCs);

		for (auto& i : Threads) { i.join(); }
		Threads.clear();
	}
	virtual void CreateDescriptor() override {
		auto& Desc = CbvSrvUavDescs.emplace_back();
		auto& Heap = Desc.first;
		auto& Handle = Desc.second;

		const D3D12_DESCRIPTOR_HEAP_DESC DHD = { .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, .NumDescriptors = 1, .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, .NodeMask = 0 }; //!< SRV
		VERIFY_SUCCEEDED(Device->CreateDescriptorHeap(&DHD, COM_PTR_UUIDOF_PUTVOID(Heap)));

		auto CDH = Heap->GetCPUDescriptorHandleForHeapStart();
		auto GDH = Heap->GetGPUDescriptorHandleForHeapStart();
		const auto IncSize = Device->GetDescriptorHandleIncrementSize(Heap->GetDesc().Type);

		const auto& Tex = XTKTextures[0];
		Device->CreateShaderResourceView(COM_PTR_GET(Tex.Resource), &Tex.SRV, CDH);
		Handle.emplace_back(GDH);
		CDH.ptr += IncSize;
		GDH.ptr += IncSize;

		//DXGI_SWAP_CHAIN_DESC1 SCD;
		//SwapChain->GetDesc1(&SCD);
		//for (UINT i = 0; i < SCD.BufferCount; ++i) {
		//	const auto& CB = ConstantBuffers[i];
		//	const D3D12_CONSTANT_BUFFER_VIEW_DESC CBVD = { .BufferLocation = CB.Resource->GetGPUVirtualAddress(), .SizeInBytes = static_cast<UINT>(CB.Resource->GetDesc().Width)};
		//	Device->CreateConstantBufferView(&CBVD, CDH);
		//	Handle.emplace_back(GDH);
		//	CDH.ptr += IncSize;
		//	GDH.ptr += IncSize;
		//}
		const auto& CB = ConstantBuffers[0];
		const D3D12_CONSTANT_BUFFER_VIEW_DESC CBVD = { .BufferLocation = CB.Resource->GetGPUVirtualAddress(), .SizeInBytes = static_cast<UINT>(CB.Resource->GetDesc().Width) };
		Device->CreateConstantBufferView(&CBVD, CDH);
		Handle.emplace_back(GDH);
		CDH.ptr += IncSize;
		GDH.ptr += IncSize;
	}
	virtual void PopulateCommandList(const size_t i) override {
		const auto PS = COM_PTR_GET(PipelineStates[0]);

		const auto DescIndex = 0;//i;

		//!< バンドルコマンドリスト
		const auto BCL = COM_PTR_GET(BundleCommandLists[i]);
		const auto BCA = COM_PTR_GET(BundleCommandAllocators[0]);
		VERIFY_SUCCEEDED(BCL->Reset(BCA, PS));	
		{
			BCL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			BCL->ExecuteIndirect(COM_PTR_GET(IndirectBuffers[0].CommandSignature), 1, COM_PTR_GET(IndirectBuffers[0].Resource), 0, nullptr, 0);
		}
		VERIFY_SUCCEEDED(BCL->Close());

		//!< ダイレクトコマンドリスト
		const auto CL = COM_PTR_GET(DirectCommandLists[i]);
		const auto CA = COM_PTR_GET(DirectCommandAllocators[0]);
		VERIFY_SUCCEEDED(CL->Reset(CA, PS));
		{
			CL->SetGraphicsRootSignature(COM_PTR_GET(RootSignatures[0]));

			CL->RSSetViewports(static_cast<UINT>(size(Viewports)), data(Viewports));
			CL->RSSetScissorRects(static_cast<UINT>(size(ScissorRects)), data(ScissorRects));

			const auto SCR = COM_PTR_GET(SwapChainResources[i]);
			ResourceBarrier(CL, SCR, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			{
				const std::array CHs = { SwapChainCPUHandles[i] };
				CL->OMSetRenderTargets(static_cast<UINT>(size(CHs)), data(CHs), FALSE, nullptr);

				//!< デスクリプタ
				{
					const auto& Desc = CbvSrvUavDescs[0];
					const auto& Heap = Desc.first;
					const auto& Handle = Desc.second;
					const std::array DHs = { COM_PTR_GET(Heap) };
					CL->SetDescriptorHeaps(static_cast<UINT>(size(DHs)), data(DHs));
					CL->SetGraphicsRootDescriptorTable(0, Handle[0]); //!< SRV
					CL->SetGraphicsRootDescriptorTable(1, Handle[1 + DescIndex]); //!< CBV
				}

				CL->ExecuteBundle(BCL);
			}
			ResourceBarrier(CL, SCR, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		}
		VERIFY_SUCCEEDED(CL->Close());
	}

	virtual uint32_t GetViewportMax() const override { return D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; }
};
