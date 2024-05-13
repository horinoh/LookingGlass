#pragma once

#include "Holo.h"

#ifdef USE_TEXTURE
#include "VKImage.h"
#else
#include "VK.h"
#endif

#ifdef USE_GLTF
#include "GltfSDK.h"
#endif
#ifdef USE_FBX
#include "FBX.h"
#endif

class HoloVK : public VK, public Holo
{
public:
	virtual uint32_t GetViewportMax() const override {
		VkPhysicalDeviceProperties PDP;
		vkGetPhysicalDeviceProperties(CurrentPhysicalDevice, &PDP);
		return PDP.limits.maxViewports;
	}
};

#ifdef USE_TEXTURE
class HoloImageVK : public VKImage, public Holo
{
public:
	virtual uint32_t GetViewportMax() const override {
		VkPhysicalDeviceProperties PDP;
		vkGetPhysicalDeviceProperties(CurrentPhysicalDevice, &PDP);
		return PDP.limits.maxViewports;
	}
};
#endif

class HoloViewsVK : public Holo
{
public:
	void CreateViewportScissor(const FLOAT MinDepth = 0.0f, const FLOAT MaxDepth = 1.0f) {
		//!<【Pass0】キルトレンダーターゲットを分割 [Split quilt render target]
		const auto W = QuiltX / LenticularBuffer.TileX, H = QuiltY / LenticularBuffer.TileY;
		const auto Ext2D = VkExtent2D({ .width = static_cast<uint32_t>(W), .height = static_cast<uint32_t>(H) });
		for (auto i = 0; i < LenticularBuffer.TileY; ++i) {
			const auto Y = QuiltY - H * i;
			for (auto j = 0; j < LenticularBuffer.TileX; ++j) {
				const auto X = j * W;
				QuiltViewports.emplace_back(VkViewport({ .x = static_cast<float>(X), .y = static_cast<float>(Y), .width = static_cast<float>(W), .height = -static_cast<float>(H), .minDepth = MinDepth, .maxDepth = MaxDepth }));
				QuiltScissorRects.emplace_back(VkRect2D({ VkOffset2D({.x = static_cast<int32_t>(X), .y = static_cast<int32_t>(Y - H) }), Ext2D }));
			}
		}
	}
	virtual void CreateProjectionMatrix(const int i) override {
		if (-1 == i) { ProjectionMatrices.clear(); return; }

		//!< 左右方向にずれている角度(ラジアン)
		const auto OffsetAngle = (static_cast<float>(i) / (LenticularBuffer.TileX * LenticularBuffer.TileY - 1.0f) - 0.5f) * ViewCone;
		//!< 左右方向にずれている距離
		const auto OffsetX = CameraDistance * std::tan(OffsetAngle);

		auto Prj = glm::perspective(Fov, LenticularBuffer.DisplayAspect, 0.1f, 100.0f);
		Prj[2][0] += OffsetX / (CameraSize * LenticularBuffer.DisplayAspect);

		ProjectionMatrices.emplace_back(Prj);
	}
	virtual void CreateViewMatrix(const int i) override {
		if (-1 == i) { ViewMatrices.clear(); return; }

		const auto OffsetAngle = (static_cast<float>(i) / (LenticularBuffer.TileX * LenticularBuffer.TileY - 1.0f) - 0.5f) * ViewCone;
		const auto OffsetX = CameraDistance * std::tan(OffsetAngle);

		const auto OffsetLocal = glm::vec3(View * glm::vec4(OffsetX, 0.0f, CameraDistance, 1.0f));
		ViewMatrices.emplace_back(glm::translate(View, OffsetLocal));
	}
protected:
	std::vector<VkViewport> QuiltViewports;
	std::vector<VkRect2D> QuiltScissorRects;

	std::vector<glm::mat4> ProjectionMatrices;
	std::vector<glm::mat4> ViewMatrices;

	glm::mat4 View;
};

#ifdef USE_TEXTURE
class HoloViewsImageVK : public HoloViewsVK, public VKImage
{
public:
	virtual uint32_t GetViewportMax() const override {
		VkPhysicalDeviceProperties PDP;
		vkGetPhysicalDeviceProperties(CurrentPhysicalDevice, &PDP);
		return PDP.limits.maxViewports;
	}
};
#endif

#ifdef USE_GLTF
class HoloGLTFVK : public VK, public HoloViewsVK, public Gltf::SDK
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
								std::vector<uint16_t> Indices16(Accessor.count);
								std::ranges::copy(ResourceReader->ReadBinaryData<uint16_t>(Document, Accessor), std::begin(Indices16));
								//!< uint32_t フォーマットで扱う
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
	std::vector<uint32_t> Indices;
	std::vector<glm::vec3> Vertices;
	std::vector<glm::vec3> Normals;
};
#endif
#ifdef USE_FBX
class HoloFBXVK : public VK, public HoloViewsVK, public Fbx
{
public:
	glm::vec3 ToVec3(const FbxVector4& rhs) { return glm::vec3(static_cast<FLOAT>(rhs[0]), static_cast<FLOAT>(rhs[1]), static_cast<FLOAT>(rhs[2])); }
	virtual void Process(FbxMesh* Mesh) override {
		Fbx::Process(Mesh);

		for (auto i = 0; i < Mesh->GetPolygonCount(); ++i) {
			for (auto j = 0; j < Mesh->GetPolygonSize(i); ++j) {
				Indices.emplace_back(i * Mesh->GetPolygonSize(i) + j); //!< RH
				Vertices.emplace_back(ToVec3(Mesh->GetControlPoints()[Mesh->GetPolygonVertex(i, j)]));
			}
		}
		AdjustScale(Vertices, GetMeshScale());

		FbxArray<FbxVector4> Nrms;
		Mesh->GetPolygonVertexNormals(Nrms);
		for (auto i = 0; i < Nrms.Size(); ++i) {
			Normals.emplace_back(ToVec3(Nrms[i]));
		}

		LOG(std::data(std::format("VertexCount = {}\n", std::size(Vertices))));
		LOG(std::data(std::format("PolygonCount = {}\n", std::size(Indices) / 3)));
	}
	virtual float GetMeshScale() const { return 5.0f; }
protected:
	std::vector<uint32_t> Indices;
	std::vector<glm::vec3> Vertices;
	std::vector<glm::vec3> Normals;
};
#endif