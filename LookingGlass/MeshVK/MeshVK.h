#pragma once

#include "resource.h"

#include "../VK.h"
#include "../Holo.h"
#include "../FBX.h"

class MeshVK : public VK, public Holo, public Fbx
{
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		Holo::SetHoloWindow(hWnd, hInstance);

		CreateProjectionMatrices();
		{
			const auto Pos = glm::vec3(0.0f, 0.0f, 1.0f);
			const auto Tag = glm::vec3(0.0f);
			const auto Up = glm::vec3(0.0f, 1.0f, 0.0f);
			View = glm::lookAt(Pos, Tag, Up);
			CreateViewMatrices();
		}
		UpdateViewProjectionBuffer();

		VK::OnCreate(hWnd, hInstance, Title);
	}

	glm::vec3 ToVec3(const FbxVector4& rhs) { return glm::vec3(static_cast<FLOAT>(rhs[0]), static_cast<FLOAT>(rhs[1]), static_cast<FLOAT>(rhs[2])); }
	virtual void Process(FbxMesh* Mesh) override {
		Fbx::Process(Mesh);

		for (auto i = 0; i < Mesh->GetPolygonCount(); ++i) {
			for (auto j = 0; j < Mesh->GetPolygonSize(i); ++j) {
				Indices.emplace_back(i * Mesh->GetPolygonSize(i) + j); //!< RH
				Vertices.emplace_back(ToVec3(Mesh->GetControlPoints()[Mesh->GetPolygonVertex(i, j)]));
			}
		}
		AdjustScale(Vertices, 5.0f);

		FbxArray<FbxVector4> Nrms;
		Mesh->GetPolygonVertexNormals(Nrms);
		for (auto i = 0; i < Nrms.Size(); ++i) {
			Normals.emplace_back(ToVec3(Nrms[i]));
		}
	}

	virtual void AllocateCommandBuffer() override {
		//!<�y�p�X0�z�R�}���h�o�b�t�@ [Pass0]
		VK::AllocateCommandBuffer();
		//!<�y�p�X1�z�Z�J���_���R�}���h�o�b�t�@ [Pass1]
		VK::AllocateSecondaryCommandBuffer(2);
	}

	virtual void CreateGeometry() override {
		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;
		const auto CB = CommandBuffers[0];

		//!<�y�p�X0�z���b�V���`��p [Pass0 To draw mesh] 
		Load(std::filesystem::path("..") / "Asset" / "bunny.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "dragon.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "happy_vrip_res2.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "Bee.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "ALucy.FBX");

		VertexBuffers.emplace_back().Create(Device, PDMP, TotalSizeOf(Vertices));
		VK::Scoped<StagingBuffer> StagingPass0Vertex(Device);
		StagingPass0Vertex.Create(Device, PDMP, TotalSizeOf(Vertices), data(Vertices));

		VertexBuffers.emplace_back().Create(Device, PDMP, TotalSizeOf(Normals));
		VK::Scoped<StagingBuffer> StagingPass0Normal(Device);
		StagingPass0Normal.Create(Device, PDMP, TotalSizeOf(Normals), data(Normals));

		IndexBuffers.emplace_back().Create(Device, PDMP, TotalSizeOf(Indices));
		VK::Scoped<StagingBuffer> StagingPass0Index(Device);
		StagingPass0Index.Create(Device, PDMP, TotalSizeOf(Indices), data(Indices));

		const VkDrawIndexedIndirectCommand DIIC = {
			.indexCount = static_cast<uint32_t>(size(Indices)), 
			.instanceCount = 1, 
			.firstIndex = 0, 
			.vertexOffset = 0, 
			.firstInstance = 0 
		};
		IndirectBuffers.emplace_back().Create(Device, PDMP, DIIC);
		VK::Scoped<StagingBuffer> StagingPass0Indirect(Device);
		StagingPass0Indirect.Create(Device, PDMP, sizeof(DIIC), &DIIC);

		//!<�y�p�X0�z�����_�[�e�N�X�`���`��p [Pass1 To draw render texture]
		constexpr VkDrawIndirectCommand DIC = { 
			.vertexCount = 4, 
			.instanceCount = 1, 
			.firstVertex = 0,
			.firstInstance = 0 
		};
		IndirectBuffers.emplace_back().Create(Device, PDMP, DIC).SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, sizeof(DIC), &DIC);
		VK::Scoped<StagingBuffer> StagingPass1Indirect(Device);
		StagingPass1Indirect.Create(Device, PDMP, sizeof(DIC), &DIC);

		//!< �R�s�[�R�}���h���s [Issue copy command]
		constexpr VkCommandBufferBeginInfo CBBI = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 
			.pNext = nullptr, 
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 
			.pInheritanceInfo = nullptr 
		};
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CB, &CBBI)); {
			//!< �y�p�X0�z[Pass0]
			VertexBuffers[0].PopulateCopyCommand(CB, TotalSizeOf(Vertices), StagingPass0Vertex.Buffer);
			VertexBuffers[1].PopulateCopyCommand(CB, TotalSizeOf(Normals), StagingPass0Normal.Buffer);
			IndexBuffers[0].PopulateCopyCommand(CB, TotalSizeOf(Indices), StagingPass0Index.Buffer);
			IndirectBuffers[0].PopulateCopyCommand(CB, sizeof(DIIC), StagingPass0Indirect.Buffer);

			//!< �y�p�X1�z[Pass1]
			IndirectBuffers[1].PopulateCopyCommand(CB, sizeof(DIC), StagingPass1Indirect.Buffer);
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CB));
		VK::SubmitAndWait(GraphicsQueue, CB);
	}

	virtual void CreateUniformBuffer() override {
		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;

		//!<�y�p�X0�z[Pass0]
		UniformBuffers.emplace_back().Create(Device, PDMP, sizeof(ViewProjectionBuffer));
		CopyToHostVisibleDeviceMemory(Device, UniformBuffers.back().DeviceMemory, 0, sizeof(ViewProjectionBuffer), &ViewProjectionBuffer);

		//!<�y�p�X1�z[Pass1]
		UniformBuffers.emplace_back().Create(Device, PDMP, sizeof(LenticularBuffer));
		CopyToHostVisibleDeviceMemory(Device, UniformBuffers.back().DeviceMemory, 0, sizeof(LenticularBuffer), &LenticularBuffer);
	}
	virtual void CreateTexture() override {
		//!<�y�p�X0�z�����_�[�^�[�Q�b�g�A�f�v�X (�L���g�T�C�Y) [Pass0 Render target and depth (quilt size)]
		CreateTexture_Render(QuiltWidth, QuiltHeight);
		CreateTexture_Depth(QuiltWidth, QuiltHeight);
	}
	virtual void CreateImmutableSampler() override {
		//!< �y�p�X1�z[Pass1]
		CreateImmutableSampler_LinearRepeat();
	}
	virtual void CreatePipelineLayout() override {
		//!<�y�p�X0�z[Pass0 Using dynamic offset UNIFORM_BUFFER_DYNAMIC]
		//!< �_�C�i�~�b�N�I�t�Z�b�g���g�p����ׁAUNIFORM_BUFFER_DYNAMIC
		{
			CreateDescriptorSetLayout(DescriptorSetLayouts.emplace_back(), 0, {
				VkDescriptorSetLayoutBinding({.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT, .pImmutableSamplers = nullptr }),
			});
			VK::CreatePipelineLayout(PipelineLayouts.emplace_back(), { DescriptorSetLayouts[0] }, {});
		}

		//!<�y�p�X1�z[Pass1]
		{
			const std::array ISs = { Samplers[0] };
			CreateDescriptorSetLayout(DescriptorSetLayouts.emplace_back(), 0, {
				VkDescriptorSetLayoutBinding({.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = static_cast<uint32_t>(size(ISs)), .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .pImmutableSamplers = data(ISs) }),
				VkDescriptorSetLayoutBinding({.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .pImmutableSamplers = nullptr }),
			});
			VK::CreatePipelineLayout(PipelineLayouts.emplace_back(), { DescriptorSetLayouts[1] }, {});
		}
	}
	virtual void CreateRenderPass() override { 
		//!< �y�p�X0�z[Pass0]
		CreateRenderPass_Depth();
		//!< �y�p�X1�z[Pass1]
		CreateRenderPass_None();
	}
	virtual void CreatePipeline() override {
		Pipelines.emplace_back();
		Pipelines.emplace_back();

		constexpr VkPipelineRasterizationStateCreateInfo PRSCI = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE, .depthBiasConstantFactor = 0.0f, .depthBiasClamp = 0.0f, .depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f
		};
		
		//!<�y�p�X0�z[Pass0]
		const std::vector VIBDs = {
			VkVertexInputBindingDescription({.binding = 0, .stride = sizeof(Vertices[0]), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }),
			VkVertexInputBindingDescription({.binding = 1, .stride = sizeof(Normals[0]), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }),
		};
		const std::vector VIADs = {
			VkVertexInputAttributeDescription({.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 }),
			VkVertexInputAttributeDescription({.location = 1, .binding = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 }),
		};
		const std::array SMsPass0 = {
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass0VK.vert.spv"),
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass0VK.frag.spv"),
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass0VK.geom.spv"),
		};
		const std::array PSSCIsPass0 = {
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = SMsPass0[0], .pName = "main", .pSpecializationInfo = nullptr }),
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = SMsPass0[1], .pName = "main", .pSpecializationInfo = nullptr }),
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_GEOMETRY_BIT, .module = SMsPass0[2], .pName = "main", .pSpecializationInfo = nullptr }),
		};
		CreatePipelineState_VsFsGs_Input(Pipelines[0], PipelineLayouts[0], RenderPasses[0], VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, PRSCI, VK_TRUE, VIBDs, VIADs, PSSCIsPass0);
		
		//!<�y�p�X1�z[Pass1]
		const std::array SMsPass1 = {
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass1VK.vert.spv"),
#ifdef DISPLAY_QUILT
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshQuiltVK.frag.spv"),
#else
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass1VK.frag.spv"),
#endif
		};
		const std::array PSSCIsPass1 = {
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = SMsPass1[0], .pName = "main", .pSpecializationInfo = nullptr }),
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = SMsPass1[1], .pName = "main", .pSpecializationInfo = nullptr }),
		};
		CreatePipeline_VsFs(Pipelines[1], PipelineLayouts[1], RenderPasses[1], VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0, PRSCI, VK_FALSE, PSSCIsPass1);

		for (auto& i : Threads) { i.join(); }
		Threads.clear();

		for (auto i : SMsPass0) { vkDestroyShaderModule(Device, i, GetAllocationCallbacks()); }
		for (auto i : SMsPass1) { vkDestroyShaderModule(Device, i, GetAllocationCallbacks()); }
	}
	virtual void CreateDescriptor() override {
		//!<�y�p�X0�z[Pass0 Using dynamic offset UNIFORM_BUFFER_DYNAMIC]
		//!< �_�C�i�~�b�N�I�t�Z�b�g (UNIFORM_BUFFER_DYNAMIC) ���g�p
		{
			const auto DescCount = 1;

			const auto UBIndex = 0;
			const auto DSIndex = 0;

			VK::CreateDescriptorPool(DescriptorPools.emplace_back(), 0, {
				VkDescriptorPoolSize({.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, .descriptorCount = DescCount }),
			});
			auto DSL = DescriptorSetLayouts[0];
			auto DP = DescriptorPools[0];
			const std::array DSLs = { DSL };
			for (uint32_t i = 0; i < DescCount; ++i) {
				const VkDescriptorSetAllocateInfo DSAI = {
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					.pNext = nullptr,
					.descriptorPool = DP,
					.descriptorSetCount = static_cast<uint32_t>(size(DSLs)), .pSetLayouts = data(DSLs)
				};
				VERIFY_SUCCEEDED(vkAllocateDescriptorSets(Device, &DSAI, &DescriptorSets.emplace_back()));
			}

			struct DescriptorUpdateInfo
			{
				VkDescriptorBufferInfo DBI[1];
			};
			VkDescriptorUpdateTemplate DUT;
			VK::CreateDescriptorUpdateTemplate(DUT, VK_PIPELINE_BIND_POINT_GRAPHICS, {
				VkDescriptorUpdateTemplateEntry({
					.dstBinding = 0, .dstArrayElement = 0,
					.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
					.offset = offsetof(DescriptorUpdateInfo, DBI), .stride = sizeof(DescriptorUpdateInfo)
				}),
			}, DSL);

			//!< �_�C�i�~�b�N�I�t�Z�b�g (UNIFORM_BUFFER_DYNAMIC) ���g�p����ꍇ�A.range �ɂ� VK_WHOLE_SIZE �ł͖����I�t�Z�b�g���Ɏg�p����T�C�Y���w�肷��
			//!< [When using dynamic offset, .range value is data size used in each offset]
			//!<	100(VK_WHOLE_SIZE) = 25(DynamicOffset) * 4 �Ȃ� 25 ���w�肷��Ƃ�������
			const auto DynamicOffset = GetViewportMax() * sizeof(ViewProjectionBuffer.ViewProjection[0]);
			for (uint32_t i = 0; i < DescCount; ++i) {
				const DescriptorUpdateInfo DUI = {
					VkDescriptorBufferInfo({.buffer = UniformBuffers[UBIndex + i].Buffer, .offset = 0, .range = DynamicOffset }),
				};
				vkUpdateDescriptorSetWithTemplate(Device, DescriptorSets[DSIndex + i], DUT, &DUI);
			}
			vkDestroyDescriptorUpdateTemplate(Device, DUT, GetAllocationCallbacks());
		}

		//!< �y�p�X1�z[Pass1]
		{
			const auto DescCount = 1;

			const auto UBIndex = 1;
			const auto DSIndex = 1;

			VK::CreateDescriptorPool(DescriptorPools.emplace_back(), 0, {
				VkDescriptorPoolSize({.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1 }),
				VkDescriptorPoolSize({.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1 }),
			});

			auto DSL = DescriptorSetLayouts[1];
			auto DP = DescriptorPools[1];
			const std::array DSLs = { DSL };
			for (uint32_t i = 0; i < DescCount; ++i) {
				const VkDescriptorSetAllocateInfo DSAI = {
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					.pNext = nullptr,
					.descriptorPool = DP,
					.descriptorSetCount = static_cast<uint32_t>(size(DSLs)), .pSetLayouts = data(DSLs)
				};
				VERIFY_SUCCEEDED(vkAllocateDescriptorSets(Device, &DSAI, &DescriptorSets.emplace_back()));
			}

			struct DescriptorUpdateInfo
			{
				VkDescriptorImageInfo DII[1];
				VkDescriptorBufferInfo DBI[1];
			};
			VkDescriptorUpdateTemplate DUT;
			VK::CreateDescriptorUpdateTemplate(DUT, VK_PIPELINE_BIND_POINT_GRAPHICS, {
				VkDescriptorUpdateTemplateEntry({
					.dstBinding = 0, .dstArrayElement = 0,
					.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.offset = offsetof(DescriptorUpdateInfo, DII), .stride = sizeof(VkDescriptorImageInfo)
				}),
				VkDescriptorUpdateTemplateEntry({
					.dstBinding = 1, .dstArrayElement = 0,
					.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.offset = offsetof(DescriptorUpdateInfo, DBI), .stride = sizeof(DescriptorUpdateInfo)
				}),
			}, DSL);
			for (uint32_t i = 0; i < DescCount; ++i) {
				const DescriptorUpdateInfo DUI = {
					VkDescriptorImageInfo({.sampler = VK_NULL_HANDLE, .imageView = RenderTextures[0].View, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }),
					VkDescriptorBufferInfo({.buffer = UniformBuffers[UBIndex + i].Buffer, .offset = 0, .range = VK_WHOLE_SIZE}),
				};
				vkUpdateDescriptorSetWithTemplate(Device, DescriptorSets[DSIndex + i], DUT, &DUI);
			}
			vkDestroyDescriptorUpdateTemplate(Device, DUT, GetAllocationCallbacks());
		}
	}
	virtual void CreateFramebuffer() override {
		//!< �y�p�X0�z[Pass0 Render target (quilt size)]
		//!< �����_�[�^�[�Q�b�g (�L���g�T�C�Y)
		VK::CreateFramebuffer(Framebuffers.emplace_back(), RenderPasses[0], QuiltWidth, QuiltHeight, 1, { RenderTextures[0].View, DepthTextures[0].View });

		//!<�y�p�X1�z[Pass1 Swapchain (screen size)]
		//!< �X���b�v�`�F�C�� (�X�N���[���T�C�Y)
		for (const auto& i : SwapchainBackBuffers) {
			VK::CreateFramebuffer(Framebuffers.emplace_back(), RenderPasses[1], SurfaceExtent2D.width, SurfaceExtent2D.height, 1, { i.ImageView });
		}
	}
	virtual void CreateViewport(const float Width, const float Height, const float MinDepth = 0.0f, const float MaxDepth = 1.0f) override {
		//!<�y�p�X0�z�L���g�����_�[�^�[�Q�b�g�𕪊� [Pass0 Split quilt render target]
		const auto W = QuiltWidth / LenticularBuffer.Column, H = QuiltHeight / LenticularBuffer.Row;
		const auto Ext2D = VkExtent2D({ .width = static_cast<uint32_t>(W), .height = static_cast<uint32_t>(H) });
		for (auto i = 0; i < LenticularBuffer.Row; ++i) {
			const auto Y = QuiltHeight - H * i;
			for (auto j = 0; j < LenticularBuffer.Column; ++j) {
				const auto X = j * W;
				QuiltViewports.emplace_back(VkViewport({ .x = X, .y = Y, .width = W, .height = -H, .minDepth = MinDepth, .maxDepth = MaxDepth }));
				QuiltScissorRects.emplace_back(VkRect2D({ VkOffset2D({.x = static_cast<int32_t>(X), .y = static_cast<int32_t>(Y - H) }), Ext2D }));
			}
		}

		//!<�y�p�X1�z�X�N���[�����g�p [Pass1 Using screen]
		VK::CreateViewport(Width, Height, MinDepth, MaxDepth);
	}
	virtual void PopulateSecondaryCommandBuffer(const size_t i) override {
		if (0 == i) {
			//!<�y�p�X0�z[Pass0]
			{
				const auto RP = RenderPasses[0];
				const auto PL = Pipelines[0];
				const auto PLL = PipelineLayouts[0];
				const auto SCB = SecondaryCommandBuffers[0];
				const auto DS = DescriptorSets[0];

				const VkCommandBufferInheritanceInfo CBII = {
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
					.pNext = nullptr,
					.renderPass = RP,
					.subpass = 0,
					.framebuffer = VK_NULL_HANDLE,
					.occlusionQueryEnable = VK_FALSE, .queryFlags = 0,
					.pipelineStatistics = 0,
				};
				const VkCommandBufferBeginInfo SCBBI = {
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					.pNext = nullptr,
					.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
					.pInheritanceInfo = &CBII
				};
				VERIFY_SUCCEEDED(vkBeginCommandBuffer(SCB, &SCBBI)); {
					vkCmdBindPipeline(SCB, VK_PIPELINE_BIND_POINT_GRAPHICS, PL);

					//!< �`�斈�Ƀ��j�t�H�[���o�b�t�@�̃A�N�Z�X����I�t�Z�b�g���� [Offset uniform buffer access on each draw]
					const auto DynamicOffset = GetViewportMax() * sizeof(ViewProjectionBuffer.ViewProjection[0]);
					//!< �A���C�����g���K�v
					VkMemoryRequirements MR;
					vkGetBufferMemoryRequirements(Device, UniformBuffers[0].Buffer, &MR);

					//!< �L���g�p�^�[���`�� (�r���[�|�[�g�����`�搔�ɐ���������ׁA�v��������s) [Because viewport max is 16, need to draw few times]
					for (uint32_t j = 0; j < GetViewportDrawCount(); ++j) {
						const auto Offset = GetViewportSetOffset(j);
						const auto Count = GetViewportSetCount(j);

						vkCmdSetViewport(SCB, 0, Count, &QuiltViewports[Offset]);
						vkCmdSetScissor(SCB, 0, Count, &QuiltScissorRects[Offset]);

						const std::array DSs = { DS };
						const std::array DynamicOffsets = { static_cast<uint32_t>(RoundUp(DynamicOffset * j, MR.alignment)) };
						vkCmdBindDescriptorSets(SCB, VK_PIPELINE_BIND_POINT_GRAPHICS, PLL, 0, static_cast<uint32_t>(size(DSs)), data(DSs), static_cast<uint32_t>(size(DynamicOffsets)), data(DynamicOffsets));

						const std::array VBs = { VertexBuffers[0].Buffer };
						const std::array NBs = { VertexBuffers[1].Buffer };
						const std::array Offsets = { VkDeviceSize(0) };
						vkCmdBindVertexBuffers(SCB, 0, static_cast<uint32_t>(size(VBs)), data(VBs), data(Offsets));
						vkCmdBindVertexBuffers(SCB, 1, static_cast<uint32_t>(size(NBs)), data(NBs), data(Offsets));
						vkCmdBindIndexBuffer(SCB, IndexBuffers[0].Buffer, 0, VK_INDEX_TYPE_UINT32);

						vkCmdDrawIndexedIndirect(SCB, IndirectBuffers[0].Buffer, 0, 1, 0);
					}
				} VERIFY_SUCCEEDED(vkEndCommandBuffer(SCB));
			}
			//!<�y�p�X1�z[Pass1]
			{
				const auto RP = RenderPasses[1];
				const auto PL = Pipelines[1];
				const auto PLL = PipelineLayouts[1];
				const auto SCB = SecondaryCommandBuffers[1];
				const auto DS = DescriptorSets[1];

				const VkCommandBufferInheritanceInfo CBII = {
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
					.pNext = nullptr,
					.renderPass = RP,
					.subpass = 0,
					.framebuffer = VK_NULL_HANDLE,
					.occlusionQueryEnable = VK_FALSE, .queryFlags = 0,
					.pipelineStatistics = 0,
				};
				const VkCommandBufferBeginInfo SCBBI = {
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					.pNext = nullptr,
					.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
					.pInheritanceInfo = &CBII
				};
				VERIFY_SUCCEEDED(vkBeginCommandBuffer(SCB, &SCBBI)); {
					vkCmdBindPipeline(SCB, VK_PIPELINE_BIND_POINT_GRAPHICS, PL);

					vkCmdSetViewport(SCB, 0, static_cast<uint32_t>(size(Viewports)), data(Viewports));
					vkCmdSetScissor(SCB, 0, static_cast<uint32_t>(size(ScissorRects)), data(ScissorRects));

					const std::array DSs = { DS };
					constexpr std::array<uint32_t, 0> DynamicOffsets = {};
					vkCmdBindDescriptorSets(SCB, VK_PIPELINE_BIND_POINT_GRAPHICS, PLL, 0, static_cast<uint32_t>(size(DSs)), data(DSs), static_cast<uint32_t>(size(DynamicOffsets)), data(DynamicOffsets));

					vkCmdDrawIndirect(SCB, IndirectBuffers[1].Buffer, 0, 1, 0);
				} VERIFY_SUCCEEDED(vkEndCommandBuffer(SCB));
			}
		}
	}
	virtual void PopulateCommandBuffer(const size_t i) override {
		const auto CB = CommandBuffers[i];

		constexpr VkCommandBufferBeginInfo CBBI = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CB, &CBBI)); {
			//!<�y�p�X0�z[Pass0]
			{
				const auto RP = RenderPasses[0];
				const auto FB = Framebuffers[0];
				const auto SCB = SecondaryCommandBuffers[0];

				constexpr std::array CVs = { VkClearValue({.color = Colors::SkyBlue }), VkClearValue({.depthStencil = {.depth = 1.0f, .stencil = 0 } }) };
				const VkRenderPassBeginInfo RPBI = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.pNext = nullptr,
					.renderPass = RP,
					.framebuffer = FB,
					.renderArea = VkRect2D({.offset = VkOffset2D({.x = 0, .y = 0 }), .extent = VkExtent2D({.width = QuiltWidth, .height = QuiltHeight})}), //!< �L���g�T�C�Y
					.clearValueCount = static_cast<uint32_t>(size(CVs)), .pClearValues = data(CVs)
				};
				vkCmdBeginRenderPass(CB, &RPBI, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS); {
					const std::array SCBs = { SCB };
					vkCmdExecuteCommands(CB, static_cast<uint32_t>(size(SCBs)), data(SCBs));
				} vkCmdEndRenderPass(CB);
			}

			//!< �o���A [Barrier]
			//!< �J���[�A�^�b�`�����g ���� �V�F�[�_���[�h��
			ImageMemoryBarrier(CB,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				RenderTextures[0].Image);

			//!<�y�p�X1�z[Pass1]
			{
				const auto RP = RenderPasses[1];
				const auto FB = Framebuffers[1 + i];
				const auto SCB = SecondaryCommandBuffers[1];

				constexpr std::array<VkClearValue, 0> CVs = {};
				const VkRenderPassBeginInfo RPBI = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.pNext = nullptr,
					.renderPass = RP,
					.framebuffer = FB,
					.renderArea = VkRect2D({.offset = VkOffset2D({.x = 0, .y = 0 }), .extent = SurfaceExtent2D }), //!< �X�N���[���T�C�Y
					.clearValueCount = static_cast<uint32_t>(size(CVs)), .pClearValues = data(CVs)
				};
				vkCmdBeginRenderPass(CB, &RPBI, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS); {
					const std::array SCBs = { SCB };
					vkCmdExecuteCommands(CB, static_cast<uint32_t>(size(SCBs)), data(SCBs));
				} vkCmdEndRenderPass(CB);				
			}
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CB));
	}

	virtual uint32_t GetViewportMax() const override { 
		VkPhysicalDeviceProperties PDP;
		vkGetPhysicalDeviceProperties(CurrentPhysicalDevice, &PDP);
		return PDP.limits.maxViewports;
	}
	virtual void CreateProjectionMatrix(const int i) override {
		if (-1 == i) { ProjectionMatrices.clear(); return; }

		//!< ���E�����ɂ���Ă���p�x(���W�A��)
		const auto OffsetAngle = (static_cast<float>(i) / (LenticularBuffer.Column * LenticularBuffer.Row - 1.0f) - 0.5f) * ViewCone;
		//!< ���E�����ɂ���Ă��鋗��
		const auto OffsetX = CameraDistance * std::tan(OffsetAngle);

		auto Prj = glm::perspective(Fov, LenticularBuffer.DisplayAspect, 0.1f, 100.0f);
		Prj[2][0] += OffsetX / (CameraSize * LenticularBuffer.DisplayAspect);

		ProjectionMatrices.emplace_back(Prj);
	}
	virtual void CreateViewMatrix(const int i) override {
		if (-1 == i) { ViewMatrices.clear(); return; }

		const auto OffsetAngle = (static_cast<float>(i) / (LenticularBuffer.Column * LenticularBuffer.Row - 1.0f) - 0.5f) * ViewCone;
		const auto OffsetX = CameraDistance * std::tan(OffsetAngle);

		const auto OffsetLocal = glm::vec3(View * glm::vec4(OffsetX, 0.0f, CameraDistance, 1.0f));
		ViewMatrices.emplace_back(glm::translate(View, OffsetLocal));
	}
	virtual void UpdateViewProjectionBuffer() {
		const auto Count = (std::min)(static_cast<size_t>(LenticularBuffer.Column * LenticularBuffer.Row), _countof(ViewProjectionBuffer.ViewProjection));
		for (auto i = 0; i < Count; ++i) {
			ViewProjectionBuffer.ViewProjection[i] = ProjectionMatrices[i] * ViewMatrices[i];
		}
	}

protected:
	std::vector<uint32_t> Indices;
	std::vector<glm::vec3> Vertices;
	std::vector<glm::vec3> Normals;

	std::vector<VkViewport> QuiltViewports;
	std::vector<VkRect2D> QuiltScissorRects;

	std::vector<glm::mat4> ProjectionMatrices;
	glm::mat4 View;
	std::vector<glm::mat4> ViewMatrices;

	struct VIEW_PROJECTION_BUFFER {
		glm::mat4 ViewProjection[64]; //!< 64 ������Ώ\�� [64 will be enough]
	};
	VIEW_PROJECTION_BUFFER ViewProjectionBuffer;
};