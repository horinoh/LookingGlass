#pragma once

#include "resource.h"

#include "../DX.h"
#include "../Holo.h"
#include "../FBX.h"

class MeshDX : public DX, public Holo, public Fbx
{
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		CreateProjectionMatrices();
		{
			const auto Pos = DirectX::XMVectorSet(0.0f, 0.0f, 3.0f, 1.0f);
			const auto Tag = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			const auto Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
			View = DirectX::XMMatrixLookAtRH(Pos, Tag, Up);
		}
		CreateViewMatrices();
		updateViewProjectionBuffer();

		DX::OnCreate(hWnd, hInstance, Title);
	}

	DirectX::XMFLOAT3 ToFloat3(const FbxVector4& rhs) { return DirectX::XMFLOAT3(static_cast<FLOAT>(rhs[0]), static_cast<FLOAT>(rhs[1]), static_cast<FLOAT>(rhs[2])); }
	virtual void Process(FbxMesh* Mesh) override {
		Fbx::Process(Mesh);

		for (auto i = 0; i < Mesh->GetPolygonCount(); ++i) {
			for (auto j = 0; j < Mesh->GetPolygonSize(i); ++j) {
				Indices.emplace_back(i * Mesh->GetPolygonSize(i) + j); //!< RH
				Vertices.emplace_back(ToFloat3(Mesh->GetControlPoints()[Mesh->GetPolygonVertex(i, j)]));
			}
		}
		AdjustScale(Vertices, 1.0f);

		FbxArray<FbxVector4> Nrms;
		Mesh->GetPolygonVertexNormals(Nrms);
		for (auto i = 0; i < Nrms.Size(); ++i) {
			Normals.emplace_back(ToFloat3(Nrms[i]));
		}
	}

	virtual void CreateCommandList() override {
		DX::CreateCommandList();

		//!< �y�p�X1�z�o���h���R�}���h���X�g
		const auto BCA = BundleCommandAllocators[0];
		DXGI_SWAP_CHAIN_DESC1 SCD;
		SwapChain->GetDesc1(&SCD);
		for (UINT i = 0; i < SCD.BufferCount; ++i) {
			VERIFY_SUCCEEDED(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, COM_PTR_GET(BCA), nullptr, COM_PTR_UUIDOF_PUTVOID(BundleCommandLists.emplace_back())));
			VERIFY_SUCCEEDED(BundleCommandLists.back()->Close());
		}
	}

	virtual void CreateGeometry() override {
		const auto CA = COM_PTR_GET(DirectCommandAllocators[0]);
		const auto CL = COM_PTR_GET(DirectCommandLists[0]);
		const auto GCQ = COM_PTR_GET(GraphicsCommandQueue);
		const auto GF = COM_PTR_GET(GraphicsFence);

		//!<�y�p�X0�z���b�V���`��p
		Load(std::filesystem::path("..") / "Asset" / "bunny.FBX");
		VertexBuffers.emplace_back().Create(COM_PTR_GET(Device), TotalSizeOf(Vertices), sizeof(Vertices[0]));
		UploadResource UploadPass0Vertex;
		UploadPass0Vertex.Create(COM_PTR_GET(Device), TotalSizeOf(Vertices), data(Vertices));

		VertexBuffers.emplace_back().Create(COM_PTR_GET(Device), TotalSizeOf(Normals), sizeof(Normals[0]));
		UploadResource UploadPass0Normal;
		UploadPass0Normal.Create(COM_PTR_GET(Device), TotalSizeOf(Normals), data(Normals));

		IndexBuffers.emplace_back().Create(COM_PTR_GET(Device), TotalSizeOf(Indices), DXGI_FORMAT_R32_UINT);
		UploadResource UploadPass0Index;
		UploadPass0Index.Create(COM_PTR_GET(Device), TotalSizeOf(Indices), data(Indices));

		const D3D12_DRAW_INDEXED_ARGUMENTS DIA = { 
			.IndexCountPerInstance = static_cast<UINT32>(size(Indices)), 
			.InstanceCount = 1, 
			.StartIndexLocation = 0, 
			.BaseVertexLocation = 0, 
			.StartInstanceLocation = 0 
		};
		IndirectBuffers.emplace_back().Create(COM_PTR_GET(Device), DIA);
		UploadResource UploadPass0Indirect;
		UploadPass0Indirect.Create(COM_PTR_GET(Device), sizeof(DIA), &DIA);


		//!<�y�p�X1�z�t���X�N���[���`��p
		constexpr D3D12_DRAW_ARGUMENTS DA = {
			.VertexCountPerInstance = 4, 
			.InstanceCount = 1, 
			.StartVertexLocation = 0, 
			.StartInstanceLocation = 0
		};
		IndirectBuffers.emplace_back().Create(COM_PTR_GET(Device), DA).ExecuteCopyCommand(COM_PTR_GET(Device), CA, CL, GCQ, GF, sizeof(DA), &DA);
		UploadResource UploadPass1Indirect;
		UploadPass1Indirect.Create(COM_PTR_GET(Device), sizeof(DA), &DA);

		//!< �R�}���h���s
		VERIFY_SUCCEEDED(CL->Reset(CA, nullptr)); {
			//!< �y�p�X0�z
			VertexBuffers[0].PopulateCopyCommand(CL, TotalSizeOf(Vertices), COM_PTR_GET(UploadPass0Vertex.Resource));
			VertexBuffers[1].PopulateCopyCommand(CL, TotalSizeOf(Normals), COM_PTR_GET(UploadPass0Normal.Resource));
			IndexBuffers[0].PopulateCopyCommand(CL, TotalSizeOf(Indices), COM_PTR_GET(UploadPass0Index.Resource));
			IndirectBuffers[0].PopulateCopyCommand(CL, sizeof(DIA), COM_PTR_GET(UploadPass0Indirect.Resource));

			//!< �y�p�X1�z
			IndirectBuffers[1].PopulateCopyCommand(CL, sizeof(DA), COM_PTR_GET(UploadPass1Indirect.Resource));
		} VERIFY_SUCCEEDED(CL->Close());
		DX::ExecuteAndWait(GCQ, CL, COM_PTR_GET(GraphicsFence));
	}
	virtual void CreateConstantBuffer() override {
		//!<�y�p�X0�z
		ConstantBuffers.emplace_back().Create(COM_PTR_GET(Device), sizeof(ViewProjectionBuffer));
		CopyToUploadResource(COM_PTR_GET(ConstantBuffers[0].Resource), RoundUp256(sizeof(ViewProjectionBuffer)), &ViewProjectionBuffer);

		//!<�y�p�X1�z
		ConstantBuffers.emplace_back().Create(COM_PTR_GET(Device), sizeof(*LenticularBuffer));
		CopyToUploadResource(COM_PTR_GET(ConstantBuffers[1].Resource), RoundUp256(sizeof(*LenticularBuffer)), LenticularBuffer);

	}
	virtual void CreateTexture() override {
		//!<�y�p�X0�z�����_�[�^�[�Q�b�g�A�f�v�X
		CreateTexture_Render(QuiltWidth, QuiltHeight);
		CreateTexture_Depth(QuiltWidth, QuiltHeight);
	}
	virtual void CreateStaticSampler() override {
		//!<�y�p�X1�z
		CreateStaticSampler_LinearWrap(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	}
	virtual void CreateRootSignature() override {
		//!<�y�p�X0�z
		{
			COM_PTR<ID3DBlob> Blob;
			constexpr std::array DRs_Cbv = {
				D3D12_DESCRIPTOR_RANGE({.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV, .NumDescriptors = 1, .BaseShaderRegister = 0, .RegisterSpace = 0, .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND })
			};
			DX::SerializeRootSignature(Blob, 
				{
					//!< CBV
					D3D12_ROOT_PARAMETER({
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
						.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE({.NumDescriptorRanges = static_cast<UINT>(size(DRs_Cbv)), .pDescriptorRanges = data(DRs_Cbv) }),
						.ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY
					}),
					//!< ROOT_CONSTANTS
					D3D12_ROOT_PARAMETER({
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
						.Constants = D3D12_ROOT_CONSTANTS({.ShaderRegister = 1, .RegisterSpace = 0, .Num32BitValues = 1 }),
						.ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY
					}),
				}, 
				{
				}, 
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | SHADER_ROOT_ACCESS_GS);
			VERIFY_SUCCEEDED(Device->CreateRootSignature(0, Blob->GetBufferPointer(), Blob->GetBufferSize(), COM_PTR_UUIDOF_PUTVOID(RootSignatures.emplace_back())));
		}
		//!<�y�p�X1�z
		{
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

		//!< �y�p�X0�z
		const std::vector IEDs = {
			D3D12_INPUT_ELEMENT_DESC({ .SemanticName = "POSITION", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 0, .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, .InstanceDataStepRate = 0 }),
			D3D12_INPUT_ELEMENT_DESC({ .SemanticName = "NORMAL", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 1, .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, .InstanceDataStepRate = 0 }),
		};
		std::vector<COM_PTR<ID3DBlob>> SBsPass0;
		VERIFY_SUCCEEDED(D3DReadFileToBlob(data((std::filesystem::path(".") / "MeshPass0DX.vs.cso").wstring()), COM_PTR_PUT(SBsPass0.emplace_back())));
		VERIFY_SUCCEEDED(D3DReadFileToBlob(data((std::filesystem::path(".") / "MeshPass0DX.ps.cso").wstring()), COM_PTR_PUT(SBsPass0.emplace_back())));
		VERIFY_SUCCEEDED(D3DReadFileToBlob(data((std::filesystem::path(".") / "MeshPass0DX.gs.cso").wstring()), COM_PTR_PUT(SBsPass0.emplace_back())));
		const std::array SBCsPass0 = {
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBsPass0[0]->GetBufferPointer(), .BytecodeLength = SBsPass0[0]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBsPass0[1]->GetBufferPointer(), .BytecodeLength = SBsPass0[1]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBsPass0[2]->GetBufferPointer(), .BytecodeLength = SBsPass0[2]->GetBufferSize() }),
		};
		CreatePipelineState_VsPsGs_Input(PipelineStates[0], COM_PTR_GET(RootSignatures[0]), D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, RD, TRUE, IEDs, SBCsPass0);

		//!< �y�p�X1�z
		std::vector<COM_PTR<ID3DBlob>> SBsPass1;
		VERIFY_SUCCEEDED(D3DReadFileToBlob(data((std::filesystem::path(".") / "MeshPass1DX.vs.cso").wstring()), COM_PTR_PUT(SBsPass1.emplace_back())));
		VERIFY_SUCCEEDED(D3DReadFileToBlob(data((std::filesystem::path(".") / "MeshPass1DX.ps.cso").wstring()), COM_PTR_PUT(SBsPass1.emplace_back())));
		const std::array SBCsPass1 = {
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBsPass1[0]->GetBufferPointer(), .BytecodeLength = SBsPass1[0]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBsPass1[1]->GetBufferPointer(), .BytecodeLength = SBsPass1[1]->GetBufferSize() }),
		};
		CreatePipelineState_VsPs(PipelineStates[1], COM_PTR_GET(RootSignatures[1]), D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, RD, FALSE, SBCsPass1);

		for (auto& i : Threads) { i.join(); }
		Threads.clear();
	}
	virtual void CreateDescriptor() override {
		//!<�y�p�X0�z
		{
			//!< �����_�[�^�[�Q�b�g�r���[
			{
				auto& Desc = RtvDescs.emplace_back();
				auto& Heap = Desc.first;
				auto& Handle = Desc.second;

				const D3D12_DESCRIPTOR_HEAP_DESC DHD = { .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV, .NumDescriptors = 1, .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, .NodeMask = 0 };
				VERIFY_SUCCEEDED(Device->CreateDescriptorHeap(&DHD, COM_PTR_UUIDOF_PUTVOID(Heap)));

				auto CDH = Heap->GetCPUDescriptorHandleForHeapStart();
				const auto IncSize = Device->GetDescriptorHandleIncrementSize(Heap->GetDesc().Type);

				const auto& Tex = RenderTextures[0];
				Device->CreateRenderTargetView(COM_PTR_GET(Tex.Resource), &Tex.RTV, CDH);
				Handle.emplace_back(CDH);
				CDH.ptr += IncSize;
			}
			//!< �f�v�X�X�e���V���r���[
			{
				auto& Desc = DsvDescs.emplace_back();
				auto& Heap = Desc.first;
				auto& Handle = Desc.second;

				const D3D12_DESCRIPTOR_HEAP_DESC DHD = { .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV, .NumDescriptors = 1, .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, .NodeMask = 0 };
				VERIFY_SUCCEEDED(Device->CreateDescriptorHeap(&DHD, COM_PTR_UUIDOF_PUTVOID(Heap)));

				auto CDH = Heap->GetCPUDescriptorHandleForHeapStart();
				const auto IncSize = Device->GetDescriptorHandleIncrementSize(Heap->GetDesc().Type);

				const auto& Tex = DepthTextures[0];
				Device->CreateDepthStencilView(COM_PTR_GET(Tex.Resource), &Tex.DSV, CDH);
				Handle.emplace_back(CDH);
				CDH.ptr += IncSize;
			}
			//!< �R���X�^���g�o�b�t�@�[�r���[
			{
				auto& Desc = CbvSrvUavDescs.emplace_back();
				auto& Heap = Desc.first;
				auto& Handle = Desc.second;

				const D3D12_DESCRIPTOR_HEAP_DESC DHD = { .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, .NumDescriptors = 1, .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, .NodeMask = 0 };
				VERIFY_SUCCEEDED(Device->CreateDescriptorHeap(&DHD, COM_PTR_UUIDOF_PUTVOID(Heap)));

				auto CDH = Heap->GetCPUDescriptorHandleForHeapStart();
				auto GDH = Heap->GetGPUDescriptorHandleForHeapStart();
				const auto IncSize = Device->GetDescriptorHandleIncrementSize(Heap->GetDesc().Type);

				const auto& CB = ConstantBuffers[0];
				const D3D12_CONSTANT_BUFFER_VIEW_DESC CBVD = { .BufferLocation = CB.Resource->GetGPUVirtualAddress(), .SizeInBytes = static_cast<UINT>(CB.Resource->GetDesc().Width) };
				Device->CreateConstantBufferView(&CBVD, CDH);
				Handle.emplace_back(GDH);
				CDH.ptr += IncSize;
				GDH.ptr += IncSize;
			}
		}

		//!<�y�p�X1�z
		{
			auto& Desc = CbvSrvUavDescs.emplace_back();
			auto& Heap = Desc.first;
			auto& Handle = Desc.second;

			const D3D12_DESCRIPTOR_HEAP_DESC DHD = { .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, .NumDescriptors = 1, .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, .NodeMask = 0 }; //!< SRV
			VERIFY_SUCCEEDED(Device->CreateDescriptorHeap(&DHD, COM_PTR_UUIDOF_PUTVOID(Heap)));

			auto CDH = Heap->GetCPUDescriptorHandleForHeapStart();
			auto GDH = Heap->GetGPUDescriptorHandleForHeapStart();
			const auto IncSize = Device->GetDescriptorHandleIncrementSize(Heap->GetDesc().Type);

			//!< �V�F�[�_���\�[�X�r���[
			{
				const auto& Tex = RenderTextures[0];
				Device->CreateShaderResourceView(COM_PTR_GET(Tex.Resource), &Tex.SRV, CDH);
				Handle.emplace_back(GDH);
				CDH.ptr += IncSize;
				GDH.ptr += IncSize;
			}
			//!< �R���X�^���g�o�b�t�@�[�r���[
			{
				const auto& CB = ConstantBuffers[1];
				const D3D12_CONSTANT_BUFFER_VIEW_DESC CBVD = { .BufferLocation = CB.Resource->GetGPUVirtualAddress(), .SizeInBytes = static_cast<UINT>(CB.Resource->GetDesc().Width) };
				Device->CreateConstantBufferView(&CBVD, CDH);
				Handle.emplace_back(GDH);
				CDH.ptr += IncSize;
				GDH.ptr += IncSize;
			}
		}
	}
	virtual void CreateViewport(const FLOAT Width, const FLOAT Height, const FLOAT MinDepth = 0.0f, const FLOAT MaxDepth = 1.0f) {
		D3D12_FEATURE_DATA_D3D12_OPTIONS3 FDO3;
		VERIFY_SUCCEEDED(Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, reinterpret_cast<void*>(&FDO3), sizeof(FDO3)));
		assert(D3D12_VIEW_INSTANCING_TIER_1 < FDO3.ViewInstancingTier && "");

		//!<�y�p�X0�z�L���g�����_�[�^�[�Q�b�g�𕪊�
		//const auto W = QuiltWidth / LenticularBuffer->Column, H = QuiltHeight / LenticularBuffer->Row;
		const auto W = Width / LenticularBuffer->Column, H = Height / LenticularBuffer->Row;
		for (auto i = 0; i < LenticularBuffer->Row; ++i) {
			//const auto Y = QuiltHeight - H * (i + 1);
			const auto Y = Height - H * (i + 1);
			for (auto j = 0; j < LenticularBuffer->Column; ++j) {
				const auto X = j * W;
				QuiltViewports.emplace_back(D3D12_VIEWPORT({ .TopLeftX = X, .TopLeftY = Y, .Width = W, .Height = H, .MinDepth = MinDepth, .MaxDepth = MaxDepth }));
				QuiltScissorRects.emplace_back(D3D12_RECT({ .left = static_cast<LONG>(X), .top = static_cast<LONG>(Y), .right = static_cast<LONG>(X + W), .bottom = static_cast<LONG>(Y + H) }));
			}
		}

		//!<�y�p�X1�z�X�N���[�����g�p
		DX::CreateViewport(Width, Height, MinDepth, MaxDepth);
	}
	virtual void PopulateCommandList(const size_t i) override {
		const auto PS = COM_PTR_GET(PipelineStates[0]);

		const auto DescIndex = 0;//i;

		//!<�y�p�X0�z�o���h���R�}���h���X�g
		const auto BCL = COM_PTR_GET(BundleCommandLists[i]);
		const auto BCA = COM_PTR_GET(BundleCommandAllocators[0]);
		VERIFY_SUCCEEDED(BCL->Reset(BCA, PS));
		{
			BCL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			const std::array VBVs = { VertexBuffers[0].View, VertexBuffers[1].View };
			BCL->IASetVertexBuffers(0, static_cast<UINT>(size(VBVs)), data(VBVs));
			BCL->IASetIndexBuffer(&IndexBuffers[0].View);

			const auto IB = IndirectBuffers[0];
			BCL->ExecuteIndirect(COM_PTR_GET(IB.CommandSignature), 1, COM_PTR_GET(IB.Resource), 0, nullptr, 0);
		}
		VERIFY_SUCCEEDED(BCL->Close());

		//!< �_�C���N�g�R�}���h���X�g
		const auto GCL = COM_PTR_GET(DirectCommandLists[i]);
		const auto CA = COM_PTR_GET(DirectCommandAllocators[0]);
		VERIFY_SUCCEEDED(GCL->Reset(CA, PS));
		{
			//!<�y�p�X0�z
			GCL->SetGraphicsRootSignature(COM_PTR_GET(RootSignatures[0]));
			GCL->SetGraphicsRoot32BitConstants(1, 1, &ViewportOffset, 0);

			const auto SCR = COM_PTR_GET(SwapChainResources[i]);
			ResourceBarrier(GCL, SCR, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			{
				//!< �f�X�N���v�^
				{
					//!< �����_�[�^�[�Q�b�g
					{
#if 0
						//!< �����_�[�e�N�X�`��
						const auto& HandleRTV = RtvDescs[0].second[0];
#else
						//!< �X���b�v�`�F�C��
						const auto& HandleRTV = SwapChainCPUHandles[i];
#endif
						//!< �f�v�X
						const auto& HandleDSV = DsvDescs[0].second[0];

						constexpr std::array<D3D12_RECT, 0> Rects = {};
						GCL->ClearRenderTargetView(HandleRTV, DirectX::Colors::SkyBlue, static_cast<UINT>(size(Rects)), data(Rects));
						GCL->ClearDepthStencilView(HandleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, static_cast<UINT>(size(Rects)), data(Rects));

						const std::array CHs = { HandleRTV };
						GCL->OMSetRenderTargets(static_cast<UINT>(size(CHs)), data(CHs), FALSE, &HandleDSV);
					}
					//!< �R���X�^���g�o�b�t�@
					{
						const auto& Desc = CbvSrvUavDescs[0];
						const auto& Heap = Desc.first;
						const auto& Handle = Desc.second;
						const std::array DHs = { COM_PTR_GET(Heap) };
						GCL->SetDescriptorHeaps(static_cast<UINT>(size(DHs)), data(DHs));
						GCL->SetGraphicsRootDescriptorTable(0, Handle[0]);
					}
				}

				//!< �L���g�p�^�[���`�� (�r���[�|�[�g�����`�搔�ɐ���������ׁA�v��������s)
				for (uint32_t j = 0; j < GetViewportDrawCount(); ++j) {
					const auto Offset = GetViewportSetOffset(j);
					const auto Count = GetViewportSetCount(j);

					//ViewportOffset = Offset;

					GCL->RSSetViewports(Count, &QuiltViewports[Offset]);
					GCL->RSSetScissorRects(Count, &QuiltScissorRects[Offset]);

					GCL->ExecuteBundle(BCL);
				}
			}
			ResourceBarrier(GCL, SCR, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

			//!<�y�p�X1�z#TODO
		}
		VERIFY_SUCCEEDED(GCL->Close());
	}

	virtual uint32_t GetViewportMax() const override { 
		return D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; 
	}
	virtual void CreateProjectionMatrix(const int i) override {
		if (-1 == i) { ProjectionMatrices.clear(); return; }

		//!< ���E�����ɂ���Ă���p�x(���W�A��)
		const auto OffsetAngle = (static_cast<float>(i) / (LenticularBuffer->Column * LenticularBuffer->Row - 1.0f) - 0.5f) * ViewCone;
		//!< ���E�����ɂ���Ă��鋗��
		const auto OffsetX = CameraDistance * std::tan(OffsetAngle);

		auto Prj = DirectX::XMMatrixPerspectiveFovRH(Fov, LenticularBuffer->DisplayAspect, 0.1f, 100.0f);
		Prj.r[2].m128_f32[0] += OffsetX / (CameraSize * LenticularBuffer->DisplayAspect);

		ProjectionMatrices.emplace_back(Prj);
	}
	virtual void CreateViewMatrix(const int i) override {
		if (-1 == i) { ViewMatrices.clear(); return; }

		const auto OffsetAngle = (static_cast<float>(i) / (LenticularBuffer->Column * LenticularBuffer->Row - 1.0f) - 0.5f) * ViewCone;
		const auto OffsetX = CameraDistance * std::tan(OffsetAngle);

		const auto OffsetLocal = DirectX::XMVector4Transform(DirectX::XMVectorSet(OffsetX, 0.0f, CameraDistance, 1.0f), View);
		ViewMatrices.emplace_back(View * DirectX::XMMatrixTranslationFromVector(OffsetLocal));

	}
	virtual void updateViewProjectionBuffer() {
		const auto ColRow = static_cast<int>(LenticularBuffer->Column * LenticularBuffer->Row);
		for (auto i = 0; i < ColRow; ++i) {
			DirectX::XMStoreFloat4x4(&ViewProjectionBuffer.ViewProjection[i], ViewMatrices[i] * ProjectionMatrices[i]);
		}
	}

protected:
	std::vector<UINT32> Indices;
	std::vector<DirectX::XMFLOAT3> Vertices;
	std::vector<DirectX::XMFLOAT3> Normals;

	std::vector<D3D12_VIEWPORT> QuiltViewports;
	std::vector<D3D12_RECT> QuiltScissorRects;

	std::vector<DirectX::XMMATRIX> ProjectionMatrices;
	DirectX::XMMATRIX View;
	std::vector<DirectX::XMMATRIX> ViewMatrices;

	struct VIEW_PROJECTION_BUFFER {
		DirectX::XMFLOAT4X4 ViewProjection[64];
	};
	VIEW_PROJECTION_BUFFER ViewProjectionBuffer;

	struct WORLD_BUFFER {
		DirectX::XMFLOAT4X4 World;
	};
	WORLD_BUFFER WorldBuffer;

	uint32_t ViewportOffset = 0;
};