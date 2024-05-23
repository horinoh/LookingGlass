#pragma once

#include "resource.h"

//#define USE_GLTF
#define USE_FBX
#include "../HoloDX.h"

#ifdef USE_GLTF
class MeshDX : public HoloGLTFDX
#else
class MeshDX : public HoloFBXDX
#endif
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
#ifdef USE_GLTF
		Load(std::filesystem::path("..") / "Asset" / "Mesh" / "bunny.glb");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "dragon.glb");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "happy_vrip.glb");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "Sphere.glb");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "Box.glb");
#else
		Load(std::filesystem::path("..") / "Asset" / "Mesh" / "bunny.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "dragon.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "happy_vrip.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "Bee.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "ALucy.FBX");
#endif	

		VertexBuffers.emplace_back().Create(COM_PTR_GET(Device), TotalSizeOf(Vertices), sizeof(Vertices[0]));
		UploadResource UploadPass0Vertex;
		UploadPass0Vertex.Create(COM_PTR_GET(Device), TotalSizeOf(Vertices), std::data(Vertices));

		VertexBuffers.emplace_back().Create(COM_PTR_GET(Device), TotalSizeOf(Normals), sizeof(Normals[0]));
		UploadResource UploadPass0Normal;
		UploadPass0Normal.Create(COM_PTR_GET(Device), TotalSizeOf(Normals), std::data(Normals));

		IndexBuffers.emplace_back().Create(COM_PTR_GET(Device), TotalSizeOf(Indices), DXGI_FORMAT_R32_UINT);
		UploadResource UploadPass0Index;
		UploadPass0Index.Create(COM_PTR_GET(Device), TotalSizeOf(Indices), std::data(Indices));

		const D3D12_DRAW_INDEXED_ARGUMENTS DIA = { 
			.IndexCountPerInstance = static_cast<UINT32>(std::size(Indices)),
			.InstanceCount = 1, 
			.StartIndexLocation = 0, 
			.BaseVertexLocation = 0, 
			.StartInstanceLocation = 0 
		};
		LOG(std::data(std::format("InstanceCount = {}\n", DIA.InstanceCount)));
		IndirectBuffers.emplace_back().Create(COM_PTR_GET(Device), DIA);
		UploadResource UploadPass0Indirect;
		UploadPass0Indirect.Create(COM_PTR_GET(Device), sizeof(DIA), &DIA);

		//!<【Pass1】フルスクリーン描画用 [To draw render texture]
		constexpr D3D12_DRAW_ARGUMENTS DA = {
			.VertexCountPerInstance = 4, 
			.InstanceCount = 1, 
			.StartVertexLocation = 0, 
			.StartInstanceLocation = 0
		};
		IndirectBuffers.emplace_back().Create(COM_PTR_GET(Device), DA).ExecuteCopyCommand(COM_PTR_GET(Device), CA, CL, GCQ, GF, sizeof(DA), &DA);
		UploadResource UploadPass1Indirect;
		UploadPass1Indirect.Create(COM_PTR_GET(Device), sizeof(DA), &DA);

		//!< コマンド発行 [Issue upload command]
		VERIFY_SUCCEEDED(CL->Reset(CA, nullptr)); {
			//!<【Pass0】
			VertexBuffers[0].PopulateCopyCommand(CL, TotalSizeOf(Vertices), COM_PTR_GET(UploadPass0Vertex.Resource));
			VertexBuffers[1].PopulateCopyCommand(CL, TotalSizeOf(Normals), COM_PTR_GET(UploadPass0Normal.Resource));
			IndexBuffers[0].PopulateCopyCommand(CL, TotalSizeOf(Indices), COM_PTR_GET(UploadPass0Index.Resource));
			IndirectBuffers[0].PopulateCopyCommand(CL, sizeof(DIA), COM_PTR_GET(UploadPass0Indirect.Resource));

			//!<【Pass1】
			IndirectBuffers[1].PopulateCopyCommand(CL, sizeof(DA), COM_PTR_GET(UploadPass1Indirect.Resource));
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
	}
	virtual void CreateStaticSampler() override {
		//!<【Pass1】
		CreateStaticSampler_LinearWrap(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
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
			DX::SerializeRootSignature(Blob, 
				{
					//!< CBV -> SetGraphicsRootDescriptorTable(0,..)
					D3D12_ROOT_PARAMETER1({
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
						.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1({.NumDescriptorRanges = static_cast<UINT>(std::size(DRs_CBV)), .pDescriptorRanges = std::data(DRs_CBV) }),
						.ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY
					}),
				}, 
				{
				}, 
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | SHADER_ROOT_ACCESS_GS);
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
		const std::vector IEDs = {
			D3D12_INPUT_ELEMENT_DESC({ .SemanticName = "POSITION", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 0, .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, .InstanceDataStepRate = 0 }),
			D3D12_INPUT_ELEMENT_DESC({ .SemanticName = "NORMAL", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 1, .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, .InstanceDataStepRate = 0 }),
		};
		std::vector<COM_PTR<ID3DBlob>> SBs_Pass0;
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "MeshPass0DX.vs.cso").wstring()), COM_PTR_PUT(SBs_Pass0.emplace_back())));
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "MeshPass0DX.ps.cso").wstring()), COM_PTR_PUT(SBs_Pass0.emplace_back())));
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "MeshPass0DX.gs.cso").wstring()), COM_PTR_PUT(SBs_Pass0.emplace_back())));
		const std::array SBCsPass0 = {
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs_Pass0[0]->GetBufferPointer(), .BytecodeLength = SBs_Pass0[0]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs_Pass0[1]->GetBufferPointer(), .BytecodeLength = SBs_Pass0[1]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs_Pass0[2]->GetBufferPointer(), .BytecodeLength = SBs_Pass0[2]->GetBufferSize() }),
		};
		CreatePipelineState_VsPsGs_Input(PipelineStates[0], COM_PTR_GET(RootSignatures[0]), D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, RD, TRUE, IEDs, SBCsPass0);

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
			const auto DescCount = GetViewportDrawCount();

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
	void PopulateBundleCommandList_Pass0() {
		const auto BCA = COM_PTR_GET(BundleCommandAllocators[0]);
		const auto BCL = COM_PTR_GET(BundleCommandLists[0]);
		const auto PS = COM_PTR_GET(PipelineStates[0]);
		const auto RS = COM_PTR_GET(RootSignatures[0]);
		VERIFY_SUCCEEDED(BCL->Reset(BCA, PS));
		{
			BCL->SetGraphicsRootSignature(RS);

			BCL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			const std::array VBVs = { VertexBuffers[0].View, VertexBuffers[1].View };
			BCL->IASetVertexBuffers(0, static_cast<UINT>(std::size(VBVs)), std::data(VBVs));
			BCL->IASetIndexBuffer(&IndexBuffers[0].View);

			const auto IB = IndirectBuffers[0];
			BCL->ExecuteIndirect(COM_PTR_GET(IB.CommandSignature), 1, COM_PTR_GET(IB.Resource), 0, nullptr, 0);
		}
		VERIFY_SUCCEEDED(BCL->Close());
	}
	void PopulateBundleCommandList_Pass1() {
		const auto BCA = COM_PTR_GET(BundleCommandAllocators[0]);
		const auto BCL = COM_PTR_GET(BundleCommandLists[1]);
		const auto PS = COM_PTR_GET(PipelineStates[1]);
		const auto RS = COM_PTR_GET(RootSignatures[1]);
		VERIFY_SUCCEEDED(BCL->Reset(BCA, PS));
		{
			BCL->SetGraphicsRootSignature(RS);

			BCL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			
			const auto IB = IndirectBuffers[0];
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

					//!< キルトパターン描画 (ビューポート同時描画数に制限がある為、要複数回実行) [Because viewport max is 16, need to draw few times]
					for (uint32_t j = 0; j < GetViewportDrawCount(); ++j) {
						const auto Offset = GetViewportSetOffset(j);
						const auto Count = GetViewportSetCount(j);

						DCL->RSSetViewports(Count, &QuiltViewports[Offset]);
						DCL->RSSetScissorRects(Count, &QuiltScissorRects[Offset]);

						//!< オフセット毎のハンドルを使用 [Use handle on each offset]
						DCL->SetGraphicsRootDescriptorTable(0, Handle[j]); //!< CBV

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
		DirectX::XMFLOAT4X4 ViewProjection[64]; //!< 64 もあれば十分 [64 will be enough]
	};
	VIEW_PROJECTION_BUFFER ViewProjectionBuffer;
};