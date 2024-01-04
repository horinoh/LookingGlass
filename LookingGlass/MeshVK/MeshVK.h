#pragma once

#include "resource.h"

#include "../VK.h"
#include "../Holo.h"
#include "../FBX.h"

class MeshVK : public VK, public Holo, public Fbx
{
public:
	std::vector<uint32_t> Indices;
	std::vector<glm::vec3> Vertices;
	std::vector<glm::vec3> Normals;
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
		//!<【パス0】プライマリ、セカンダリコマンドバッファ
		VK::AllocateCommandBuffer();

		//!<【パス1】セカンダリコマンドバッファ
		const auto SCP = SecondaryCommandPools[0];
		const auto Count = static_cast<uint32_t>(size(SwapchainImages));
		const auto Prev = size(SecondaryCommandBuffers);
		SecondaryCommandBuffers.resize(Prev + Count);
		const VkCommandBufferAllocateInfo CBAI = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = SCP,
			.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			.commandBufferCount = Count
		};
		VERIFY_SUCCEEDED(vkAllocateCommandBuffers(Device, &CBAI, &SecondaryCommandBuffers[Prev]));
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

		//!< コマンド発行
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
	}
	virtual void CreateTexture() override {
		//!<【パス0】レンダーターゲット、デプス
		CreateTexture_Render(QuiltWidth, QuiltHeight);
		CreateTexture_Depth(QuiltWidth, QuiltHeight);
	}
	virtual void CreateImmutableSampler() override {
		//!< 【パス1】
		CreateImmutableSampler_LinearRepeat();
	}
	virtual void CreatePipelineLayout() override {
		//!< #TODO
	}
	virtual void CreateRenderPass() override { 
		//!< 【パス0】
		CreateRenderPass_Depth();
		//!< 【パス1】
		CreateRenderPass_None();
	}
	virtual void CreatePipeline() override {
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
		
		//!< 【パス0】
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
			//VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass0VK.geom.spv"),
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass0VK.frag.spv"),
		};
		const std::array PSSCIsPass0 = {
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = SMsPass0[0], .pName = "main", .pSpecializationInfo = nullptr }),
			//VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_GEOMETRY_BIT, .module = SMsPass0[1], .pName = "main", .pSpecializationInfo = nullptr }),
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = SMsPass0[1/*2*/], .pName = "main", .pSpecializationInfo = nullptr}),
		};
		CreatePipeline_VsFs_Input(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, PRSCI, VK_TRUE, VIBDs, VIADs, PSSCIsPass0);
		//CreatePipelineState_VsGsFs_Input(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, PRSCI, VK_TRUE, VIBDs, VIADs, PSSCIsPass0);
		for (auto i : SMsPass0) { vkDestroyShaderModule(Device, i, GetAllocationCallbacks()); }

		//!< 【パス1】
		const std::array SMsPass1 = {
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass0VK.vert.spv"),
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass0VK.frag.spv"),
		};
		const std::array PSSCIsPass1 = {
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = SMsPass1[0], .pName = "main", .pSpecializationInfo = nullptr }),
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = SMsPass1[1], .pName = "main", .pSpecializationInfo = nullptr }),
		};
		CreatePipeline_VsFs(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0, PRSCI, VK_FALSE, PSSCIsPass1);
		for (auto i : SMsPass1) { vkDestroyShaderModule(Device, i, GetAllocationCallbacks()); }
	}
	virtual void CreateDescriptor() override {
		//!< #TODO
	}
	virtual void CreateFramebuffer() override {
#if 0
		//!< 【パス0】
		VK::CreateFramebuffer(Framebuffers.emplace_back(), RenderPasses[0], QuiltWidth, QuiltHeight, 1, { RenderTextures[0].View, DepthTextures[0].View });

		//!< 【パス1】
		for (auto i : SwapchainImageViews) {
			VK::CreateFramebuffer(Framebuffers.emplace_back(), RenderPasses[1], SurfaceExtent2D.width, SurfaceExtent2D.height, 1, { i });
		}
#else
		for (auto i : SwapchainImageViews) {
			VK::CreateFramebuffer(Framebuffers.emplace_back(), RenderPasses[0], SurfaceExtent2D.width, SurfaceExtent2D.height, 1, { i, DepthTextures[0].View });
		}
#endif
	}
	virtual void PopulateCommandBuffer(const size_t i) override {
		//!<【パス0】セカンダリコマンドバッファ
		const auto RP = RenderPasses[0];
		const auto FB = Framebuffers[i];
		const auto PLL = PipelineLayouts[0];
		const auto PL = Pipelines[0];

		const auto SCB = SecondaryCommandBuffers[i];
		const VkCommandBufferInheritanceInfo CBII = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			.pNext = nullptr,
			.renderPass = RP,
			.subpass = 0,
			.framebuffer = FB,
			.occlusionQueryEnable = VK_FALSE, .queryFlags = 0,
			.pipelineStatistics = 0,
		};
		const VkCommandBufferBeginInfo SCBBI = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
			.pInheritanceInfo = &CBII
		};
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(SCB, &SCBBI)); {
			vkCmdSetViewport(SCB, 0, static_cast<uint32_t>(size(Viewports)), data(Viewports));
			vkCmdSetScissor(SCB, 0, static_cast<uint32_t>(size(ScissorRects)), data(ScissorRects));

			vkCmdBindPipeline(SCB, VK_PIPELINE_BIND_POINT_GRAPHICS, PL);

			const std::array VBs = { VertexBuffers[0].Buffer };
			const std::array NBs = { VertexBuffers[1].Buffer };
			const std::array Offsets = { VkDeviceSize(0) };
			vkCmdBindVertexBuffers(SCB, 0, static_cast<uint32_t>(size(VBs)), data(VBs), data(Offsets));
			vkCmdBindVertexBuffers(SCB, 1, static_cast<uint32_t>(size(NBs)), data(NBs), data(Offsets));
			vkCmdBindIndexBuffer(SCB, IndexBuffers[0].Buffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexedIndirect(SCB, IndirectBuffers[0].Buffer, 0, 1, 0);
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(SCB));

		//!<【パス1】セカンダリコマンドバッファ
		//const auto RP1 = RenderPasses[1];
		//const auto FB1 = Framebuffers[i + 1];
		//const auto PLL1 = PipelineLayouts[0];
		//const auto PL1 = Pipelines[0];

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
				.renderPass = RP,
				.framebuffer = FB,
				.renderArea = VkRect2D({.offset = VkOffset2D({.x = 0, .y = 0 }), .extent = SurfaceExtent2D }),
				.clearValueCount = static_cast<uint32_t>(size(CVs)), .pClearValues = data(CVs)
			};
			vkCmdBeginRenderPass(CB, &RPBI, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS); {
				const std::array SCBs = { SCB };
				vkCmdExecuteCommands(CB, static_cast<uint32_t>(size(SCBs)), data(SCBs));
			} vkCmdEndRenderPass(CB);

			//!< バリア #TODO

			//!<【パス1】#TODO

		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CB));
	}
};