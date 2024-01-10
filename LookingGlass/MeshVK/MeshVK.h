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
			const auto Pos = glm::vec3(0.0f, 0.0f, 3.0f);
			const auto Tag = glm::vec3(0.0f);
			const auto Up = glm::vec3(0.0f, 1.0f, 0.0f);
			View = glm::lookAt(Pos, Tag, Up);
			CreateViewMatrices();
		}
		updateViewProjectionBuffer();

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
		AdjustScale(Vertices, 1.0f);

		FbxArray<FbxVector4> Nrms;
		Mesh->GetPolygonVertexNormals(Nrms);
		for (auto i = 0; i < Nrms.Size(); ++i) {
			Normals.emplace_back(ToVec3(Nrms[i]));
		}
	}

	virtual void AllocateCommandBuffer() override {
		//!<【パス0】コマンドバッファ
		VK::AllocateCommandBuffer();
		//!<【パス1】セカンダリコマンドバッファ
		VK::AllocateSecondaryCommandBuffer(2);
	}

	virtual void CreateGeometry() override {
		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;
		const auto CB = CommandBuffers[0];

		//!<【パス0】メッシュ描画用
		Load(std::filesystem::path("..") / "Asset" / "bunny.FBX");
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

		//!<【パス0】レンダーテクスチャ描画用
		constexpr VkDrawIndirectCommand DIC = { 
			.vertexCount = 4, 
			.instanceCount = 1, 
			.firstVertex = 0,
			.firstInstance = 0 
		};
		IndirectBuffers.emplace_back().Create(Device, PDMP, DIC).SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, sizeof(DIC), &DIC);
		VK::Scoped<StagingBuffer> StagingPass1Indirect(Device);
		StagingPass1Indirect.Create(Device, PDMP, sizeof(DIC), &DIC);

		//!< コピーコマンド発行
		constexpr VkCommandBufferBeginInfo CBBI = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 
			.pNext = nullptr, 
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 
			.pInheritanceInfo = nullptr 
		};
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CB, &CBBI)); {
			//!< 【パス0】
			VertexBuffers[0].PopulateCopyCommand(CB, TotalSizeOf(Vertices), StagingPass0Vertex.Buffer);
			VertexBuffers[1].PopulateCopyCommand(CB, TotalSizeOf(Normals), StagingPass0Normal.Buffer);
			IndexBuffers[0].PopulateCopyCommand(CB, TotalSizeOf(Indices), StagingPass0Index.Buffer);
			IndirectBuffers[0].PopulateCopyCommand(CB, sizeof(DIIC), StagingPass0Indirect.Buffer);

			//!< 【パス1】
			IndirectBuffers[1].PopulateCopyCommand(CB, sizeof(DIC), StagingPass1Indirect.Buffer);
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CB));
		VK::SubmitAndWait(GraphicsQueue, CB);
	}

	virtual void CreateUniformBuffer() override {
		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;

		//!<【パス0】
		UniformBuffers.emplace_back().Create(Device, PDMP, sizeof(ViewProjectionBuffer));
		CopyToHostVisibleDeviceMemory(Device, UniformBuffers[0].DeviceMemory, 0, sizeof(ViewProjectionBuffer), &ViewProjectionBuffer);

		//!<【パス1】
		UniformBuffers.emplace_back().Create(Device, PDMP, sizeof(*LenticularBuffer));
		CopyToHostVisibleDeviceMemory(Device, UniformBuffers[1].DeviceMemory, 0, sizeof(*LenticularBuffer), LenticularBuffer);
	}
	virtual void CreateTexture() override {
		//!<【パス0】レンダーターゲット、デプス (キルトサイズ)
		CreateTexture_Render(QuiltWidth, QuiltHeight);
		CreateTexture_Depth(QuiltWidth, QuiltHeight);
	}
	virtual void CreateImmutableSampler() override {
		//!< 【パス1】
		CreateImmutableSampler_LinearRepeat();
	}
	virtual void CreatePipelineLayout() override {
		//!<【パス0】
		{
			CreateDescriptorSetLayout(DescriptorSetLayouts.emplace_back(), 0, {
				VkDescriptorSetLayoutBinding({.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT, .pImmutableSamplers = nullptr }),
			});
			VK::CreatePipelineLayout(PipelineLayouts.emplace_back(), { DescriptorSetLayouts[0] }, {});
		}

		//!<【パス1】
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
		//!< 【パス0】
		CreateRenderPass_Depth();
		//!< 【パス1】
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
		
		//!<【パス0】
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
		
		//!<【パス1】
		const std::array SMsPass1 = {
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass1VK.vert.spv"),
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass1VK.frag.spv"),
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
		//!< 【パス0】
		{
			VK::CreateDescriptorPool(DescriptorPools.emplace_back(), 0, {
				VkDescriptorPoolSize({.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1 }),
			});
			auto DSL = DescriptorSetLayouts[0];
			auto DP = DescriptorPools[0];
			const std::array DSLs = { DSL };
			const VkDescriptorSetAllocateInfo DSAI = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.pNext = nullptr,
				.descriptorPool = DP,
				.descriptorSetCount = static_cast<uint32_t>(size(DSLs)), .pSetLayouts = data(DSLs)
			};
			VERIFY_SUCCEEDED(vkAllocateDescriptorSets(Device, &DSAI, &DescriptorSets.emplace_back()));

			struct DescriptorUpdateInfo
			{
				VkDescriptorBufferInfo DBI[1];
			};
			VkDescriptorUpdateTemplate DUT;
			VK::CreateDescriptorUpdateTemplate(DUT, VK_PIPELINE_BIND_POINT_GRAPHICS, {
				VkDescriptorUpdateTemplateEntry({
					.dstBinding = 0, .dstArrayElement = 0,
					.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.offset = offsetof(DescriptorUpdateInfo, DBI), .stride = sizeof(DescriptorUpdateInfo)
				}),
			}, DSL);
			const DescriptorUpdateInfo DUI = {
				VkDescriptorBufferInfo({.buffer = UniformBuffers[0].Buffer, .offset = 0, .range = VK_WHOLE_SIZE}),
			};
			vkUpdateDescriptorSetWithTemplate(Device, DescriptorSets[0], DUT, &DUI);
			vkDestroyDescriptorUpdateTemplate(Device, DUT, GetAllocationCallbacks());
		}

		//!< 【パス1】
		{
			VK::CreateDescriptorPool(DescriptorPools.emplace_back(), 0, {
				VkDescriptorPoolSize({.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1 }),
				VkDescriptorPoolSize({.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1 }),
			});

			auto DSL = DescriptorSetLayouts[1];
			auto DP = DescriptorPools[1];
			const std::array DSLs = { DSL };
			const VkDescriptorSetAllocateInfo DSAI = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.pNext = nullptr,
				.descriptorPool = DP,
				.descriptorSetCount = static_cast<uint32_t>(size(DSLs)), .pSetLayouts = data(DSLs)
			};
			VERIFY_SUCCEEDED(vkAllocateDescriptorSets(Device, &DSAI, &DescriptorSets.emplace_back()));

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
			const DescriptorUpdateInfo DUI = {
				VkDescriptorImageInfo({.sampler = VK_NULL_HANDLE, .imageView = RenderTextures[0].View, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }),
				VkDescriptorBufferInfo({.buffer = UniformBuffers[1].Buffer, .offset = 0, .range = VK_WHOLE_SIZE}),
			};
			vkUpdateDescriptorSetWithTemplate(Device, DescriptorSets[1], DUT, &DUI);
			vkDestroyDescriptorUpdateTemplate(Device, DUT, GetAllocationCallbacks());
		}
	}
	virtual void CreateFramebuffer() override {
		//!< 【パス0】
		//!< レンダーターゲット (キルトサイズ)
		VK::CreateFramebuffer(Framebuffers.emplace_back(), RenderPasses[0], QuiltWidth, QuiltHeight, 1, { RenderTextures[0].View, DepthTextures[0].View });

		//!<【パス1】
		//!< スワップチェイン (スクリーンサイズ)
		for (auto i : SwapchainImageViews) {
			VK::CreateFramebuffer(Framebuffers.emplace_back(), RenderPasses[1], SurfaceExtent2D.width, SurfaceExtent2D.height, 1, { i });
		}
	}
	virtual void CreateViewport(const float Width, const float Height, const float MinDepth = 0.0f, const float MaxDepth = 1.0f) override {
		//!<【パス0】キルトレンダーターゲットを分割
		const auto W = QuiltWidth / LenticularBuffer->Column, H = QuiltHeight / LenticularBuffer->Row;
		const auto Ext2D = VkExtent2D({ .width = static_cast<uint32_t>(W), .height = static_cast<uint32_t>(H) });
		for (auto i = 0; i < LenticularBuffer->Row; ++i) {
			const auto Y = QuiltHeight - H * i;
			for (auto j = 0; j < LenticularBuffer->Column; ++j) {
				const auto X = j * W;
				QuiltViewports.emplace_back(VkViewport({ .x = X, .y = Y, .width = W, .height = -H, .minDepth = MinDepth, .maxDepth = MaxDepth }));
				QuiltScissorRects.emplace_back(VkRect2D({ VkOffset2D({.x = static_cast<int32_t>(X), .y = static_cast<int32_t>(Y - H) }), Ext2D }));
			}
		}

		//!<【パス1】スクリーンを使用
		VK::CreateViewport(Width, Height, MinDepth, MaxDepth);
	}
	virtual void PopulateSecondaryCommandBuffer(const size_t i) override {
		const auto RP0 = RenderPasses[0];
		const auto RP1 = RenderPasses[1];
		const auto FB0 = Framebuffers[0];
		const auto FB1 = Framebuffers[1];

		//!<【パス0】セカンダリコマンドバッファ
		const auto SCB0 = SecondaryCommandBuffers[0];
		{
			const auto PL = Pipelines[0];
			const VkCommandBufferInheritanceInfo CBII = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
				.pNext = nullptr,
				.renderPass = RP0,
				.subpass = 0,
				.framebuffer = FB0,
				.occlusionQueryEnable = VK_FALSE, .queryFlags = 0,
				.pipelineStatistics = 0,
			};
			const VkCommandBufferBeginInfo SCBBI = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = nullptr,
				.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
				.pInheritanceInfo = &CBII
			};
			VERIFY_SUCCEEDED(vkBeginCommandBuffer(SCB0, &SCBBI)); {
				vkCmdBindPipeline(SCB0, VK_PIPELINE_BIND_POINT_GRAPHICS, PL);

				const std::array VBs = { VertexBuffers[0].Buffer };
				const std::array NBs = { VertexBuffers[1].Buffer };
				const std::array Offsets = { VkDeviceSize(0) };
				vkCmdBindVertexBuffers(SCB0, 0, static_cast<uint32_t>(size(VBs)), data(VBs), data(Offsets));
				vkCmdBindVertexBuffers(SCB0, 1, static_cast<uint32_t>(size(NBs)), data(NBs), data(Offsets));
				vkCmdBindIndexBuffer(SCB0, IndexBuffers[0].Buffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdDrawIndexedIndirect(SCB0, IndirectBuffers[0].Buffer, 0, 1, 0);
			} VERIFY_SUCCEEDED(vkEndCommandBuffer(SCB0));
		}

		//!<【パス1】セカンダリコマンドバッファ
		const auto SCB1 = SecondaryCommandBuffers[1];
		{
			const auto PL = Pipelines[1];
			const VkCommandBufferInheritanceInfo CBII = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
				.pNext = nullptr,
				.renderPass = RP1,
				.subpass = 0,
				.framebuffer = FB1,
				.occlusionQueryEnable = VK_FALSE, .queryFlags = 0,
				.pipelineStatistics = 0,
			};
			const VkCommandBufferBeginInfo SCBBI = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = nullptr,
				.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
				.pInheritanceInfo = &CBII
			};
			VERIFY_SUCCEEDED(vkBeginCommandBuffer(SCB1, &SCBBI)); {
				vkCmdSetViewport(SCB1, 0, static_cast<uint32_t>(size(Viewports)), data(Viewports));
				vkCmdSetScissor(SCB1, 0, static_cast<uint32_t>(size(ScissorRects)), data(ScissorRects));

				vkCmdBindPipeline(SCB1, VK_PIPELINE_BIND_POINT_GRAPHICS, PL);

				vkCmdDrawIndirect(SCB1, IndirectBuffers[1].Buffer, 0, 1, 0);
			} VERIFY_SUCCEEDED(vkEndCommandBuffer(SCB1));
		}

	}
	virtual void PopulateCommandBuffer(const size_t i) override {
		const auto RP0 = RenderPasses[0];
		const auto RP1 = RenderPasses[1];
		const auto FB0 = Framebuffers[0];
		const auto FB1 = Framebuffers[1 + i];

		const auto SCB0 = SecondaryCommandBuffers[0];
		const auto SCB1 = SecondaryCommandBuffers[1];

		//!< コマンドバッファ
		const auto CB = CommandBuffers[i];
		constexpr VkCommandBufferBeginInfo CBBI = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CB, &CBBI)); {
			//!<【パス0】
			constexpr std::array CVs = { VkClearValue({.color = Colors::SkyBlue }), VkClearValue({.depthStencil = {.depth = 1.0f, .stencil = 0 } }) };
			const VkRenderPassBeginInfo RPBI = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.pNext = nullptr,
				.renderPass = RP0,
				.framebuffer = FB0,
				.renderArea = VkRect2D({.offset = VkOffset2D({.x = 0, .y = 0 }), .extent = VkExtent2D({.width = QuiltWidth, .height = QuiltHeight})}), //!< キルトサイズ
				.clearValueCount = static_cast<uint32_t>(size(CVs)), .pClearValues = data(CVs)
			};
			vkCmdBeginRenderPass(CB, &RPBI, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS); {

				//!< デスクリプタ
				{
					const auto PLL = PipelineLayouts[0];

					const std::array DSs = { DescriptorSets[0] };
					constexpr std::array<uint32_t, 0> DynamicOffsets = {};
					vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, PLL, 0, static_cast<uint32_t>(size(DSs)), data(DSs), static_cast<uint32_t>(size(DynamicOffsets)), data(DynamicOffsets));
				}

				//!< キルトパターン描画 (ビューポート同時描画数に制限がある為、要複数回実行)
				for (uint32_t j = 0; j < GetViewportDrawCount(); ++j) {
					const auto Offset = GetViewportSetOffset(j);
					const auto Count = GetViewportSetCount(j);

					vkCmdSetViewport(CB, 0, Count, &QuiltViewports[Offset]);
					vkCmdSetScissor(CB, 0, Count, &QuiltScissorRects[Offset]);

					const std::array SCBs = { SCB0 };
					vkCmdExecuteCommands(CB, static_cast<uint32_t>(size(SCBs)), data(SCBs));
				}
			} vkCmdEndRenderPass(CB);

			//!< バリア
			//!< カラーアタッチメント から シェーダリードへ
			ImageMemoryBarrier(CB,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				RenderTextures[0].Image);

			//!<【パス1】
			{
				constexpr std::array<VkClearValue, 0> CVs = {};
				const VkRenderPassBeginInfo RPBI = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.pNext = nullptr,
					.renderPass = RP1,
					.framebuffer = FB1,
					.renderArea = VkRect2D({.offset = VkOffset2D({.x = 0, .y = 0 }), .extent = SurfaceExtent2D }), //!< スクリーンサイズ
					.clearValueCount = static_cast<uint32_t>(size(CVs)), .pClearValues = data(CVs)
				};
				vkCmdBeginRenderPass(CB, &RPBI, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS); {

					//!< デスクリプタ
					{
						const auto PLL = PipelineLayouts[1];

						const std::array DSs = { DescriptorSets[1] };
						constexpr std::array<uint32_t, 0> DynamicOffsets = {};
						vkCmdBindDescriptorSets(CB, VK_PIPELINE_BIND_POINT_GRAPHICS, PLL, 0, static_cast<uint32_t>(size(DSs)), data(DSs), static_cast<uint32_t>(size(DynamicOffsets)), data(DynamicOffsets));
					}

					const std::array SCBs = { SCB1 };
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

		//!< 左右方向にずれている角度(ラジアン)
		const auto OffsetAngle = (static_cast<float>(i) / (LenticularBuffer->Column * LenticularBuffer->Row - 1.0f) - 0.5f) * ViewCone;
		//!< 左右方向にずれている距離
		const auto OffsetX = CameraDistance * std::tan(OffsetAngle);

		auto Prj = glm::perspective(Fov, LenticularBuffer->DisplayAspect, 0.1f, 100.0f);
		Prj[2][0] += OffsetX / (CameraSize * LenticularBuffer->DisplayAspect);

		ProjectionMatrices.emplace_back(Prj);
	}
	virtual void CreateViewMatrix(const int i) override {
		if (-1 == i) { ViewMatrices.clear(); return; }

		const auto OffsetAngle = (static_cast<float>(i) / (LenticularBuffer->Column * LenticularBuffer->Row - 1.0f) - 0.5f) * ViewCone;
		const auto OffsetX = CameraDistance * std::tan(OffsetAngle);

		const auto OffsetLocal = glm::vec3(View * glm::vec4(OffsetX, 0.0f, CameraDistance, 1.0f));
		ViewMatrices.emplace_back(glm::translate(View, OffsetLocal));
	}
	virtual void updateViewProjectionBuffer() {
		const auto ColRow = static_cast<int>(LenticularBuffer->Column * LenticularBuffer->Row);
		for (auto i = 0; i < ColRow; ++i) {
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
		glm::mat4 ViewProjection[64];
	};
	VIEW_PROJECTION_BUFFER ViewProjectionBuffer;

	//struct WORLD_BUFFER {
	//	glm::mat4 World;
	//};
	//WORLD_BUFFER WorldBuffer;
};