#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloDX.h"

class DisplacementDX : public HoloViewsImageDX
{
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		Holo::SetHoloWindow(hWnd, hInstance);

		CreateProjectionMatrices();
		{
			const auto Pos = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);
			const auto Tag = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			const auto Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
			View = DirectX::XMMatrixLookAtRH(Pos, Tag, Up);
		}
		CreateViewMatrices();
		UpdateViewProjectionBuffer();

		DX::OnCreate(hWnd, hInstance, Title);
	}

	virtual void CreateCommandList() override {
		DX::CreateCommandList();
		DX::CreateBundleCommandList(2);
	}

	virtual void CreateGeometry() override {
		const auto CA = COM_PTR_GET(DirectCommandAllocators[0]);
		const auto CL = COM_PTR_GET(DirectCommandLists[0]);
		const auto GCQ = COM_PTR_GET(GraphicsCommandQueue);
		const auto GF = COM_PTR_GET(GraphicsFence);

		//!<【Pass0】メッシュ描画用 [To draw mesh] 
		constexpr D3D12_DRAW_ARGUMENTS DA0 = {
			.VertexCountPerInstance = 1,
			.InstanceCount = 1,
			.StartVertexLocation = 0,
			.StartInstanceLocation = 0
		};
		LOG(std::data(std::format("InstanceCount = {}\n", DA0.InstanceCount)));
		IndirectBuffers.emplace_back().Create(COM_PTR_GET(Device), DA0);
		UploadResource UploadPass0Indirect;
		UploadPass0Indirect.Create(COM_PTR_GET(Device), sizeof(DA0), &DA0);

		//!<【Pass1】フルスクリーン描画用 [To draw render texture]
		constexpr D3D12_DRAW_ARGUMENTS DA1 = {
			.VertexCountPerInstance = 4,
			.InstanceCount = 1,
			.StartVertexLocation = 0,
			.StartInstanceLocation = 0
		};
		IndirectBuffers.emplace_back().Create(COM_PTR_GET(Device), DA1).ExecuteCopyCommand(COM_PTR_GET(Device), CA, CL, GCQ, GF, sizeof(DA1), &DA1);
		UploadResource UploadPass1Indirect;
		UploadPass1Indirect.Create(COM_PTR_GET(Device), sizeof(DA1), &DA1);

		//!< コマンド発行 [Issue upload command]
		VERIFY_SUCCEEDED(CL->Reset(CA, nullptr)); {
			//!<【Pass0】
			IndirectBuffers[0].PopulateCopyCommand(CL, sizeof(DA0), COM_PTR_GET(UploadPass0Indirect.Resource));

			//!<【Pass1】
			IndirectBuffers[1].PopulateCopyCommand(CL, sizeof(DA1), COM_PTR_GET(UploadPass1Indirect.Resource));
		} VERIFY_SUCCEEDED(CL->Close());
		DX::ExecuteAndWait(GCQ, CL, COM_PTR_GET(GraphicsFence));
	}
	virtual void CreateConstantBuffer() override {
		//!<【Pass0】
		ConstantBuffers.emplace_back().Create(COM_PTR_GET(Device), sizeof(ViewProjectionBuffer));
		CopyToUploadResource(COM_PTR_GET(ConstantBuffers.back().Resource), RoundUp256(sizeof(ViewProjectionBuffer)), &ViewProjectionBuffer);

		//!<【Pass1】
		ConstantBuffers.emplace_back().Create(COM_PTR_GET(Device), sizeof(LenticularBuffer));
		CopyToUploadResource(COM_PTR_GET(ConstantBuffers.back().Resource), RoundUp256(sizeof(LenticularBuffer)), &LenticularBuffer);
	}
	virtual void CreateTexture() override {
		//!<【Pass0】レンダー、デプスターゲット (キルトサイズ)  [Render and depth target (quilt size)]
		CreateTexture_Render(QuiltX, QuiltY);
		CreateTexture_Depth(QuiltX, QuiltY);

		//!< デプス、カラーテクスチャ
		const auto DCA = DirectCommandAllocators[0];
		const auto DCL = DirectCommandLists[0];
#if 1
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "Rocks007_2K_Color.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "Rocks007_2K_Displacement.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
#else
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "color.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		XTKTextures.emplace_back().Create(COM_PTR_GET(Device), std::filesystem::path("..") / "Asset" / "depth.dds").ExecuteCopyCommand(COM_PTR_GET(Device), COM_PTR_GET(DCA), COM_PTR_GET(DCL), COM_PTR_GET(GraphicsCommandQueue), COM_PTR_GET(GraphicsFence), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
#endif
	}
	virtual void CreateStaticSampler() override {
		//!<【Pass1】
		CreateStaticSampler_LinearWrap(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
		CreateStaticSampler_LinearWrap(1, 0, D3D12_SHADER_VISIBILITY_DOMAIN);
	}
	virtual void CreateRootSignature() override {
		//!<【Pass0】
		{
			COM_PTR<ID3DBlob> Blob;
			constexpr std::array DRs_CBV = {
				D3D12_DESCRIPTOR_RANGE1({
					.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
					.NumDescriptors = 1, .BaseShaderRegister = 0, .RegisterSpace = 0,
					.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
					.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
				})
			};
			constexpr std::array DRs_SRV0 = {
				D3D12_DESCRIPTOR_RANGE1({
					.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
					.NumDescriptors = 1, .BaseShaderRegister = 0, .RegisterSpace = 0, 
					.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
					.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND 
				})
			};
			constexpr std::array DRs_SRV1 = {
				D3D12_DESCRIPTOR_RANGE1({
					.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
					.NumDescriptors = 1, .BaseShaderRegister = 1, .RegisterSpace = 0, 
					.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
					.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
				})
			};
			DX::SerializeRootSignature(Blob,
				{
					D3D12_ROOT_PARAMETER1({
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
						.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1({.NumDescriptorRanges = static_cast<UINT>(std::size(DRs_CBV)), .pDescriptorRanges = std::data(DRs_CBV) }),
						.ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY
					}),
					D3D12_ROOT_PARAMETER1({
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
						.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1({.NumDescriptorRanges = static_cast<UINT>(std::size(DRs_SRV0)), .pDescriptorRanges = std::data(DRs_SRV0)}),
						.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
					}),
					D3D12_ROOT_PARAMETER1({
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
						.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1({.NumDescriptorRanges = static_cast<UINT>(std::size(DRs_SRV1)), .pDescriptorRanges = std::data(DRs_SRV1) }),
						.ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN
					}),
				},
				{
					StaticSamplerDescs[0], //!< SHADER_VISIBILITY_PIXEL
					StaticSamplerDescs[1], //!< SHADER_VISIBILITY_DOMAIN
				},
				SHADER_ROOT_ACCESS_DS_GS_PS);
			VERIFY_SUCCEEDED(Device->CreateRootSignature(0, Blob->GetBufferPointer(), Blob->GetBufferSize(), COM_PTR_UUIDOF_PUTVOID(RootSignatures.emplace_back())));
		}
		//!<【Pass1】
		{
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
					D3D12_ROOT_PARAMETER1({
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
						.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1({.NumDescriptorRanges = static_cast<uint32_t>(std::size(DRs_SRV)), .pDescriptorRanges = std::data(DRs_SRV) }),
						.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
					}),
					D3D12_ROOT_PARAMETER1({
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
						.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1({.NumDescriptorRanges = static_cast<UINT>(std::size(DRs_CBV)), .pDescriptorRanges = std::data(DRs_CBV) }),
						.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
					}),
				},
				{
					StaticSamplerDescs[0], //!< SHADER_VISIBILITY_PIXEL
				},
				SHADER_ROOT_ACCESS_PS);
			VERIFY_SUCCEEDED(Device->CreateRootSignature(0, Blob->GetBufferPointer(), Blob->GetBufferSize(), COM_PTR_UUIDOF_PUTVOID(RootSignatures.emplace_back())));
		}
	}
	virtual void CreatePipelineState() override {
		PipelineStates.emplace_back();
		PipelineStates.emplace_back();

		constexpr D3D12_RASTERIZER_DESC RD = {
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_BACK, .FrontCounterClockwise = TRUE,
			.DepthBias = D3D12_DEFAULT_DEPTH_BIAS, .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP, .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
			.DepthClipEnable = TRUE,
			.MultisampleEnable = FALSE, .AntialiasedLineEnable = FALSE, .ForcedSampleCount = 0,
			.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};

		//!<【Pass0】
		std::vector<COM_PTR<ID3DBlob>> SBs_Pass0;
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "DisplacementDX.vs.cso").wstring()), COM_PTR_PUT(SBs_Pass0.emplace_back())));
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "DisplacementDX.ps.cso").wstring()), COM_PTR_PUT(SBs_Pass0.emplace_back())));
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "DisplacementDX.ds.cso").wstring()), COM_PTR_PUT(SBs_Pass0.emplace_back())));
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "DisplacementDX.hs.cso").wstring()), COM_PTR_PUT(SBs_Pass0.emplace_back())));
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "DisplacementDX.gs.cso").wstring()), COM_PTR_PUT(SBs_Pass0.emplace_back())));
		const std::array SBCsPass0 = {
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs_Pass0[0]->GetBufferPointer(), .BytecodeLength = SBs_Pass0[0]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs_Pass0[1]->GetBufferPointer(), .BytecodeLength = SBs_Pass0[1]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs_Pass0[2]->GetBufferPointer(), .BytecodeLength = SBs_Pass0[2]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs_Pass0[3]->GetBufferPointer(), .BytecodeLength = SBs_Pass0[3]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs_Pass0[4]->GetBufferPointer(), .BytecodeLength = SBs_Pass0[4]->GetBufferSize() }),
		};
		CreatePipelineState_VsPsDsHsGs(PipelineStates[0], COM_PTR_GET(RootSignatures[0]), D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH, RD, TRUE, SBCsPass0);

		//!< [Pass1]
		std::vector<COM_PTR<ID3DBlob>> SBs_Pass1;
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "FinalPassDX.vs.cso").wstring()), COM_PTR_PUT(SBs_Pass1.emplace_back())));
#ifdef DISPLAY_QUILT
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "FinalPassQuiltDispDX.ps.cso").wstring()), COM_PTR_PUT(SBs_Pass1.emplace_back())));
#else
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "FinalPassDX.ps.cso").wstring()), COM_PTR_PUT(SBs_Pass1.emplace_back())));
#endif
		const std::array SBCsPass1 = {
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs_Pass1[0]->GetBufferPointer(), .BytecodeLength = SBs_Pass1[0]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs_Pass1[1]->GetBufferPointer(), .BytecodeLength = SBs_Pass1[1]->GetBufferSize() }),
		};
		CreatePipelineState_VsPs(PipelineStates[1], COM_PTR_GET(RootSignatures[1]), D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, RD, FALSE, SBCsPass1);

		for (auto& i : Threads) { i.join(); }
		Threads.clear();
	}
	void CreateDescriptor_Pass0() {
		{
			const auto DescCount = 1;

			//!< レンダーターゲットビュー [Render target view]
			{
				auto& Desc = RtvDescs.emplace_back();
				auto& Heap = Desc.first;
				auto& Handle = Desc.second;
				const D3D12_DESCRIPTOR_HEAP_DESC DHD = {
					.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
					.NumDescriptors = DescCount,
					.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
					.NodeMask = 0
				};
				VERIFY_SUCCEEDED(Device->CreateDescriptorHeap(&DHD, COM_PTR_UUIDOF_PUTVOID(Heap)));
				auto CDH = Heap->GetCPUDescriptorHandleForHeapStart();
				const auto IncSize = Device->GetDescriptorHandleIncrementSize(Heap->GetDesc().Type);

				const auto& Tex = RenderTextures[0];
				Device->CreateRenderTargetView(COM_PTR_GET(Tex.Resource), &Tex.RTV, CDH);
				Handle.emplace_back(CDH);
				CDH.ptr += IncSize;
			}
			//!< デプスステンシルビュー [Depth stencil view]
			{
				auto& Desc = DsvDescs.emplace_back();
				auto& Heap = Desc.first;
				auto& Handle = Desc.second;
				const D3D12_DESCRIPTOR_HEAP_DESC DHD = {
					.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
					.NumDescriptors = DescCount,
					.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
					.NodeMask = 0
				};
				VERIFY_SUCCEEDED(Device->CreateDescriptorHeap(&DHD, COM_PTR_UUIDOF_PUTVOID(Heap)));
				auto CDH = Heap->GetCPUDescriptorHandleForHeapStart();
				const auto IncSize = Device->GetDescriptorHandleIncrementSize(Heap->GetDesc().Type);

				const auto& Tex = DepthTextures[0];
				Device->CreateDepthStencilView(COM_PTR_GET(Tex.Resource), &Tex.DSV, CDH);
				Handle.emplace_back(CDH);
				CDH.ptr += IncSize;
			}
		}

		//!< コンスタントバッファービュー [Constant buffer view]
		{
			const auto DescCount = GetViewportDrawCount() + 2;

			{
				auto& Desc = CbvSrvUavDescs.emplace_back();
				auto& Heap = Desc.first;
				auto& Handle = Desc.second;
				const D3D12_DESCRIPTOR_HEAP_DESC DHD = {
					.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
					.NumDescriptors = DescCount,
					.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
					.NodeMask = 0
				};
				VERIFY_SUCCEEDED(Device->CreateDescriptorHeap(&DHD, COM_PTR_UUIDOF_PUTVOID(Heap)));
				auto CDH = Heap->GetCPUDescriptorHandleForHeapStart();
				auto GDH = Heap->GetGPUDescriptorHandleForHeapStart();
				const auto IncSize = Device->GetDescriptorHandleIncrementSize(Heap->GetDesc().Type);

				const auto& CB = ConstantBuffers[0];
				//!< オフセット毎に使用するサイズ [Offset size]
				const auto DynamicOffset = GetViewportMax() * sizeof(ViewProjectionBuffer.ViewProjection[0]);
				//!< ビュー、ハンドルを描画回数分用意する [View and handles of draw count]
				for (UINT i = 0; i < GetViewportDrawCount(); ++i) {
					//!< オフセット毎の位置、サイズ [Offset location and size]
					const D3D12_CONSTANT_BUFFER_VIEW_DESC CBVD = {
						.BufferLocation = CB.Resource->GetGPUVirtualAddress() + DynamicOffset * i,
						.SizeInBytes = static_cast<UINT>(DynamicOffset)
					};
					Device->CreateConstantBufferView(&CBVD, CDH);
					Handle.emplace_back(GDH);
					CDH.ptr += IncSize;
					GDH.ptr += IncSize;
				}
				//!< SRV0
				Device->CreateShaderResourceView(COM_PTR_GET(XTKTextures[0].Resource), &XTKTextures[0].SRV, CDH);
				Handle.emplace_back(GDH);
				CDH.ptr += IncSize;
				GDH.ptr += IncSize;
				//!< SRV1
				Device->CreateShaderResourceView(COM_PTR_GET(XTKTextures[1].Resource), &XTKTextures[1].SRV, CDH);
				Handle.emplace_back(GDH);
				CDH.ptr += IncSize;
				GDH.ptr += IncSize;
			}
		}
	}
	void CreateDescriptor_Pass1() {
		auto& Desc = CbvSrvUavDescs.emplace_back();
		auto& Heap = Desc.first;
		auto& Handle = Desc.second;
		const D3D12_DESCRIPTOR_HEAP_DESC DHD = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 2,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			.NodeMask = 0
		};
		VERIFY_SUCCEEDED(Device->CreateDescriptorHeap(&DHD, COM_PTR_UUIDOF_PUTVOID(Heap)));
		auto CDH = Heap->GetCPUDescriptorHandleForHeapStart();
		auto GDH = Heap->GetGPUDescriptorHandleForHeapStart();
		const auto IncSize = Device->GetDescriptorHandleIncrementSize(Heap->GetDesc().Type);

		//!< シェーダリソースビュー [Shader resource view]
		{
			const auto& Tex = RenderTextures[0];
			Device->CreateShaderResourceView(COM_PTR_GET(Tex.Resource), &Tex.SRV, CDH);
			Handle.emplace_back(GDH);
			CDH.ptr += IncSize;
			GDH.ptr += IncSize;
		}
		//!< コンスタントバッファービュー [Constant buffer view]
		{
			const auto& CB = ConstantBuffers[1];
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
	virtual void CreateDescriptor() override {
		CreateDescriptor_Pass0();
		CreateDescriptor_Pass1();
	}
	virtual void CreateViewport(const FLOAT Width, const FLOAT Height, const FLOAT MinDepth = 0.0f, const FLOAT MaxDepth = 1.0f) override {
		D3D12_FEATURE_DATA_D3D12_OPTIONS3 FDO3;
		VERIFY_SUCCEEDED(Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, reinterpret_cast<void*>(&FDO3), sizeof(FDO3)));
		assert(D3D12_VIEW_INSTANCING_TIER_1 < FDO3.ViewInstancingTier && "");

		//!<【Pass0】
		HoloViewsDX::CreateViewportScissor(MinDepth, MaxDepth);
		//!<【Pass1】スクリーンを使用 [Using screen]
		DX::CreateViewport(Width, Height, MinDepth, MaxDepth);
	}
	void PopulateBundleCommandList_Pass0() {
		const auto BCA = COM_PTR_GET(BundleCommandAllocators[0]);
		const auto BCL = COM_PTR_GET(BundleCommandLists[0]);
		const auto PS = COM_PTR_GET(PipelineStates[0]);
		VERIFY_SUCCEEDED(BCL->Reset(BCA, PS));
		{
			BCL->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
			
			const auto IB = IndirectBuffers[0];
			BCL->ExecuteIndirect(COM_PTR_GET(IB.CommandSignature), 1, COM_PTR_GET(IB.Resource), 0, nullptr, 0);
		}
		VERIFY_SUCCEEDED(BCL->Close());
	}
	void PopulateBundleCommandList_Pass1() {
		const auto BCA = COM_PTR_GET(BundleCommandAllocators[0]);
		const auto BCL = COM_PTR_GET(BundleCommandLists[1]);
		const auto PS = COM_PTR_GET(PipelineStates[1]);
		VERIFY_SUCCEEDED(BCL->Reset(BCA, PS));
		{
			BCL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			const auto IB = IndirectBuffers[1];
			BCL->ExecuteIndirect(COM_PTR_GET(IB.CommandSignature), 1, COM_PTR_GET(IB.Resource), 0, nullptr, 0);
		}
		VERIFY_SUCCEEDED(BCL->Close());
	}
	virtual void PopulateBundleCommandList(const size_t i) override {
		if (0 == i) {
			//!<【Pass0】
			PopulateBundleCommandList_Pass0();
			//!<【Pass1】
			PopulateBundleCommandList_Pass1();
		}
	}
	virtual void PopulateCommandList(const size_t i) override {
		const auto DCL = COM_PTR_GET(DirectCommandLists[i]);
		const auto DCA = COM_PTR_GET(DirectCommandAllocators[0]);
		VERIFY_SUCCEEDED(DCL->Reset(DCA, nullptr));
		{
			//!<【Pass0】
			{
				const auto RS = COM_PTR_GET(RootSignatures[0]);
				const auto BCL = COM_PTR_GET(BundleCommandLists[0]);

				DCL->SetGraphicsRootSignature(RS);

				//!< レンダー、デプスターゲット [Render, depth target]
				{
					const auto& HandleRTV = RtvDescs[0].second[0];
					const auto& HandleDSV = DsvDescs[0].second[0];

					constexpr std::array<D3D12_RECT, 0> Rects = {};
					DCL->ClearRenderTargetView(HandleRTV, DirectX::Colors::SkyBlue, static_cast<UINT>(std::size(Rects)), std::data(Rects));
					DCL->ClearDepthStencilView(HandleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, static_cast<UINT>(std::size(Rects)), std::data(Rects));

					const std::array CHs = { HandleRTV };
					DCL->OMSetRenderTargets(static_cast<UINT>(std::size(CHs)), std::data(CHs), FALSE, &HandleDSV);
				}

				{
					//!< コンスタントバッファ [Constant buffer]
					const auto& Desc = CbvSrvUavDescs[0];
					const auto& Heap = Desc.first;
					const auto& Handle = Desc.second;
					const std::array DHs = { COM_PTR_GET(Heap) };
					DCL->SetDescriptorHeaps(static_cast<UINT>(std::size(DHs)), std::data(DHs));

					const auto DrawCount = GetViewportDrawCount();
					//!< SRV0, SRV1
					DCL->SetGraphicsRootDescriptorTable(1, Handle[DrawCount]);	
					DCL->SetGraphicsRootDescriptorTable(2, Handle[DrawCount + 1]);

					//!< キルトパターン描画 (ビューポート同時描画数に制限がある為、要複数回実行) [Because viewport max is 16, need to draw few times]
					for (uint32_t j = 0; j < DrawCount; ++j) {
						const auto Offset = GetViewportSetOffset(j);
						const auto Count = GetViewportSetCount(j);

						DCL->RSSetViewports(Count, &QuiltViewports[Offset]);
						DCL->RSSetScissorRects(Count, &QuiltScissorRects[Offset]);

						//!< CBV オフセット毎のハンドルを使用 [Use handle on each offset]
						DCL->SetGraphicsRootDescriptorTable(0, Handle[j]);

						DCL->ExecuteBundle(BCL);
					}
				}
			}

			const auto SCR = COM_PTR_GET(SwapChainBackBuffers[i].Resource);
			const auto RT = COM_PTR_GET(RenderTextures[0].Resource);

			//!< バリア [Barrier]
			//!< (スワップチェインをレンダーターゲットへ、レンダーテクスチャをシェーダリソースへ)
			ResourceBarrier2(DCL,
				SCR, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET,
				RT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			//!<【Pass1】
			{
				const auto RS = COM_PTR_GET(RootSignatures[1]);
				const auto BCL = COM_PTR_GET(BundleCommandLists[1]);

				DCL->SetGraphicsRootSignature(RS);

				DCL->RSSetViewports(static_cast<UINT>(std::size(Viewports)), std::data(Viewports));
				DCL->RSSetScissorRects(static_cast<UINT>(std::size(ScissorRects)), std::data(ScissorRects));

				const std::array CHs = { SwapChainBackBuffers[i].Handle };
				DCL->OMSetRenderTargets(static_cast<UINT>(std::size(CHs)), std::data(CHs), FALSE, nullptr);

				//!< デスクリプタ [Descriptor]
				{
					const auto& Desc = CbvSrvUavDescs[1];
					const auto& Heap = Desc.first;
					const auto& Handle = Desc.second;

					const std::array DHs = { COM_PTR_GET(Heap) };
					DCL->SetDescriptorHeaps(static_cast<UINT>(std::size(DHs)), std::data(DHs));
					DCL->SetGraphicsRootDescriptorTable(0, Handle[0]); //!< SRV
					DCL->SetGraphicsRootDescriptorTable(1, Handle[1]); //!< CBV
				}

				DCL->ExecuteBundle(BCL);
			}

			//!< バリア [Barrier]
			//!< (スワップチェインをプレゼントへ、レンダーテクスチャをレンダーターゲットへ)
			ResourceBarrier2(DCL,
				SCR, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT,
				RT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}
		VERIFY_SUCCEEDED(DCL->Close());
	}
	virtual void UpdateViewProjectionBuffer() override {
		const auto Count = (std::min)(static_cast<size_t>(LenticularBuffer.TileX * LenticularBuffer.TileY), _countof(ViewProjectionBuffer.ViewProjection));
		for (auto i = 0; i < Count; ++i) {
			DirectX::XMStoreFloat4x4(&ViewProjectionBuffer.ViewProjection[i], ViewMatrices[i] * ProjectionMatrices[i]);
		}
	}

protected:
	struct VIEW_PROJECTION_BUFFER {
		DirectX::XMFLOAT4X4 ViewProjection[64];
	};
	VIEW_PROJECTION_BUFFER ViewProjectionBuffer;
};