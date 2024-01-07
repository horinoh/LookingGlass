#pragma once

#include <vector>
#include <array>
#include <iostream>
#include <cassert>
#include <filesystem>

//!< DLL ライブラリを使用する場合は FBXSDK_SHARED を定義する必要がある
#define FBXSDK_SHARED
#include <fbxsdk.h>

static std::ostream& operator<<(std::ostream& lhs, const FbxVector2& rhs) { lhs << rhs[0] << ", " << rhs[1] << std::endl; return lhs; }
static std::ostream& operator<<(std::ostream& lhs, const FbxVector4& rhs) { lhs << rhs[0] << ", " << rhs[1] << ", " << rhs[2] << ", " << rhs[3] << std::endl; return lhs; }
static std::ostream& operator<<(std::ostream& lhs, const FbxColor& rhs) { lhs << rhs[0] << ", " << rhs[1] << ", " << rhs[2] << ", " << rhs[3] << std::endl; return lhs; }

class Fbx
{
public:
	Fbx() {
		Manager = FbxManager::Create();
		const auto IOSettings = FbxIOSettings::Create(Manager, IOSROOT);
		if (nullptr != IOSettings) {
			Manager->SetIOSettings(IOSettings);
		}
	}
	virtual ~Fbx() {
		if (nullptr != Scene) { Scene->Destroy(); }
		if (nullptr != Manager) { Manager->Destroy(); }
	}

	virtual void Process(FbxMesh* Mesh) {
		if (nullptr != Mesh) {
			Mesh->GenerateNormals();

#pragma region POLYGON
			for (auto i = 0; i < Mesh->GetPolygonCount(); ++i) {
#pragma region POLYGON_SZIE
				for (auto j = 0; j < Mesh->GetPolygonSize(i); ++j) {
					const auto PolygonVertex = Mesh->GetPolygonVertex(i, j);
					if (0 <= PolygonVertex) {
#pragma region VERTEX_COLOR
						for (auto k = 0; k < Mesh->GetElementVertexColorCount(); ++k) {
							const auto VertexColor = Mesh->GetElementVertexColor(k);
							switch (VertexColor->GetMappingMode()) {
							default: break;
							case FbxGeometryElementVertexColor::eByControlPoint:
								break;
							case FbxGeometryElementVertexColor::eByPolygonVertex:
								break;
							}
						}
#pragma endregion

#pragma region UV
						for (auto k = 0; k < Mesh->GetElementUVCount(); ++k) {
							const auto UV = Mesh->GetElementUV(k);
							switch (UV->GetMappingMode()) {
							default: break;
							case FbxGeometryElementUV::eByControlPoint:
								switch (UV->GetReferenceMode()) {
								case FbxGeometryElementUV::eDirect:
									break;
								case FbxGeometryElementUV::eIndexToDirect:
									break;
								default: break;
								}
								break;
							case FbxGeometryElementUV::eByPolygonVertex:
								switch (UV->GetReferenceMode())
								{
								case FbxGeometryElementUV::eDirect:
								case FbxGeometryElementUV::eIndexToDirect:
									break;
								default: break;
								}
								break;
							}
						}
#pragma endregion

#pragma region NORMAL
						for (auto k = 0; k < Mesh->GetElementNormalCount(); ++k) {
							const auto Normal = Mesh->GetElementNormal(k);
							switch (Normal->GetMappingMode()) {
							default: break;
							case FbxGeometryElementNormal::eByControlPoint:
								if (FbxGeometryElement::eDirect == Normal->GetReferenceMode()) {
								}
								break;
							case FbxGeometryElementNormal::eByPolygonVertex:
								switch (Normal->GetReferenceMode()) {
								default: break;
								case FbxGeometryElementNormal::eDirect:
									break;
								case FbxGeometryElementNormal::eIndexToDirect:
									break;
								}
								break;
							}
						}
#pragma endregion

#pragma region TANGENT
						for (auto k = 0; k < Mesh->GetElementTangentCount(); ++k) {
							const auto Tangent = Mesh->GetElementTangent(k);
							if (FbxGeometryElement::eByPolygonVertex == Tangent->GetMappingMode()) {
								switch (Tangent->GetReferenceMode()) {
								default: break;
								case FbxGeometryElementTangent::eDirect:
									break;
								case FbxGeometryElementTangent::eIndexToDirect:
									break;
								}
							}
						}
#pragma endregion

#pragma region BINORMAL
						for (auto k = 0; k < Mesh->GetElementBinormalCount(); ++k) {
							const auto Binormal = Mesh->GetElementBinormal(k);
							if (FbxGeometryElement::eByPolygonVertex == Binormal->GetMappingMode()) {
								switch (Binormal->GetReferenceMode())
								{
								default: break;
								case FbxGeometryElementBinormal::eDirect:
									break;
								case FbxGeometryElementBinormal::eIndexToDirect:
									break;
								}
							}
						}
#pragma endregion
					}
				}
#pragma endregion //!< POLYGON_SIZE
			}
#pragma endregion //!< POLYGON
		}
	}
	virtual void Process(FbxNodeAttribute* Attr) {
		if (nullptr != Attr) {
			switch (Attr->GetAttributeType())
			{
				using enum FbxNodeAttribute::EType;
			default:
				break;
			case eUnknown:
				break;
			case eNull:
				break;
			case eMarker:
				break;
			case eSkeleton:
				break;
			case eMesh:
				Process(FbxCast<FbxMesh>(Attr));
				break;
			case eNurbs:
				break;
			case ePatch:
				break;
			case eCamera:
				break;
			case eCameraStereo:
				break;
			case eCameraSwitcher:
				break;
			case eLight:
				break;
			case eOpticalReference:
				break;
			case eOpticalMarker:
				break;
			case eNurbsCurve:
				break;
			case eTrimNurbsSurface:
				break;
			case eBoundary:
				break;
			case eNurbsSurface:
				break;
			case eShape:
				break;
			case eLODGroup:
				break;
			case eSubDiv:
				break;
			case eCachedEffect:
				break;
			case eLine:
				break;
			}
		}
	}
	virtual void Process(FbxNode* Node) {
		if (nullptr != Node) {
			//Tabs(); std::cout << "[ Node ] " << Node->GetName() << std::endl;
			[[maybe_unused]] const auto T = Node->LclTranslation.Get();
			[[maybe_unused]] const auto R = Node->LclRotation.Get();
			[[maybe_unused]] const auto S = Node->LclScaling.Get();

			[[maybe_unused]] const auto GTrans = Node->EvaluateGlobalTransform();
			[[maybe_unused]] const auto LTrans = Node->EvaluateLocalTransform();

			FbxTime Time;
			[[maybe_unused]] const auto GTransAnim = Node->EvaluateGlobalTransform(Time);

			const auto Evaluator = Scene->GetAnimationEvaluator();
			if (nullptr != Evaluator) {
				const FbxTime t(32);
				const auto GlobalMatrix = Evaluator->GetNodeGlobalTransform(Node, t);
				const auto LocalMatrix = Evaluator->GetNodeLocalTransform(Node, t);

				const auto LocalTranslation = Evaluator->GetNodeLocalTranslation(Node, t);
				const auto LocalRotation = Evaluator->GetNodeLocalRotation(Node, t);
				const auto LocalScaling = Evaluator->GetNodeLocalScaling(Node, t);
			}

			//PushTab();
			for (auto i = 0; i < Node->GetNodeAttributeCount(); ++i) {
				Process(Node->GetNodeAttributeByIndex(i));
			}
			//PopTab();

			//PushTab();
			for (auto i = 0; i < Node->GetChildCount(); i++) {
				Process(Node->GetChild(i));
			}
			//PopTab();
		}
	}
	virtual void Process(FbxSurfaceMaterial* Material) {
		if (nullptr != Material) {
			constexpr std::array Implementations = { FBXSDK_IMPLEMENTATION_CGFX, FBXSDK_IMPLEMENTATION_HLSL, FBXSDK_IMPLEMENTATION_SFX, FBXSDK_IMPLEMENTATION_OGS, FBXSDK_IMPLEMENTATION_SSSL };
			const FbxImplementation* Implementation = nullptr;
			for (auto i : Implementations) {
				Implementation = GetImplementation(Material, i);
				if (nullptr != Implementation) { break; }
			}
#pragma region IMPLEMENTATION
			if (nullptr != Implementation) {
				const auto RootTable = Implementation->GetRootTable();
				for (auto j = 0; j < RootTable->GetEntryCount(); ++j) {
					const auto Entry = RootTable->GetEntry(j);
					const auto EntryType = Entry.GetEntryType(true);

					FbxProperty Prop;
					if (0 == std::strcmp(FbxPropertyEntryView::sEntryType, EntryType)) {
						Prop = Material->FindPropertyHierarchical(Entry.GetSource());
						if (!Prop.IsValid()) {
							Prop = Material->RootProperty.FindHierarchical(Entry.GetSource());
						}
					}
					else if (0 == std::strcmp(FbxConstantEntryView::sEntryType, EntryType)) {
						Prop = Implementation->GetConstants().FindHierarchical(Entry.GetSource());
					}
					if (Prop.IsValid()) {
						if (Prop.GetSrcObjectCount<FbxTexture>() > 0) {
							for (int k = 0; k < Prop.GetSrcObjectCount<FbxFileTexture>(); ++k) {
							}
							for (int k = 0; k < Prop.GetSrcObjectCount<FbxLayeredTexture>(); ++k) {
							}
							for (int k = 0; k < Prop.GetSrcObjectCount<FbxProceduralTexture>(); ++k) {
							}
						}
						else {
							const auto PropType = Prop.GetPropertyDataType();
							if (FbxBoolDT == PropType) {
								Prop.Get<FbxBool>();
							}
							else if (FbxIntDT == PropType || FbxEnumDT == PropType) {
								Prop.Get<FbxInt>();
							}
							else if (FbxFloatDT == PropType) {
								Prop.Get<FbxFloat>();
							}
							else if (FbxDoubleDT == PropType) {
								Prop.Get<FbxDouble>();
							}
							else if (FbxStringDT == PropType || FbxUrlDT == PropType || FbxXRefUrlDT == PropType) {
								Prop.Get<FbxString>();
							}
							else if (FbxDouble2DT == PropType) {
								Prop.Get<FbxDouble2>();
							}
							else if (FbxDouble3DT == PropType || FbxColor3DT == PropType) {
								Prop.Get<FbxDouble3>();
							}
							else if (FbxDouble4DT == PropType || FbxColor4DT == PropType) {
								Prop.Get<FbxDouble4>();
							}
							else if (FbxDouble4x4DT == PropType) {
								Prop.Get<FbxDouble4x4>();
							}
						}
					}
				}
			}
#pragma endregion
#pragma region PHONG_LAMBERT
			else {
				const auto Lambert = FbxCast<FbxSurfaceLambert>(Material);
				if (nullptr != Lambert) {
					const auto Emissive = Lambert->Emissive.Get();
					const auto EmissiveFactor = Lambert->EmissiveFactor.Get();

					const auto Ambient = Lambert->Ambient.Get();
					const auto AmbientFactor = Lambert->AmbientFactor.Get();

					const auto Diffuse = Lambert->Diffuse.Get();
					const auto DiffuseFactor = Lambert->DiffuseFactor.Get();
					//!< テクスチャの有無を調べる例
					for (auto i = 0; i < Lambert->Diffuse.GetSrcObjectCount<FbxFileTexture>(); ++i) {
						const auto FileTexture = Lambert->Diffuse.GetSrcObject<FbxFileTexture>(i);
						if (nullptr != FileTexture) {
						}
					}

					const auto NormalMap = Lambert->NormalMap.Get();
					for (auto i = 0; i < Lambert->NormalMap.GetSrcObjectCount<FbxFileTexture>(); ++i) {
						const auto FileTexture = Lambert->NormalMap.GetSrcObject<FbxFileTexture>(i);
						if (nullptr != FileTexture) {
						}
					}

					const auto Bump = Lambert->Bump.Get();
					const auto BumpFactor = Lambert->BumpFactor.Get();

					const auto TransparentColor = Lambert->TransparentColor.Get();
					const auto TransparencyFactor = Lambert->TransparencyFactor.Get();

					const auto DisplacementColor = Lambert->DisplacementColor.Get();
					const auto DisplacementFactor = Lambert->DisplacementFactor.Get();

					const auto VectorDisplacementColor = Lambert->VectorDisplacementColor.Get();
					const auto VectorDisplacementFactor = Lambert->VectorDisplacementFactor.Get();

					const auto Phong = FbxCast<FbxSurfacePhong>(Lambert);
					if (nullptr != Phong) {
						const auto Specular = Phong->Specular.Get();
						const auto SpecularFactor = Phong->SpecularFactor.Get();
						const auto Shiness = Phong->Shininess.Get();
						const auto Reflection = Phong->Reflection.Get();
						const auto ReflectionFactor = Phong->ReflectionFactor.Get();
					}
				}
			}
#pragma endregion
		}
	}

	virtual void Convert(FbxScene* Scn) {
		if (nullptr != Scn) {
			//!< ノードのトランスフォームに影響する、頂点等には影響しない
			{
				//!< 【OpenGL】RH
				FbxAxisSystem::OpenGL.ConvertScene(Scn);
				//!< 【DirectX】LH
				//FbxAxisSystem::DirectX.ConvertScene(Scn);
			}

			//!< 【単位】m (メートル)
			FbxSystemUnit::m.ConvertScene(Scn);

			//!< 三角形ポリゴン化
			if (FbxGeometryConverter(Manager).Triangulate(Scn, true)) {}
		}
	}

	virtual void Load(const std::filesystem::path& Path) {
		if (nullptr != Manager) {
			const auto Importer = FbxImporter::Create(Manager, "");
			if (nullptr != Importer) {
				if (Importer->Initialize(data(Path.string()), -1, Manager->GetIOSettings())) {
					Scene = FbxScene::Create(Manager, "");
					if (Importer->Import(Scene)) {
						Convert(Scene);

						int Major, Minor, Revision;
						Importer->GetFileVersion(Major, Minor, Revision);
						
						//!< ノードをたどる
						Process(Scene->GetRootNode());
					}
				} else {
					std::cerr << Importer->GetStatus().GetErrorString() << std::endl;
					switch (Importer->GetStatus().GetCode()) {
					case FbxStatus::eSuccess:break;
					case FbxStatus::eFailure:break;
					case FbxStatus::eInsufficientMemory:break;
					case FbxStatus::eInvalidParameter:break;
					case FbxStatus::eIndexOutOfRange:break;
					case FbxStatus::ePasswordError:break;
					case FbxStatus::eInvalidFileVersion:break;
					case FbxStatus::eInvalidFile:break;
					case FbxStatus::eSceneCheckFail:break;
					}
					__debugbreak();
				}
				Importer->Destroy();
			}
		}
	}

protected:
	FbxManager* Manager = nullptr;
	FbxScene* Scene = nullptr;
};
