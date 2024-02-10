#pragma once

#include "resource.h"

//#define USE_GLTF
#include "../DX.h"
#include "../Holo.h"

#ifdef USE_GLTF
#include "../GltfSDK.h"
class MeshDX : public DX, public Holo, public Gltf::SDK
#else
#include "../FBX.h"
class MeshDX : public DX, public Holo, public Fbx
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

#ifdef USE_GLTF
	virtual void Process() override {
		for (const auto& i : Document.meshes.Elements()) {
			for (const auto& j : i.primitives) {
				switch (j.mode)
				{
				case Microsoft::glTF::MeshMode::MESH_TRIANGLES:
					break;
				default:
					break;
				}

				if (empty(Indices)) {
					if (Document.accessors.Has(j.indicesAccessorId)) {
						const auto& Accessor = Document.accessors.Get(j.indicesAccessorId);
						switch (Accessor.componentType)
						{
						case Microsoft::glTF::ComponentType::COMPONENT_UNSIGNED_SHORT:
							switch (Accessor.type)
							{
							case Microsoft::glTF::AccessorType::TYPE_SCALAR:
							{
								std::vector<UINT32> Indices16(Accessor.count);
								std::ranges::copy(ResourceReader->ReadBinaryData<uint16_t>(Document, Accessor), std::begin(Indices16));

								Indices.reserve(Accessor.count);
								for (auto i : Indices16) {
									Indices.emplace_back(i);
								}
							}
							break;
							default: break;
							}
							break;
						case Microsoft::glTF::ComponentType::COMPONENT_UNSIGNED_INT:
							switch (Accessor.type)
							{
							case Microsoft::glTF::AccessorType::TYPE_SCALAR:
							{
								Indices.resize(Accessor.count);
								std::ranges::copy(ResourceReader->ReadBinaryData<uint32_t>(Document, Accessor), std::begin(Indices));
							}
							break;
							default: break;
							}
							break;
						default: break;
						}
					}
				}

				std::string AccessorId;
				if (empty(Vertices)) {
					if (j.TryGetAttributeAccessorId(Microsoft::glTF::ACCESSOR_POSITION, AccessorId))
					{
						const auto& Accessor = Document.accessors.Get(AccessorId);
						Vertices.resize(Accessor.count);
						switch (Accessor.componentType)
						{
						case Microsoft::glTF::ComponentType::COMPONENT_FLOAT:
							switch (Accessor.type)
							{
							case Microsoft::glTF::AccessorType::TYPE_VEC3:
							{
								std::memcpy(data(Vertices), data(ResourceReader->ReadBinaryData<float>(Document, Accessor)), TotalSizeOf(Vertices));

								AdjustScale(Vertices, 5.0f);
							}
							break;
							default: break;
							}
							break;
						default: break;
						}
					}
				}
				if (empty(Normals)) {
					if (j.TryGetAttributeAccessorId(Microsoft::glTF::ACCESSOR_NORMAL, AccessorId))
					{
						const auto& Accessor = Document.accessors.Get(AccessorId);
						Normals.resize(Accessor.count);
						switch (Accessor.componentType)
						{
						case Microsoft::glTF::ComponentType::COMPONENT_FLOAT:
							switch (Accessor.type)
							{
							case Microsoft::glTF::AccessorType::TYPE_VEC3:
							{
								std::memcpy(data(Normals), data(ResourceReader->ReadBinaryData<float>(Document, Accessor)), TotalSizeOf(Normals));
							}
							break;
							default: break;
							}
							break;
						default: break;
						}
					}
				}
			}
		}
	}	
#else
	DirectX::XMFLOAT3 ToFloat3(const FbxVector4& rhs) { return DirectX::XMFLOAT3(static_cast<FLOAT>(rhs[0]), static_cast<FLOAT>(rhs[1]), static_cast<FLOAT>(rhs[2])); }
	virtual void Process(FbxMesh* Mesh) override {
		Fbx::Process(Mesh);

		for (auto i = 0; i < Mesh->GetPolygonCount(); ++i) {
			for (auto j = 0; j < Mesh->GetPolygonSize(i); ++j) {
				Indices.emplace_back(i * Mesh->GetPolygonSize(i) + j); //!< RH
				Vertices.emplace_back(ToFloat3(Mesh->GetControlPoints()[Mesh->GetPolygonVertex(i, j)]));
			}
		}
		AdjustScale(Vertices, 5.0f);

		FbxArray<FbxVector4> Nrms;
		Mesh->GetPolygonVertexNormals(Nrms);
		for (auto i = 0; i < Nrms.Size(); ++i) {
			Normals.emplace_back(ToFloat3(Nrms[i]));
		}
	}
#endif

	virtual void CreateCommandList() override {
		//!< 【Pass0】コマンドリスト
		DX::CreateCommandList();
		//!< 【Pass1】バンドルコマンドリスト
		DX::CreateBundleCommandList(2);
	}

	virtual void CreateGeometry() override {
		const auto CA = COM_PTR_GET(DirectCommandAllocators[0]);
		const auto CL = COM_PTR_GET(DirectCommandLists[0]);
		const auto GCQ = COM_PTR_GET(GraphicsCommandQueue);
		const auto GF = COM_PTR_GET(GraphicsFence);

		//!<【Pass0】メッシュ描画用 [To draw mesh] 
#ifdef USE_GLTF
		Load(std::filesystem::path("..") / "Asset" / "bunny.glb");
		//Load(std::filesystem::path("..") / "Asset" / "dragon.glb");
		//Load(std::filesystem::path("..") / "Asset" / "happy_vrip.glb");
		//Load(std::filesystem::path("..") / "Asset" / "Sphere.glb");
		//Load(std::filesystem::path("..") / "Asset" / "Box.glb");
#else
		Load(std::filesystem::path("..") / "Asset" / "bunny.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "dragon.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "happy_vrip.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "Bee.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "ALucy.FBX");
#endif	

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

		//!< コマンド発行 [Issue copy command]
		VERIFY_SUCCEEDED(CL->Reset(CA, nullptr)); {
			//!< 【Pass0】
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
		//!<【Pass0】レンダーターゲット、デプス (キルトサイズ)  [Render target and depth (quilt size)]
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
			constexpr std::array DRs_Cbv = {
				D3D12_DESCRIPTOR_RANGE1({
					.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 
					.NumDescriptors = 1, 
					.BaseShaderRegister = 0, 
					.RegisterSpace = 0,
					.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
					.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND 
				})
			};
			DX::SerializeRootSignature(Blob, 
				{
					//!< CBV -> SetGraphicsRootDescriptorTable(0,..)
					D3D12_ROOT_PARAMETER1({
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
						.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1({.NumDescriptorRanges = static_cast<UINT>(size(DRs_Cbv)), .pDescriptorRanges = data(DRs_Cbv) }),
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
			constexpr std::array DRs_Srv = {
				D3D12_DESCRIPTOR_RANGE1({
					.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 
					.NumDescriptors = 1, 
					.BaseShaderRegister = 0, 
					.RegisterSpace = 0, 
					.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
					.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND 
				})
			};
			constexpr std::array DRs_Cbv = {
				D3D12_DESCRIPTOR_RANGE1({
					.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 
					.NumDescriptors = 1, 
					.BaseShaderRegister = 0, 
					.RegisterSpace = 0, 
					.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
					.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND 
				})
			};
			DX::SerializeRootSignature(Blob, 
				{
					//!< SRV -> SetGraphicsRootDescriptorTable(0,..)
					D3D12_ROOT_PARAMETER1({
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
						.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1({.NumDescriptorRanges = static_cast<uint32_t>(size(DRs_Srv)), .pDescriptorRanges = data(DRs_Srv) }),
						.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
					}),
					//!< CBV -> SetGraphicsRootDescriptorTable(1,..)
					D3D12_ROOT_PARAMETER1({
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
						.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1({.NumDescriptorRanges = static_cast<UINT>(size(DRs_Cbv)), .pDescriptorRanges = data(DRs_Cbv) }),
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

		//!< 【Pass0】
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

		//!< 【パス1】[Pass1]
		std::vector<COM_PTR<ID3DBlob>> SBsPass1;
		VERIFY_SUCCEEDED(D3DReadFileToBlob(data((std::filesystem::path(".") / "MeshPass1DX.vs.cso").wstring()), COM_PTR_PUT(SBsPass1.emplace_back())));
#ifdef DISPLAY_QUILT
		VERIFY_SUCCEEDED(D3DReadFileToBlob(data((std::filesystem::path(".") / "MeshQuiltDX.ps.cso").wstring()), COM_PTR_PUT(SBsPass1.emplace_back())));
#else
		VERIFY_SUCCEEDED(D3DReadFileToBlob(data((std::filesystem::path(".") / "MeshPass1DX.ps.cso").wstring()), COM_PTR_PUT(SBsPass1.emplace_back())));
#endif
		const std::array SBCsPass1 = {
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBsPass1[0]->GetBufferPointer(), .BytecodeLength = SBsPass1[0]->GetBufferSize() }),
			D3D12_SHADER_BYTECODE({.pShaderBytecode = SBsPass1[1]->GetBufferPointer(), .BytecodeLength = SBsPass1[1]->GetBufferSize() }),
		};
		CreatePipelineState_VsPs(PipelineStates[1], COM_PTR_GET(RootSignatures[1]), D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, RD, FALSE, SBCsPass1);

		for (auto& i : Threads) { i.join(); }
		Threads.clear();
	}
	void CreateDescriptor_Pass0() {
		//!< レンダーターゲット、デプスステンシルビュー [Render target, Depth stencil view]
		{
			const auto DescCount = 1;

			for (auto i = 0; i < 1; ++i) {
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
		}

		//!< コンスタントバッファービュー [Constant buffer view]
		{
			const auto DescCount = 1;

			for (auto i = 0; i < 1; ++i) {
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
		for (auto i = 0; i < 1; ++i) {
			auto& Desc = CbvSrvUavDescs.emplace_back();
			auto& Heap = Desc.first;
			auto& Handle = Desc.second;

			const D3D12_DESCRIPTOR_HEAP_DESC DHD = {
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = 1,
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
	}
	virtual void CreateDescriptor() override {
		CreateDescriptor_Pass0();
		CreateDescriptor_Pass1();
	}
	virtual void CreateViewport(const FLOAT Width, const FLOAT Height, const FLOAT MinDepth = 0.0f, const FLOAT MaxDepth = 1.0f) {
		D3D12_FEATURE_DATA_D3D12_OPTIONS3 FDO3;
		VERIFY_SUCCEEDED(Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, reinterpret_cast<void*>(&FDO3), sizeof(FDO3)));
		assert(D3D12_VIEW_INSTANCING_TIER_1 < FDO3.ViewInstancingTier && "");

		//!<【Pass0】キルトレンダーターゲットを分割 [Split quilt render target]
		const auto W = QuiltX / LenticularBuffer.TileX, H = QuiltY / LenticularBuffer.TileY;
		for (auto i = 0; i < LenticularBuffer.TileY; ++i) {
			const auto Y = QuiltY - H * (i + 1);
			for (auto j = 0; j < LenticularBuffer.TileX; ++j) {
				const auto X = j * W;
				QuiltViewports.emplace_back(D3D12_VIEWPORT({ .TopLeftX = static_cast<FLOAT>(X), .TopLeftY = static_cast<FLOAT>(Y), .Width = static_cast<FLOAT>(W), .Height = static_cast<FLOAT>(H), .MinDepth = MinDepth, .MaxDepth = MaxDepth }));
				QuiltScissorRects.emplace_back(D3D12_RECT({ .left = static_cast<LONG>(X), .top = static_cast<LONG>(Y), .right = static_cast<LONG>(X + W), .bottom = static_cast<LONG>(Y + H) }));
			}
		}

		//!<【Pass1】スクリーンを使用 [Using screen]
		DX::CreateViewport(Width, Height, MinDepth, MaxDepth);
	}
	virtual void PopulateBundleCommandList(const size_t i) override {
		if (0 == i) {
			const auto BCA = COM_PTR_GET(BundleCommandAllocators[0]);

			//!<【Pass0】
			{
				const auto BCL = COM_PTR_GET(BundleCommandLists[0]);
				const auto PS = COM_PTR_GET(PipelineStates[0]);
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
			}

			//!<【Pass1】
			{
				const auto BCL = COM_PTR_GET(BundleCommandLists[1]);
				const auto PS = COM_PTR_GET(PipelineStates[1]);
				VERIFY_SUCCEEDED(BCL->Reset(BCA, PS));
				{
					BCL->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
					BCL->ExecuteIndirect(COM_PTR_GET(IndirectBuffers[0].CommandSignature), 1, COM_PTR_GET(IndirectBuffers[0].Resource), 0, nullptr, 0);
				}
				VERIFY_SUCCEEDED(BCL->Close());
			}
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

				//!< レンダーターゲット
				{
					//!< レンダーテクスチャ [Render texture]
					const auto& HandleRTV = RtvDescs[0].second[0];
					//!< デプス [Depth]
					const auto& HandleDSV = DsvDescs[0].second[0];

					constexpr std::array<D3D12_RECT, 0> Rects = {};
					DCL->ClearRenderTargetView(HandleRTV, DirectX::Colors::SkyBlue, static_cast<UINT>(size(Rects)), data(Rects));
					DCL->ClearDepthStencilView(HandleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, static_cast<UINT>(size(Rects)), data(Rects));

					const std::array CHs = { HandleRTV };
					DCL->OMSetRenderTargets(static_cast<UINT>(size(CHs)), data(CHs), FALSE, &HandleDSV);
				}

				{
					//!< コンスタントバッファ [Constant buffer]
					const auto& Desc = CbvSrvUavDescs[0];
					const auto& Heap = Desc.first;
					const auto& Handle = Desc.second;
					const std::array DHs = { COM_PTR_GET(Heap) };
					DCL->SetDescriptorHeaps(static_cast<UINT>(size(DHs)), data(DHs));

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
			//!< スワップチェインをレンダーターゲット、レンダーテクスチャをシェーダリソースとする
			ResourceBarrier2(DCL,
				SCR, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET,
				RT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			//!<【Pass1】
			{
				const auto RS = COM_PTR_GET(RootSignatures[1]);
				const auto BCL = COM_PTR_GET(BundleCommandLists[1]);

				DCL->SetGraphicsRootSignature(RS);
				
				DCL->RSSetViewports(static_cast<UINT>(size(Viewports)), data(Viewports));
				DCL->RSSetScissorRects(static_cast<UINT>(size(ScissorRects)), data(ScissorRects));

				const std::array CHs = { SwapChainBackBuffers[i].Handle };
				DCL->OMSetRenderTargets(static_cast<UINT>(size(CHs)), data(CHs), FALSE, nullptr);

				//!< デスクリプタ
				{
					const auto& Desc = CbvSrvUavDescs[1];
					const auto& Heap = Desc.first;
					const auto& Handle = Desc.second;

					const std::array DHs = { COM_PTR_GET(Heap) };
					DCL->SetDescriptorHeaps(static_cast<UINT>(size(DHs)), data(DHs));
					DCL->SetGraphicsRootDescriptorTable(0, Handle[0]); //!< SRV
					DCL->SetGraphicsRootDescriptorTable(1, Handle[1]); //!< CBV
				}

				DCL->ExecuteBundle(BCL);
			}

			//!< スワップチェインをプレゼント、レンダーテクスチャをレンダーターゲットとする
			ResourceBarrier2(DCL,
				SCR, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT,
				RT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}
		VERIFY_SUCCEEDED(DCL->Close());
	}

	virtual uint32_t GetViewportMax() const override { 
		return D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; 
	}
	virtual void CreateProjectionMatrix(const int i) override {
		if (-1 == i) { ProjectionMatrices.clear(); return; }

		//!< 左右方向にずれている角度(ラジアン)
		const auto OffsetAngle = (static_cast<float>(i) / (LenticularBuffer.TileX * LenticularBuffer.TileY - 1.0f) - 0.5f) * ViewCone;
		//!< 左右方向にずれている距離
		const auto OffsetX = CameraDistance * std::tan(OffsetAngle);

		auto Prj = DirectX::XMMatrixPerspectiveFovRH(Fov, LenticularBuffer.DisplayAspect, 0.1f, 100.0f);
		Prj.r[2].m128_f32[0] += OffsetX / (CameraSize * LenticularBuffer.DisplayAspect);

		ProjectionMatrices.emplace_back(Prj);
	}
	virtual void CreateViewMatrix(const int i) override {
		if (-1 == i) { ViewMatrices.clear(); return; }

		const auto OffsetAngle = (static_cast<float>(i) / (LenticularBuffer.TileX * LenticularBuffer.TileY - 1.0f) - 0.5f) * ViewCone;
		const auto OffsetX = CameraDistance * std::tan(OffsetAngle);

		const auto OffsetLocal = DirectX::XMVector4Transform(DirectX::XMVectorSet(OffsetX, 0.0f, CameraDistance, 1.0f), View);
		ViewMatrices.emplace_back(View * DirectX::XMMatrixTranslationFromVector(OffsetLocal));

	}
	virtual void UpdateViewProjectionBuffer() override {
		const auto Count = (std::min)(static_cast<size_t>(LenticularBuffer.TileX * LenticularBuffer.TileY), _countof(ViewProjectionBuffer.ViewProjection));
		for (auto i = 0; i < Count; ++i) {
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
		DirectX::XMFLOAT4X4 ViewProjection[64]; //!< 64 もあれば十分 [64 will be enough]
	};
	VIEW_PROJECTION_BUFFER ViewProjectionBuffer;
};