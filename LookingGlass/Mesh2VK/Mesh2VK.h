#pragma once

#include "resource.h"

#include "../VK.h"
#include "../Holo.h"
#include "../FBX.h"

class Mesh2VK : public VK, public Holo, public Fbx
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
		UpdateWorldBuffer();

		VK::OnCreate(hWnd, hInstance, Title);
	}
	virtual void DrawFrame(const UINT i) override {
		UpdateWorldBuffer();
		//CopyToHostVisibleDeviceMemory(Device, UniformBuffers[i].DeviceMemory, 0, sizeof(WorldBuffer), &WorldBuffer);
		CopyToHostVisibleDeviceMemory(Device, UniformBuffers[i].DeviceMemory, 0, sizeof(WorldBuffer.World[0]) * GetInstanceCount(), &WorldBuffer);
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
		AdjustScale(Vertices, 3.0f);

		FbxArray<FbxVector4> Nrms;
		Mesh->GetPolygonVertexNormals(Nrms);
		for (auto i = 0; i < Nrms.Size(); ++i) {
			Normals.emplace_back(ToVec3(Nrms[i]));
		}
	}

	virtual void AllocateCommandBuffer() override {
		VK::AllocateCommandBuffer();
		VK::AllocateSecondaryCommandBuffer(size(SwapchainBackBuffers) + 1);
	}

	virtual void CreateGeometry() override {
		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;
		const auto CB = CommandBuffers[0];

		//Load(std::filesystem::path("..") / "Asset" / "bunny.FBX");
		Load(std::filesystem::path("..") / "Asset" / "dragon.FBX");

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
			.instanceCount = GetInstanceCount(),
			.firstIndex = 0,
			.vertexOffset = 0,
			.firstInstance = 0
		};
		IndirectBuffers.emplace_back().Create(Device, PDMP, DIIC);
		VK::Scoped<StagingBuffer> StagingPass0Indirect(Device);
		StagingPass0Indirect.Create(Device, PDMP, sizeof(DIIC), &DIIC);

		constexpr VkDrawIndirectCommand DIC = {
			.vertexCount = 4,
			.instanceCount = 1,
			.firstVertex = 0,
			.firstInstance = 0
		};
		IndirectBuffers.emplace_back().Create(Device, PDMP, DIC).SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, sizeof(DIC), &DIC);
		VK::Scoped<StagingBuffer> StagingPass1Indirect(Device);
		StagingPass1Indirect.Create(Device, PDMP, sizeof(DIC), &DIC);

		constexpr VkCommandBufferBeginInfo CBBI = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr
		};
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CB, &CBBI)); {
			VertexBuffers[0].PopulateCopyCommand(CB, TotalSizeOf(Vertices), StagingPass0Vertex.Buffer);
			VertexBuffers[1].PopulateCopyCommand(CB, TotalSizeOf(Normals), StagingPass0Normal.Buffer);
			IndexBuffers[0].PopulateCopyCommand(CB, TotalSizeOf(Indices), StagingPass0Index.Buffer);
			IndirectBuffers[0].PopulateCopyCommand(CB, sizeof(DIIC), StagingPass0Indirect.Buffer);

			IndirectBuffers[1].PopulateCopyCommand(CB, sizeof(DIC), StagingPass1Indirect.Buffer);
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CB));
		VK::SubmitAndWait(GraphicsQueue, CB);
	}

	virtual void CreateUniformBuffer() override {
		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;
		for (const auto& i : SwapchainBackBuffers) {
			UniformBuffers.emplace_back().Create(Device, PDMP, sizeof(WorldBuffer));
			CopyToHostVisibleDeviceMemory(Device, UniformBuffers.back().DeviceMemory, 0, sizeof(WorldBuffer), &WorldBuffer);
		}
		for (const auto& i : SwapchainBackBuffers) {
			UniformBuffers.emplace_back().Create(Device, PDMP, sizeof(ViewProjectionBuffer));
			CopyToHostVisibleDeviceMemory(Device, UniformBuffers.back().DeviceMemory, 0, sizeof(ViewProjectionBuffer), &ViewProjectionBuffer);
		}

		UniformBuffers.emplace_back().Create(Device, PDMP, sizeof(LenticularBuffer));
		CopyToHostVisibleDeviceMemory(Device, UniformBuffers.back().DeviceMemory, 0, sizeof(LenticularBuffer), &LenticularBuffer);
	}
	virtual void CreateTexture() override {
		CreateTexture_Render(QuiltWidth, QuiltHeight);
		CreateTexture_Depth(QuiltWidth, QuiltHeight);
	}
	virtual void CreateImmutableSampler() override {
		CreateImmutableSampler_LinearRepeat();
	}
	virtual void CreatePipelineLayout() override {
		{
			CreateDescriptorSetLayout(DescriptorSetLayouts.emplace_back(), 0, {
				VkDescriptorSetLayoutBinding({.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .pImmutableSamplers = nullptr }),
				VkDescriptorSetLayoutBinding({.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT, .pImmutableSamplers = nullptr }),
			});
			VK::CreatePipelineLayout(PipelineLayouts.emplace_back(), { DescriptorSetLayouts[0] }, {});
		}
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
		CreateRenderPass_Depth();
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

		const std::vector VIBDs = {
			VkVertexInputBindingDescription({.binding = 0, .stride = sizeof(Vertices[0]), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }),
			VkVertexInputBindingDescription({.binding = 1, .stride = sizeof(Normals[0]), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }),
		};
		const std::vector VIADs = {
			VkVertexInputAttributeDescription({.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 }),
			VkVertexInputAttributeDescription({.location = 1, .binding = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 }),
		};
		const std::array SMsPass0 = {
			VK::CreateShaderModule(std::filesystem::path(".") / "Mesh2Pass0VK.vert.spv"),
			VK::CreateShaderModule(std::filesystem::path(".") / "Mesh2Pass0VK.frag.spv"),
			VK::CreateShaderModule(std::filesystem::path(".") / "Mesh2Pass0VK.geom.spv"),
		};
		const std::array PSSCIsPass0 = {
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = SMsPass0[0], .pName = "main", .pSpecializationInfo = nullptr }),
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = SMsPass0[1], .pName = "main", .pSpecializationInfo = nullptr }),
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_GEOMETRY_BIT, .module = SMsPass0[2], .pName = "main", .pSpecializationInfo = nullptr }),
		};
		CreatePipelineState_VsFsGs_Input(Pipelines[0], PipelineLayouts[0], RenderPasses[0], VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, PRSCI, VK_TRUE, VIBDs, VIADs, PSSCIsPass0);

		const std::array SMsPass1 = {
			VK::CreateShaderModule(std::filesystem::path(".") / "Mesh2Pass1VK.vert.spv"),
#ifdef DISPLAY_QUILT
			VK::CreateShaderModule(std::filesystem::path(".") / "Mesh2QuiltVK.frag.spv"),
#else
			VK::CreateShaderModule(std::filesystem::path(".") / "Mesh2Pass1VK.frag.spv"),
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
		{
			const auto DescCount = static_cast<uint32_t>(size(SwapchainBackBuffers));

			const auto UB0Index = 0;
			const auto UB1Index = static_cast<uint32_t>(size(SwapchainBackBuffers));
			const auto DSIndex = 0;

			VK::CreateDescriptorPool(DescriptorPools.emplace_back(), 0, {
				VkDescriptorPoolSize({.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = DescCount }),
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
				VkDescriptorBufferInfo DBI0[1];
				VkDescriptorBufferInfo DBI1[1];
			};
			VkDescriptorUpdateTemplate DUT;
			VK::CreateDescriptorUpdateTemplate(DUT, VK_PIPELINE_BIND_POINT_GRAPHICS, {
				VkDescriptorUpdateTemplateEntry({
					.dstBinding = 0, .dstArrayElement = 0,
					.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.offset = offsetof(DescriptorUpdateInfo, DBI0), .stride = sizeof(DescriptorUpdateInfo)
				}),
				VkDescriptorUpdateTemplateEntry({
					.dstBinding = 1, .dstArrayElement = 0,
					.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
					.offset = offsetof(DescriptorUpdateInfo, DBI1), .stride = sizeof(DescriptorUpdateInfo)
				}),
			}, DSL);
			const auto DynamicOffset = GetViewportMax() * sizeof(ViewProjectionBuffer.ViewProjection[0]);
			for (uint32_t i = 0; i < DescCount; ++i) {
				const DescriptorUpdateInfo DUI = {
					VkDescriptorBufferInfo({.buffer = UniformBuffers[UB0Index + i].Buffer, .offset = 0, .range = VK_WHOLE_SIZE }),
					VkDescriptorBufferInfo({.buffer = UniformBuffers[UB1Index + i].Buffer, .offset = 0, .range = DynamicOffset }),
				};
				vkUpdateDescriptorSetWithTemplate(Device, DescriptorSets[DSIndex + i], DUT, &DUI);
			}
			vkDestroyDescriptorUpdateTemplate(Device, DUT, GetAllocationCallbacks());
		}
		{
			const auto DescCount = 1;

			const auto UBIndex = static_cast<uint32_t>(size(SwapchainBackBuffers)) * 2;
			const auto DSIndex = static_cast<uint32_t>(size(SwapchainBackBuffers));

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
		VK::CreateFramebuffer(Framebuffers.emplace_back(), RenderPasses[0], QuiltWidth, QuiltHeight, 1, { RenderTextures[0].View, DepthTextures[0].View });
		
		for (const auto& i : SwapchainBackBuffers) {
			VK::CreateFramebuffer(Framebuffers.emplace_back(), RenderPasses[1], SurfaceExtent2D.width, SurfaceExtent2D.height, 1, { i.ImageView });
		}
	}
	virtual void CreateViewport(const float Width, const float Height, const float MinDepth = 0.0f, const float MaxDepth = 1.0f) override {
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

		VK::CreateViewport(Width, Height, MinDepth, MaxDepth);
	}
	virtual void PopulateSecondaryCommandBuffer(const size_t i) override {
		{
			const auto RP = RenderPasses[0];
			const auto PL = Pipelines[0];
			const auto PLL = PipelineLayouts[0];
			const auto SCB = SecondaryCommandBuffers[i];
			const auto DS = DescriptorSets[i];

			VkMemoryRequirements MR;
			vkGetBufferMemoryRequirements(Device, UniformBuffers[size(SwapchainBackBuffers)].Buffer, &MR);

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
				.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
				.pInheritanceInfo = &CBII
			};
			VERIFY_SUCCEEDED(vkBeginCommandBuffer(SCB, &SCBBI)); {
				vkCmdBindPipeline(SCB, VK_PIPELINE_BIND_POINT_GRAPHICS, PL);

				const auto DynamicOffset = GetViewportMax() * sizeof(ViewProjectionBuffer.ViewProjection[0]);
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
		if (0 == i) {
			const auto Index = size(SwapchainBackBuffers);

			const auto RP = RenderPasses[1];
			const auto PL = Pipelines[1];
			const auto PLL = PipelineLayouts[1];

			const auto SCB = SecondaryCommandBuffers[Index];
			const auto DS = DescriptorSets[Index];

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
	virtual void PopulateCommandBuffer(const size_t i) override {
		const auto CB = CommandBuffers[i];

		constexpr VkCommandBufferBeginInfo CBBI = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CB, &CBBI)); {
			{
				const auto RP = RenderPasses[0];
				const auto FB = Framebuffers[0];
				const auto SCB = SecondaryCommandBuffers[i];

				constexpr std::array CVs = { VkClearValue({.color = Colors::SkyBlue }), VkClearValue({.depthStencil = {.depth = 1.0f, .stencil = 0 } }) };
				const VkRenderPassBeginInfo RPBI = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.pNext = nullptr,
					.renderPass = RP,
					.framebuffer = FB,
					.renderArea = VkRect2D({.offset = VkOffset2D({.x = 0, .y = 0 }), .extent = VkExtent2D({.width = QuiltWidth, .height = QuiltHeight})}),
					.clearValueCount = static_cast<uint32_t>(size(CVs)), .pClearValues = data(CVs)
				};
				vkCmdBeginRenderPass(CB, &RPBI, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS); {
					const std::array SCBs = { SCB };
					vkCmdExecuteCommands(CB, static_cast<uint32_t>(size(SCBs)), data(SCBs));
				} vkCmdEndRenderPass(CB);
			}

			ImageMemoryBarrier(CB,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				RenderTextures[0].Image);

			{
				const auto RP = RenderPasses[1];
				const auto FB = Framebuffers[i + 1];
				const auto SCB = SecondaryCommandBuffers[size(SwapchainBackBuffers)];

				constexpr std::array<VkClearValue, 0> CVs = {};
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

		const auto OffsetAngle = (static_cast<float>(i) / (LenticularBuffer.Column * LenticularBuffer.Row - 1.0f) - 0.5f) * ViewCone;
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
			ViewProjectionBuffer.ViewProjection[i].View = ViewMatrices[i];
			ViewProjectionBuffer.ViewProjection[i].Projection = ProjectionMatrices[i];
		}
	}
	virtual void UpdateWorldBuffer() {
		Angle += 1.0f;
		while (Angle > 360.0f) { Angle -= 360.0f; }
		while (Angle < 0.0f) { Angle += 360.0f; }

		const auto Identity = glm::mat4(1.0f);
		const auto AxisX = glm::vec3(1.0f, 0.0f, 0.0f);
		const auto AxisY = glm::vec3(0.0f, 1.0f, 0.0f);
		
		const auto Count = GetInstanceCount();
		constexpr auto Radius = 2.0f;
		constexpr auto Height = 10.0f;
		const auto OffsetY = Height / static_cast<float>(Count);
		for (uint32_t i = 0; i < Count; ++i) {
			const auto Index = static_cast<float>(i);
			const auto Radian = glm::radians(Index * 90.0f);
			const auto X = Radius * cos(Radian);
			const auto Y = OffsetY * (Index - static_cast<float>(Count) * 0.5f);
			const auto Z = Radius * sin(Radian);

			//const auto RotL = glm::rotate(Identity, glm::radians(i * 45.0f + Angle), AxisX);
			const auto RotL = Identity;
			const auto RotG = glm::rotate(Identity, glm::radians(i * 45.0f + Angle), AxisY);
			WorldBuffer.World[i] = glm::translate(RotG, glm::vec3(X, Y, Z)) * RotL;
		}
	}

	uint32_t GetInstanceCount() const { return (std::min)(InstanceCount, static_cast<uint32_t>(_countof(WorldBuffer.World))); }

protected:
	std::vector<uint32_t> Indices;
	std::vector<glm::vec3> Vertices;
	std::vector<glm::vec3> Normals;

	std::vector<VkViewport> QuiltViewports;
	std::vector<VkRect2D> QuiltScissorRects;

	std::vector<glm::mat4> ProjectionMatrices;
	glm::mat4 View;
	std::vector<glm::mat4> ViewMatrices;

	struct VIEW_PROJECTION {
		glm::mat4 View;
		glm::mat4 Projection;
	};
	struct VIEW_PROJECTION_BUFFER {
		VIEW_PROJECTION ViewProjection[64];
	};
	VIEW_PROJECTION_BUFFER ViewProjectionBuffer;

	struct WORLD_BUFFER {
		glm::mat4 World[16];
	};
	WORLD_BUFFER WorldBuffer;

	float Angle = 0.0f;

	const uint32_t InstanceCount = 16;
};