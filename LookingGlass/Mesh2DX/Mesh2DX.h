#pragma once

#include "resource.h"

//#define USE_GLTF
#define USE_FBX
#include "../HoloDX.h"

#ifdef USE_GLTF
class Mesh2DX : public HoloGLTFDX
#else
class Mesh2DX : public HoloFBXDX
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
		UpdateWorldBuffer();

		DX::OnCreate(hWnd, hInstance, Title);
	}
	virtual void DrawFrame(const UINT i) override {
		UpdateWorldBuffer();
		CopyToUploadResource(COM_PTR_GET(ConstantBuffers[i].Resource), RoundUp256(sizeof(WorldBuffer)), &WorldBuffer);
	}

	virtual void CreateCommandList() override {
		DX::CreateCommandList();
		DX::CreateBundleCommandList(std::size(SwapChainBackBuffers) + 1);
	}

	virtual void CreateGeometry() override {
		const auto CA = COM_PTR_GET(DirectCommandAllocators[0]);
		const auto CL = COM_PTR_GET(DirectCommandLists[0]);
		const auto GCQ = COM_PTR_GET(GraphicsCommandQueue);
		const auto GF = COM_PTR_GET(GraphicsFence);

		//Load(std::filesystem::path("..") / "Asset" / "bunny.FBX");
		Load(std::filesystem::path("..") / "Asset" / "dragon.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "happy_vrip.FBX");

		//Load(std::filesystem::path("..") / "Asset" / "bunny.glb");
		//Load(std::filesystem::path("..") / "Asset" / "dragon.glb");
		//Load(std::filesystem::path("..") / "Asset" / "happy_vrip.glb");

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
			.InstanceCount = GetInstanceCount(),
			.StartIndexLocation = 0,
			.BaseVertexLocation = 0,
			.StartInstanceLocation = 0
		};
		LOG(std::data(std::format("InstanceCount = {}\n", DIA.InstanceCount)));
		IndirectBuffers.emplace_back().Create(COM_PTR_GET(Device), DIA);
		UploadResource UploadPass0Indirect;
		UploadPass0Indirect.Create(COM_PTR_GET(Device), sizeof(DIA), &DIA);

		constexpr D3D12_DRAW_ARGUMENTS DA = {
			.VertexCountPerInstance = 4,
			.InstanceCount = 1,
			.StartVertexLocation = 0,
			.StartInstanceLocation = 0
		};
		IndirectBuffers.emplace_back().Create(COM_PTR_GET(Device), DA).ExecuteCopyCommand(COM_PTR_GET(Device), CA, CL, GCQ, GF, sizeof(DA), &DA);
		UploadResource UploadPass1Indirect;
		UploadPass1Indirect.Create(COM_PTR_GET(Device), sizeof(DA), &DA);

		VERIFY_SUCCEEDED(CL->Reset(CA, nullptr)); {
			VertexBuffers[0].PopulateCopyCommand(CL, TotalSizeOf(Vertices), COM_PTR_GET(UploadPass0Vertex.Resource));
			VertexBuffers[1].PopulateCopyCommand(CL, TotalSizeOf(Normals), COM_PTR_GET(UploadPass0Normal.Resource));
			IndexBuffers[0].PopulateCopyCommand(CL, TotalSizeOf(Indices), COM_PTR_GET(UploadPass0Index.Resource));
			IndirectBuffers[0].PopulateCopyCommand(CL, sizeof(DIA), COM_PTR_GET(UploadPass0Indirect.Resource));

			IndirectBuffers[1].PopulateCopyCommand(CL, sizeof(DA), COM_PTR_GET(UploadPass1Indirect.Resource));
		} VERIFY_SUCCEEDED(CL->Close());
		DX::ExecuteAndWait(GCQ, CL, COM_PTR_GET(GraphicsFence));
	}
	virtual void CreateConstantBuffer() override {
		for (const auto& i : SwapChainBackBuffers) {
			ConstantBuffers.emplace_back().Create(COM_PTR_GET(Device), sizeof(WorldBuffer));
			CopyToUploadResource(COM_PTR_GET(ConstantBuffers.back().Resource), RoundUp256(sizeof(WorldBuffer)), &WorldBuffer);
		}
		for (const auto& i : SwapChainBackBuffers) {
			ConstantBuffers.emplace_back().Create(COM_PTR_GET(Device), sizeof(ViewProjectionBuffer));
			CopyToUploadResource(COM_PTR_GET(ConstantBuffers.back().Resource), RoundUp256(sizeof(ViewProjectionBuffer)), &ViewProjectionBuffer);
		}

		ConstantBuffers.emplace_back().Create(COM_PTR_GET(Device), sizeof(LenticularBuffer));
		CopyToUploadResource(COM_PTR_GET(ConstantBuffers.back().Resource), RoundUp256(sizeof(LenticularBuffer)), &LenticularBuffer);
	}
	virtual void CreateTexture() override {
		CreateTexture_Render(QuiltX, QuiltY);
		CreateTexture_Depth(QuiltX, QuiltY);
	}
	virtual void CreateStaticSampler() override {
		CreateStaticSampler_LinearWrap(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	}
	virtual void CreateRootSignature() override {
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
					D3D12_ROOT_PARAMETER1({
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
						.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1({.NumDescriptorRanges = static_cast<UINT>(std::size(DRs_CBV)), .pDescriptorRanges = std::data(DRs_CBV) }),
						.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX
					}),
					D3D12_ROOT_PARAMETER1({
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
						.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1({.NumDescriptorRanges = static_cast<UINT>(std::size(DRs_CBV)), .pDescriptorRanges = std::data(DRs_CBV) }),
						.ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY
					}),
				},
				{
				},
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | SHADER_ROOT_ACCESS_VS_GS);
			VERIFY_SUCCEEDED(Device->CreateRootSignature(0, Blob->GetBufferPointer(), Blob->GetBufferSize(), COM_PTR_UUIDOF_PUTVOID(RootSignatures.emplace_back())));
		}
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

		const std::vector IEDs = {
			D3D12_INPUT_ELEMENT_DESC({.SemanticName = "POSITION", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 0, .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, .InstanceDataStepRate = 0 }),
			D3D12_INPUT_ELEMENT_DESC({.SemanticName = "NORMAL", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 1, .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, .InstanceDataStepRate = 0 }),
		};
		std::vector<COM_PTR<ID3DBlob>> SBs_Pass0;
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "Mesh2Pass0DX.vs.cso").wstring()), COM_PTR_PUT(SBs_Pass0.emplace_back())));
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "Mesh2Pass0DX.ps.cso").wstring()), COM_PTR_PUT(SBs_Pass0.emplace_back())));
		VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "Mesh2Pass0DX.gs.cso").wstring()), COM_PTR_PUT(SBs_Pass0.emplace_back())));
		const std::array SBCsPass0 = {
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs_Pass0[0]->GetBufferPointer(), .BytecodeLength = SBs_Pass0[0]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs_Pass0[1]->GetBufferPointer(), .BytecodeLength = SBs_Pass0[1]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBs_Pass0[2]->GetBufferPointer(), .BytecodeLength = SBs_Pass0[2]->GetBufferSize() }),
		};
		CreatePipelineState_VsPsGs_Input(PipelineStates[0], COM_PTR_GET(RootSignatures[0]), D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, RD, TRUE, IEDs, SBCsPass0);

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

			{
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
				{
					auto& Desc = DsvDescs.emplace_back();
					auto& Heap = Desc.first;
					auto& Handle = Desc.second;
					const D3D12_DESCRIPTOR_HEAP_DESC DHD = {
						.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
						.NumDescriptors = 1,
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
		}
		{
			const auto BackBufferCount = std::size(SwapChainBackBuffers);
			const auto DescCount = 2;

			const auto CB0Index = 0;
			const auto CB1Index = BackBufferCount;

			for (auto i = 0; i < BackBufferCount; ++i) {
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

				{
					const auto& CB = ConstantBuffers[CB0Index + i];
					const D3D12_CONSTANT_BUFFER_VIEW_DESC CBVD = {
						.BufferLocation = CB.Resource->GetGPUVirtualAddress(),
						.SizeInBytes = static_cast<UINT>(CB.Resource->GetDesc().Width)
					};
					Device->CreateConstantBufferView(&CBVD, CDH);
					Handle.emplace_back(GDH);
					CDH.ptr += IncSize;
					GDH.ptr += IncSize;
				}
				{
					const auto& CB = ConstantBuffers[CB1Index + i];
					const auto DynamicOffset = GetViewportMax() * sizeof(ViewProjectionBuffer.ViewProjection[0]);
					for (UINT i = 0; i < GetViewportDrawCount(); ++i) {
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
	}
	void CreateDescriptor_Pass1() {
		const auto DescCount = 1;

		const auto CBIndex = std::size(SwapChainBackBuffers) * 2;

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

			{
				const auto& Tex = RenderTextures[0];
				Device->CreateShaderResourceView(COM_PTR_GET(Tex.Resource), &Tex.SRV, CDH);
				Handle.emplace_back(GDH);
				CDH.ptr += IncSize;
				GDH.ptr += IncSize;
			}
			{
				const auto& CB = ConstantBuffers[CBIndex];
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
	virtual void CreateDescriptor() override {
		CreateDescriptor_Pass0();
		CreateDescriptor_Pass1();
	}
	virtual void CreateViewport(const FLOAT Width, const FLOAT Height, const FLOAT MinDepth = 0.0f, const FLOAT MaxDepth = 1.0f) override {
		HoloViewsDX::CreateViewportScissor(MinDepth, MaxDepth);
		DX::CreateViewport(Width, Height, MinDepth, MaxDepth);
	}
	void PopulateBundleCommandList_Pass0(const size_t i) {
		const auto BCA = COM_PTR_GET(BundleCommandAllocators[0]);
		const auto BCL = COM_PTR_GET(BundleCommandLists[i]);
		const auto PS = COM_PTR_GET(PipelineStates[0]);
		VERIFY_SUCCEEDED(BCL->Reset(BCA, PS));
		{
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
		const auto BCL = COM_PTR_GET(BundleCommandLists[std::size(SwapChainBackBuffers)]);
		const auto PS = COM_PTR_GET(PipelineStates[1]);
		VERIFY_SUCCEEDED(BCL->Reset(BCA, PS));
		{
			BCL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			const auto IB = IndirectBuffers[0];
			BCL->ExecuteIndirect(COM_PTR_GET(IB.CommandSignature), 1, COM_PTR_GET(IB.Resource), 0, nullptr, 0);
		}
		VERIFY_SUCCEEDED(BCL->Close());
	}
	virtual void PopulateBundleCommandList(const size_t i) override {
		PopulateBundleCommandList_Pass0(i);

		if (0 == i) {
			PopulateBundleCommandList_Pass1();
		}
	}
	virtual void PopulateCommandList(const size_t i) override {
		const auto DCL = COM_PTR_GET(DirectCommandLists[i]);
		const auto DCA = COM_PTR_GET(DirectCommandAllocators[0]);
		VERIFY_SUCCEEDED(DCL->Reset(DCA, nullptr));
		{
			{
				const auto RS = COM_PTR_GET(RootSignatures[0]);
				const auto BCL = COM_PTR_GET(BundleCommandLists[i]);

				DCL->SetGraphicsRootSignature(RS);

				{
					const auto& DescRTV = RtvDescs[0];
					const auto& DescDSV = DsvDescs[0];
					const auto& HandleRTV = DescRTV.second;
					const auto& HandleDSV = DescDSV.second;

					constexpr std::array<D3D12_RECT, 0> Rects = {};
					DCL->ClearRenderTargetView(HandleRTV[0], DirectX::Colors::SkyBlue, static_cast<UINT>(std::size(Rects)), std::data(Rects));
					DCL->ClearDepthStencilView(HandleDSV[0], D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, static_cast<UINT>(std::size(Rects)), std::data(Rects));

					const std::array CHs = { HandleRTV[0]};
					DCL->OMSetRenderTargets(static_cast<UINT>(std::size(CHs)), std::data(CHs), FALSE, &HandleDSV[0]);
				}
				{
					const auto& Desc = CbvSrvUavDescs[i];
					const auto& Heap = Desc.first;
					const auto& Handle = Desc.second;
					const std::array DHs = { COM_PTR_GET(Heap) };
					DCL->SetDescriptorHeaps(static_cast<UINT>(std::size(DHs)), std::data(DHs));

					DCL->SetGraphicsRootDescriptorTable(0, Handle[0]);

					for (uint32_t j = 0; j < GetViewportDrawCount(); ++j) {
						const auto Offset = GetViewportSetOffset(j);
						const auto Count = GetViewportSetCount(j);

						DCL->RSSetViewports(Count, &QuiltViewports[Offset]);
						DCL->RSSetScissorRects(Count, &QuiltScissorRects[Offset]);

						DCL->SetGraphicsRootDescriptorTable(1, Handle[j + 1]);

						DCL->ExecuteBundle(BCL);
					}
				}
			}

			const auto SCR = COM_PTR_GET(SwapChainBackBuffers[i].Resource);
			const auto RT = COM_PTR_GET(RenderTextures[0].Resource);
			ResourceBarrier2(DCL,
				SCR, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET,
				RT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			{
				const auto RS = COM_PTR_GET(RootSignatures[1]);
				const auto BCL = COM_PTR_GET(BundleCommandLists[std::size(SwapChainBackBuffers)]);

				DCL->SetGraphicsRootSignature(RS);

				DCL->RSSetViewports(static_cast<UINT>(std::size(Viewports)), std::data(Viewports));
				DCL->RSSetScissorRects(static_cast<UINT>(std::size(ScissorRects)), std::data(ScissorRects));

				const std::array CHs = { SwapChainBackBuffers[i].Handle };
				DCL->OMSetRenderTargets(static_cast<UINT>(std::size(CHs)), std::data(CHs), FALSE, nullptr);

				{
					const auto& Desc = CbvSrvUavDescs[std::size(SwapChainBackBuffers)];
					const auto& Heap = Desc.first;
					const auto& Handle = Desc.second;

					const std::array DHs = { COM_PTR_GET(Heap) };
					DCL->SetDescriptorHeaps(static_cast<UINT>(std::size(DHs)), std::data(DHs));
					DCL->SetGraphicsRootDescriptorTable(0, Handle[0]);
					DCL->SetGraphicsRootDescriptorTable(1, Handle[1]);
				}

				DCL->ExecuteBundle(BCL);
			}

			ResourceBarrier2(DCL,
				SCR, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT,
				RT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}
		VERIFY_SUCCEEDED(DCL->Close());
	}
	virtual void UpdateViewProjectionBuffer() {
		const auto Count = (std::min)(static_cast<size_t>(LenticularBuffer.TileX * LenticularBuffer.TileY), _countof(ViewProjectionBuffer.ViewProjection));
		for (auto i = 0; i < Count; ++i) {
			DirectX::XMStoreFloat4x4(&ViewProjectionBuffer.ViewProjection[i].View, ViewMatrices[i]);
			DirectX::XMStoreFloat4x4(&ViewProjectionBuffer.ViewProjection[i].Projection, ProjectionMatrices[i]);
		}
	}
	virtual void UpdateWorldBuffer() {
		Angle += 1.0f;
		while (Angle > 360.0f) { Angle -= 360.0f; }
		while (Angle < 0.0f) { Angle += 360.0f; }

		const auto Count = GetInstanceCount();
		constexpr auto Radius = 2.0f;
		constexpr auto Height = 10.0f;
		const auto OffsetY = Height / static_cast<float>(Count);
		for (UINT i = 0; i < Count; ++i) {
			const auto Index = static_cast<float>(i);
			const auto Radian = DirectX::XMConvertToRadians(Index * 90.0f);
			const auto X = Radius * cos(Radian);
			const auto Y = OffsetY * (Index - static_cast<float>(Count) * 0.5f);
			const auto Z = Radius * sin(Radian);

			//const auto RotL = DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(i * 45.0f + Angle));
			const auto RotL = DirectX::XMMatrixIdentity();
			const auto RotG = DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(i * 45.0f + Angle));
			DirectX::XMStoreFloat4x4(&WorldBuffer.World[i], RotL * DirectX::XMMatrixTranslation(X, Y, Z) * RotG);
		}
	}

	UINT GetInstanceCount() const { return (std::min)(InstanceCount, static_cast<UINT>(_countof(WorldBuffer.World))); }

	virtual float GetMeshScale() const override { return 3.0f; }

protected:
	struct VIEW_PROJECTION {
		DirectX::XMFLOAT4X4 View;
		DirectX::XMFLOAT4X4 Projection;
	};
	struct VIEW_PROJECTION_BUFFER {
		VIEW_PROJECTION ViewProjection[64];
	};
	VIEW_PROJECTION_BUFFER ViewProjectionBuffer;

	struct WORLD_BUFFER {
		DirectX::XMFLOAT4X4 World[16];
	};
	WORLD_BUFFER WorldBuffer;

	float Angle = 0.0f;

	const UINT InstanceCount = 16;
};