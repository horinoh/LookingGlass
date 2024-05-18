#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloDX.h"

class QuiltDX : public HoloImageDX
{
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		//!< Looking Glass ウインドウのサイズを取得してから  [Get window size]
		Holo::SetHoloWindow(hWnd, hInstance);
		//!< DX の準備を行う [Prepare DX]
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
		ConstantBuffers.emplace_back().Create(COM_PTR_GET(Device), sizeof(LenticularBuffer));
		//!< CopyToUploadResource() はテクスチャ情報確定後 CreateTexture() 内でやっている
	}
	//!< キルト画像は dds 形式にして Asset フォルダ内へ配置しておく [Convert quilt image to dds format, and put in Asset folder]
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

		const auto RD = XTKTextures.back().Resource->GetDesc();
		//!< キルト画像の分割に合わせて 引数 Column, Row を指定すること [Specify Column Row to suit quilt image]
		Holo::UpdateLenticularBuffer(8, 6, static_cast<uint32_t>(RD.Width), RD.Height);
		auto& CB = ConstantBuffers[0];
		CopyToUploadResource(COM_PTR_GET(CB.Resource), RoundUp256(sizeof(LenticularBuffer)), &LenticularBuffer);
	}
	virtual void CreateStaticSampler() override {
		CreateStaticSampler_LinearWrap(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	}
	virtual void CreateRootSignature() override {
		COM_PTR<ID3DBlob> Blob;
		constexpr std::array DRs_SRV = {
			D3D12_DESCRIPTOR_RANGE1({
				.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 
				.NumDescriptors = 1, .BaseShaderRegister = 0, .RegisterSpace = 0,
				.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 
				.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND 
			})
		};
		constexpr std::array DRs_CBV = {
			D3D12_DESCRIPTOR_RANGE1({
				.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 
				.NumDescriptors = 1, .BaseShaderRegister = 0, .RegisterSpace = 0, 
				.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 
				.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
			})
		};
		DX::SerializeRootSignature(Blob,
			{
				//!< SRV -> SetGraphicsRootDescriptorTable(0,..)
				D3D12_ROOT_PARAMETER1({
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1({.NumDescriptorRanges = static_cast<uint32_t>(std::size(DRs_SRV)), .pDescriptorRanges = std::data(DRs_SRV) }),
					.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
				}),
				//!< CBV -> SetGraphicsRootDescriptorTable(1,..)
				D3D12_ROOT_PARAMETER1({
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1({.NumDescriptorRanges = static_cast<UINT>(std::size(DRs_CBV)), .pDescriptorRanges = std::data(DRs_CBV) }),
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
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "QuiltDX.vs.cso").wstring()), COM_PTR_PUT(SBs.emplace_back())));
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "QuiltDX.ps.cso").wstring()), COM_PTR_PUT(SBs.emplace_back())));
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
		const auto DescCount = 2;
		{
			auto& Desc = CbvSrvUavDescs.emplace_back();
			auto& Heap = Desc.first;
			auto& Handle = Desc.second;
			const D3D12_DESCRIPTOR_HEAP_DESC DHD = {
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = DescCount,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, .NodeMask = 0
			};
			VERIFY_SUCCEEDED(Device->CreateDescriptorHeap(&DHD, COM_PTR_UUIDOF_PUTVOID(Heap)));
			auto CDH = Heap->GetCPUDescriptorHandleForHeapStart();
			auto GDH = Heap->GetGPUDescriptorHandleForHeapStart();
			const auto IncSize = Device->GetDescriptorHandleIncrementSize(Heap->GetDesc().Type);

			//!< SRV
			{
				const auto& Tex = XTKTextures[0];
				Device->CreateShaderResourceView(COM_PTR_GET(Tex.Resource), &Tex.SRV, CDH);
				Handle.emplace_back(GDH);
				CDH.ptr += IncSize;
				GDH.ptr += IncSize;
			}
			//!< CBV
			{
				const auto& CB = ConstantBuffers[0];
				const D3D12_CONSTANT_BUFFER_VIEW_DESC CBVD = {
					.BufferLocation = CB.Resource->GetGPUVirtualAddress(),
					.SizeInBytes = static_cast<UINT>(CB.Resource->GetDesc().Width)
				};
				Device->CreateConstantBufferView(&CBVD, CDH);
				Handle.emplace_back(GDH);
				CDH.ptr += IncSize;
				GDH.ptr += IncSize;
			}
		}
	}
	virtual	void PopulateBundleCommandList(const size_t i) override {
		if (0 == i) {
			const auto PS = COM_PTR_GET(PipelineStates[0]);
			const auto BCL = COM_PTR_GET(BundleCommandLists[0]);
			const auto BCA = COM_PTR_GET(BundleCommandAllocators[0]);
			const auto RS = COM_PTR_GET(RootSignatures[0]);

			VERIFY_SUCCEEDED(BCL->Reset(BCA, PS));
			{
				BCL->SetGraphicsRootSignature(RS);

				BCL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				BCL->ExecuteIndirect(COM_PTR_GET(IndirectBuffers[0].CommandSignature), 1, COM_PTR_GET(IndirectBuffers[0].Resource), 0, nullptr, 0);
			}
			VERIFY_SUCCEEDED(BCL->Close());
		}
	}
	virtual void PopulateCommandList(const size_t i) override {
		const auto PS = COM_PTR_GET(PipelineStates[0]);
		const auto BCL = COM_PTR_GET(BundleCommandLists[0]);
		const auto DCA = COM_PTR_GET(DirectCommandAllocators[0]);
		const auto DCL = COM_PTR_GET(DirectCommandLists[i]);
		const auto RS = COM_PTR_GET(RootSignatures[0]);

		VERIFY_SUCCEEDED(DCL->Reset(DCA, PS));
		{
			DCL->SetGraphicsRootSignature(RS);

			DCL->RSSetViewports(static_cast<UINT>(std::size(Viewports)), std::data(Viewports));
			DCL->RSSetScissorRects(static_cast<UINT>(std::size(ScissorRects)), std::data(ScissorRects));

			const auto SCR = COM_PTR_GET(SwapChainBackBuffers[i].Resource);
			ResourceBarrier(DCL, SCR, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			{
				const std::array CHs = { SwapChainBackBuffers[i].Handle };
				DCL->OMSetRenderTargets(static_cast<UINT>(std::size(CHs)), std::data(CHs), FALSE, nullptr);

				//!< デスクリプタ [Descriptor]
				{
					const auto& Desc = CbvSrvUavDescs[0];
					const auto& Heap = Desc.first;
					const auto& Handle = Desc.second;
					const std::array DHs = { COM_PTR_GET(Heap) };
					DCL->SetDescriptorHeaps(static_cast<UINT>(std::size(DHs)), std::data(DHs));
					DCL->SetGraphicsRootDescriptorTable(0, Handle[0]); //!< SRV
					DCL->SetGraphicsRootDescriptorTable(1, Handle[1]); //!< CBV
				}

				DCL->ExecuteBundle(BCL);
			}
			ResourceBarrier(DCL, SCR, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		}
		VERIFY_SUCCEEDED(DCL->Close());
	}
};
