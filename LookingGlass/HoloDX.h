#pragma once

#include "Holo.h"

#ifdef USE_TEXTURE
#include "DXImage.h"
#else
#include "DX.h"
#endif

#ifdef USE_GLTF
#include "GltfSDK.h"
#endif
#ifdef USE_FBX
#include "FBX.h"
#endif

class HoloDX : public DX, public Holo
{
public:
	virtual uint32_t GetViewportMax() const override { return D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; }
};

#ifdef USE_TEXTURE
class HoloImageDX : public DXImage, public Holo
{
public:
	virtual uint32_t GetViewportMax() const override { return D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; }
};
#endif

class HoloViewsDX : public Holo
{
public:
	void CreateViewportScissor(const FLOAT MinDepth = 0.0f, const FLOAT MaxDepth = 1.0f) {
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
protected:
	std::vector<D3D12_VIEWPORT> QuiltViewports;
	std::vector<D3D12_RECT> QuiltScissorRects;

	std::vector<DirectX::XMMATRIX> ProjectionMatrices;
	std::vector<DirectX::XMMATRIX> ViewMatrices;

	DirectX::XMMATRIX View;
};

#ifdef USE_TEXTURE
class HoloViewsImageDX : public HoloViewsDX, public DXImage
{
public:
	virtual uint32_t GetViewportMax() const override { return D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; }
};
#endif

#ifdef USE_GLTF
class HoloGLTFDX : public DX, public HoloViewsDX, public  Gltf::SDK
{
public:
	virtual void Process() override {
		for (const auto& i : Document.meshes.Elements()) {
			for (const auto& j : i.primitives) {
				switch (j.mode)
				{
				case Microsoft::glTF::MeshMode::MESH_TRIANGLES:
					break;
				default:
					__debugbreak();
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
								//!< UINT フォーマットで扱う
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
								std::memcpy(std::data(Vertices), std::data(ResourceReader->ReadBinaryData<float>(Document, Accessor)), TotalSizeOf(Vertices));

								AdjustScale(Vertices, GetMeshScale());
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
								std::memcpy(std::data(Normals), std::data(ResourceReader->ReadBinaryData<float>(Document, Accessor)), TotalSizeOf(Normals));
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

		LOG(std::data(std::format("VertexCount = {}\n", std::size(Vertices))));
		LOG(std::data(std::format("PolygonCount = {}\n", std::size(Indices) / 3)));
	}
	virtual float GetMeshScale() const { return 5.0f; }
protected:
	std::vector<UINT32> Indices;
	std::vector<DirectX::XMFLOAT3> Vertices;
	std::vector<DirectX::XMFLOAT3> Normals;

	float Scale = 5.0f;
}; 
#endif
#ifdef USE_FBX
class HoloFBXDX : public DX, public HoloViewsDX, public Fbx
{
public:
	DirectX::XMFLOAT3 ToFloat3(const FbxVector4& rhs) { return DirectX::XMFLOAT3(static_cast<FLOAT>(rhs[0]), static_cast<FLOAT>(rhs[1]), static_cast<FLOAT>(rhs[2])); }
	virtual void Process(FbxMesh* Mesh) override {
		Fbx::Process(Mesh);

		for (auto i = 0; i < Mesh->GetPolygonCount(); ++i) {
			for (auto j = 0; j < Mesh->GetPolygonSize(i); ++j) {
				Indices.emplace_back(i * Mesh->GetPolygonSize(i) + j); //!< RH
				Vertices.emplace_back(ToFloat3(Mesh->GetControlPoints()[Mesh->GetPolygonVertex(i, j)]));
			}
		}
		AdjustScale(Vertices, GetMeshScale());

		FbxArray<FbxVector4> Nrms;
		Mesh->GetPolygonVertexNormals(Nrms);
		for (auto i = 0; i < Nrms.Size(); ++i) {
			Normals.emplace_back(ToFloat3(Nrms[i]));
		}

		LOG(std::data(std::format("VertexCount = {}\n", std::size(Vertices))));
		LOG(std::data(std::format("PolygonCount = {}\n", std::size(Indices) / 3)));
	}
	virtual float GetMeshScale() const { return 5.0f; }
protected:
	std::vector<UINT32> Indices;
	std::vector<DirectX::XMFLOAT3> Vertices;
	std::vector<DirectX::XMFLOAT3> Normals;
};
#endif
