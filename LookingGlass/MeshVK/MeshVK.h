#pragma once

#include "resource.h"

//#define USE_GLTF
#define USE_FBX
#include "../HoloVK.h"

#ifdef USE_GLTF
class MeshVK : public HoloGLTFVK
#else
class MeshVK : public HoloFBXVK
#endif
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
	virtual void AllocateCommandBuffer() override {
		VK::AllocateCommandBuffer();
		VK::AllocateSecondaryCommandBuffer(2);
	}
	virtual void CreateGeometry() override {
		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;
		const auto CB = CommandBuffers[0];

		//!<【Pass0】メッシュ描画用 [To draw mesh] 
#ifdef USE_GLTF
		Load(std::filesystem::path("..") / "Asset" / "Mesh" / "bunny.glb");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "dragon.glb");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "happy_vrip.glb");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "Sphere.glb");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "Box.glb");
#else
		Load(std::filesystem::path("..") / "Asset" / "Mesh" / "bunny.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "dragon.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "happy_vrip.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "Bee.FBX");
		//Load(std::filesystem::path("..") / "Asset" / "Mesh" / "ALucy.FBX");
#endif
		
		VertexBuffers.emplace_back().Create(Device, PDMP, TotalSizeOf(Vertices));
		VK::Scoped<StagingBuffer> StagingPass0Vertex(Device);
		StagingPass0Vertex.Create(Device, PDMP, TotalSizeOf(Vertices), std::data(Vertices));

		VertexBuffers.emplace_back().Create(Device, PDMP, TotalSizeOf(Normals));
		VK::Scoped<StagingBuffer> StagingPass0Normal(Device);
		StagingPass0Normal.Create(Device, PDMP, TotalSizeOf(Normals), std::data(Normals));

		IndexBuffers.emplace_back().Create(Device, PDMP, TotalSizeOf(Indices));
		VK::Scoped<StagingBuffer> StagingPass0Index(Device);
		StagingPass0Index.Create(Device, PDMP, TotalSizeOf(Indices), std::data(Indices));

		const VkDrawIndexedIndirectCommand DIIC = {
			.indexCount = static_cast<uint32_t>(std::size(Indices)),
			.instanceCount = 1, 
			.firstIndex = 0, 
			.vertexOffset = 0, 
			.firstInstance = 0 
		};
		LOG(std::data(std::format("InstanceCount = {}\n", DIIC.instanceCount)));
		IndirectBuffers.emplace_back().Create(Device, PDMP, DIIC);
		VK::Scoped<StagingBuffer> StagingPass0Indirect(Device);
		StagingPass0Indirect.Create(Device, PDMP, sizeof(DIIC), &DIIC);

		//!<【Pass0】レンダーテクスチャ描画用 [To draw render texture]
		constexpr VkDrawIndirectCommand DIC = { 
			.vertexCount = 4, 
			.instanceCount = 1, 
			.firstVertex = 0,
			.firstInstance = 0 
		};
		IndirectBuffers.emplace_back().Create(Device, PDMP, DIC).SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, sizeof(DIC), &DIC);
		VK::Scoped<StagingBuffer> StagingPass1Indirect(Device);
		StagingPass1Indirect.Create(Device, PDMP, sizeof(DIC), &DIC);

		//!< コピーコマンド発行 [Issue upload command]
		constexpr VkCommandBufferBeginInfo CBBI = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 
			.pNext = nullptr, 
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 
			.pInheritanceInfo = nullptr 
		};
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CB, &CBBI)); {
			//!<【Pass0】
			VertexBuffers[0].PopulateCopyCommand(CB, TotalSizeOf(Vertices), StagingPass0Vertex.Buffer);
			VertexBuffers[1].PopulateCopyCommand(CB, TotalSizeOf(Normals), StagingPass0Normal.Buffer);
			IndexBuffers[0].PopulateCopyCommand(CB, TotalSizeOf(Indices), StagingPass0Index.Buffer);
			IndirectBuffers[0].PopulateCopyCommand(CB, sizeof(DIIC), StagingPass0Indirect.Buffer);

			//!<【Pass1】
			IndirectBuffers[1].PopulateCopyCommand(CB, sizeof(DIC), StagingPass1Indirect.Buffer);
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CB));
		VK::SubmitAndWait(GraphicsQueue, CB);
	}

	virtual void CreateUniformBuffer() override {
		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;

		//!<【Pass0】
		UniformBuffers.emplace_back().Create(Device, PDMP, sizeof(ViewProjectionBuffer));
		CopyToHostVisibleDeviceMemory(Device, UniformBuffers.back().DeviceMemory, 0, sizeof(ViewProjectionBuffer), &ViewProjectionBuffer);

		//!<【Pass1】
		UniformBuffers.emplace_back().Create(Device, PDMP, sizeof(LenticularBuffer));
		CopyToHostVisibleDeviceMemory(Device, UniformBuffers.back().DeviceMemory, 0, sizeof(LenticularBuffer), &LenticularBuffer);
	}
	virtual void CreateTexture() override {
		//!<【Pass0】レンダー、デプスターゲット (キルトサイズ) [Render and depth target (quilt size)]
		CreateTexture_Render(QuiltX, QuiltY);
		CreateTexture_Depth(QuiltX, QuiltY);
	}
	virtual void CreateImmutableSampler() override {
		//!< 【Pass1】
		CreateImmutableSampler_LinearRepeat();
	}
	virtual void CreatePipelineLayout() override {
		//!<【Pass0】ダイナミックオフセット (UNIFORM_BUFFER_DYNAMIC) を使用 [Using dynamic offset (UNIFORM_BUFFER_DYNAMIC)]
		{
			CreateDescriptorSetLayout(DescriptorSetLayouts.emplace_back(), 0, {
				VkDescriptorSetLayoutBinding({.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT, .pImmutableSamplers = nullptr }),
			});
			VK::CreatePipelineLayout(PipelineLayouts.emplace_back(), { DescriptorSetLayouts[0] }, {});
		}
		//!<【Pass1】
		{
			const std::array ISs = { Samplers[0] };
			CreateDescriptorSetLayout(DescriptorSetLayouts.emplace_back(), 0, {
				VkDescriptorSetLayoutBinding({.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = static_cast<uint32_t>(std::size(ISs)), .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .pImmutableSamplers = std::data(ISs) }),
				VkDescriptorSetLayoutBinding({.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .pImmutableSamplers = nullptr }),
			});
			VK::CreatePipelineLayout(PipelineLayouts.emplace_back(), { DescriptorSetLayouts[1] }, {});
		}
	}
	virtual void CreateRenderPass() override { 
		//!< 【Pass0】
		CreateRenderPass_Depth();
		//!< 【Pass1】
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
		
		//!< [Pass0]
		const std::vector VIBDs = {
			VkVertexInputBindingDescription({.binding = 0, .stride = sizeof(Vertices[0]), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }),
			VkVertexInputBindingDescription({.binding = 1, .stride = sizeof(Normals[0]), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }),
		};
		const std::vector VIADs = {
			VkVertexInputAttributeDescription({.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 }),
			VkVertexInputAttributeDescription({.location = 1, .binding = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 }),
		};
		const std::array SMs_Pass0 = {
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass0VK.vert.spv"),
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass0VK.frag.spv"),
			VK::CreateShaderModule(std::filesystem::path(".") / "MeshPass0VK.geom.spv"),
		};
		const std::array PSSCIs_Pass0 = {
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = SMs_Pass0[0], .pName = "main", .pSpecializationInfo = nullptr }),
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = SMs_Pass0[1], .pName = "main", .pSpecializationInfo = nullptr }),
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_GEOMETRY_BIT, .module = SMs_Pass0[2], .pName = "main", .pSpecializationInfo = nullptr }),
		};
		CreatePipelineState_VsFsGs_Input(Pipelines[0], PipelineLayouts[0], RenderPasses[0], VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, PRSCI, VK_TRUE, VIBDs, VIADs, PSSCIs_Pass0);
		
		//!< [Pass1]
		const std::array SMs_Pass1 = {
			VK::CreateShaderModule(std::filesystem::path("..") / "Shaders" / "FinalPassVK.vert.spv"),
#ifdef DISPLAY_QUILT
			VK::CreateShaderModule(std::filesystem::path("..") / "Shaders" / "FinalPassQuiltDispVK.frag.spv"),
#else
			VK::CreateShaderModule(std::filesystem::path("..") / "Shaders" / "FinalPassVK.frag.spv"),
#endif
		};
		const std::array PSSCIs_Pass1 = {
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = SMs_Pass1[0], .pName = "main", .pSpecializationInfo = nullptr }),
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = SMs_Pass1[1], .pName = "main", .pSpecializationInfo = nullptr }),
		};
		CreatePipeline_VsFs(Pipelines[1], PipelineLayouts[1], RenderPasses[1], VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0, PRSCI, VK_FALSE, PSSCIs_Pass1);

		for (auto& i : Threads) { i.join(); }
		Threads.clear();

		for (auto i : SMs_Pass0) { vkDestroyShaderModule(Device, i, GetAllocationCallbacks()); }
		for (auto i : SMs_Pass1) { vkDestroyShaderModule(Device, i, GetAllocationCallbacks()); }
	}
	void CreateDescriptor_Pass0() {
		//!<【Pass0】ダイナミックオフセット (UNIFORM_BUFFER_DYNAMIC) を使用 [Using dynamic offset (UNIFORM_BUFFER_DYNAMIC)]
		VK::CreateDescriptorPool(DescriptorPools.emplace_back(), 0, {
			VkDescriptorPoolSize({.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, .descriptorCount = 1 }),
		});

		auto DSL = DescriptorSetLayouts[0];
		auto DP = DescriptorPools[0];
		const std::array DSLs = { DSL };
		{
			const VkDescriptorSetAllocateInfo DSAI = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.pNext = nullptr,
				.descriptorPool = DP,
				.descriptorSetCount = static_cast<uint32_t>(std::size(DSLs)), .pSetLayouts = std::data(DSLs)
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

		const auto UBIndex = 0;
		const auto DSIndex = 0;

		//!< ダイナミックオフセット (UNIFORM_BUFFER_DYNAMIC) を使用する場合、.range には VK_WHOLE_SIZE では無くオフセット毎に使用するサイズを指定する
		//!< [When using dynamic offset, .range value is data size used in each offset]
		//!<	ex) 100(VK_WHOLE_SIZE) = 25(DynamicOffset) * 4 なら 25 を指定するということ
		const auto DynamicOffset = GetViewportMax() * sizeof(ViewProjectionBuffer.ViewProjection[0]);
		{
			const DescriptorUpdateInfo DUI = {
				VkDescriptorBufferInfo({.buffer = UniformBuffers[UBIndex + 0].Buffer, .offset = 0, .range = DynamicOffset }),
			};
			vkUpdateDescriptorSetWithTemplate(Device, DescriptorSets[DSIndex + 0], DUT, &DUI);
		}
		vkDestroyDescriptorUpdateTemplate(Device, DUT, GetAllocationCallbacks());
	}
	void CreateDescriptor_Pass1() {
		const auto DescCount = 1;
		
		VK::CreateDescriptorPool(DescriptorPools.emplace_back(), 0, {
			VkDescriptorPoolSize({.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = DescCount }),
			VkDescriptorPoolSize({.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = DescCount }),
		});

		const auto UBIndex = 1;
		const auto DSIndex = 1;

		auto DSL = DescriptorSetLayouts[1];
		auto DP = DescriptorPools[1];
		const std::array DSLs = { DSL };
		{
			const VkDescriptorSetAllocateInfo DSAI = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.pNext = nullptr,
				.descriptorPool = DP,
				.descriptorSetCount = static_cast<uint32_t>(std::size(DSLs)), .pSetLayouts = std::data(DSLs)
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
				.descriptorCount = _countof(DescriptorUpdateInfo::DII), .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.offset = offsetof(DescriptorUpdateInfo, DII), .stride = sizeof(VkDescriptorImageInfo)
			}),
			VkDescriptorUpdateTemplateEntry({
				.dstBinding = 1, .dstArrayElement = 0,
				.descriptorCount = _countof(DescriptorUpdateInfo::DBI), .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.offset = offsetof(DescriptorUpdateInfo, DBI), .stride = sizeof(DescriptorUpdateInfo)
			}),
		}, DSL);
		{
			const DescriptorUpdateInfo DUI = {
				VkDescriptorImageInfo({.sampler = VK_NULL_HANDLE, .imageView = RenderTextures[0].View, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }),
				VkDescriptorBufferInfo({.buffer = UniformBuffers[UBIndex + 0].Buffer, .offset = 0, .range = VK_WHOLE_SIZE}),
			};
			vkUpdateDescriptorSetWithTemplate(Device, DescriptorSets[DSIndex + 0], DUT, &DUI);
		}
		vkDestroyDescriptorUpdateTemplate(Device, DUT, GetAllocationCallbacks());
	}
	virtual void CreateDescriptor() override {
		CreateDescriptor_Pass0();
		CreateDescriptor_Pass1();
	}
	virtual void CreateFramebuffer() override {
		//!<【Pass0】[Render target (quilt size)]
		//!< レンダーターゲット (キルトサイズ)
		VK::CreateFramebuffer(Framebuffers.emplace_back(), RenderPasses[0], QuiltX, QuiltY, 1, { RenderTextures[0].View, DepthTextures[0].View });

		//!<【Pass1】[Swapchain (screen size)]
		//!< スワップチェイン (スクリーンサイズ)
		for (const auto& i : SwapchainBackBuffers) {
			VK::CreateFramebuffer(Framebuffers.emplace_back(), RenderPasses[1], SurfaceExtent2D.width, SurfaceExtent2D.height, 1, { i.ImageView });
		}
	}
	void PopulateSecondaryCommandBuffer_Pass0() {
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

			//!< 描画毎にユニフォームバッファのアクセス先をオフセットする [Offset uniform buffer access on each draw]
			const auto DynamicOffset = GetViewportMax() * sizeof(ViewProjectionBuffer.ViewProjection[0]);
			//!< アライメント情報が必要 [Need alignment information]
			VkMemoryRequirements MR;
			vkGetBufferMemoryRequirements(Device, UniformBuffers[0].Buffer, &MR);

			//!< キルトパターン描画 (ビューポート同時描画数に制限がある為、要複数回実行) [Because viewport max is 16, need to draw few times]
			for (uint32_t j = 0; j < GetViewportDrawCount(); ++j) {
				const auto Offset = GetViewportSetOffset(j);
				const auto Count = GetViewportSetCount(j);

				vkCmdSetViewport(SCB, 0, Count, &QuiltViewports[Offset]);
				vkCmdSetScissor(SCB, 0, Count, &QuiltScissorRects[Offset]);

				//!< デスクリプタ [Descriptor]
				const std::array DSs = { DS };
				const std::array DynamicOffsets = { static_cast<uint32_t>(RoundUp(DynamicOffset * j, MR.alignment)) };
				vkCmdBindDescriptorSets(SCB, VK_PIPELINE_BIND_POINT_GRAPHICS, PLL, 0, static_cast<uint32_t>(std::size(DSs)), std::data(DSs), static_cast<uint32_t>(std::size(DynamicOffsets)), std::data(DynamicOffsets));

				const std::array VBs = { VertexBuffers[0].Buffer };
				const std::array NBs = { VertexBuffers[1].Buffer };
				const std::array Offsets = { VkDeviceSize(0) };
				vkCmdBindVertexBuffers(SCB, 0, static_cast<uint32_t>(std::size(VBs)), std::data(VBs), std::data(Offsets));
				vkCmdBindVertexBuffers(SCB, 1, static_cast<uint32_t>(std::size(NBs)), std::data(NBs), std::data(Offsets));
				vkCmdBindIndexBuffer(SCB, IndexBuffers[0].Buffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdDrawIndexedIndirect(SCB, IndirectBuffers[0].Buffer, 0, 1, 0);
			}
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(SCB));
	}
	void PopulateSecondaryCommandBuffer_Pass1() {
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

			vkCmdSetViewport(SCB, 0, static_cast<uint32_t>(std::size(Viewports)), std::data(Viewports));
			vkCmdSetScissor(SCB, 0, static_cast<uint32_t>(std::size(ScissorRects)), std::data(ScissorRects));

			const std::array DSs = { DS };
			constexpr std::array<uint32_t, 0> DynamicOffsets = {};
			vkCmdBindDescriptorSets(SCB, VK_PIPELINE_BIND_POINT_GRAPHICS, PLL, 0, static_cast<uint32_t>(std::size(DSs)), std::data(DSs), static_cast<uint32_t>(std::size(DynamicOffsets)), std::data(DynamicOffsets));

			vkCmdDrawIndirect(SCB, IndirectBuffers[1].Buffer, 0, 1, 0);
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(SCB));
	}
	virtual void PopulateSecondaryCommandBuffer(const size_t i) override {
		if (0 == i) {
			//!<【Pass0】
			PopulateSecondaryCommandBuffer_Pass0();
			//!<【Pass1】
			PopulateSecondaryCommandBuffer_Pass1();
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
			//!<【Pass0】
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
					.renderArea = VkRect2D({.offset = VkOffset2D({.x = 0, .y = 0 }), .extent = VkExtent2D({.width = static_cast<uint32_t>(QuiltX), .height = static_cast<uint32_t>(QuiltY) })}), //!< キルトサイズ
					.clearValueCount = static_cast<uint32_t>(size(CVs)), .pClearValues = std::data(CVs)
				};
				vkCmdBeginRenderPass(CB, &RPBI, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS); {
					const std::array SCBs = { SCB };
					vkCmdExecuteCommands(CB, static_cast<uint32_t>(std::size(SCBs)), std::data(SCBs));
				} vkCmdEndRenderPass(CB);
			}

			//!< バリア [Barrier]
			//!< (カラーアタッチメント から シェーダリードへ)
			ImageMemoryBarrier(CB,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				RenderTextures[0].Image);

			//!<【Pass1】
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
					.renderArea = VkRect2D({.offset = VkOffset2D({.x = 0, .y = 0 }), .extent = SurfaceExtent2D }),
					.clearValueCount = static_cast<uint32_t>(std::size(CVs)), .pClearValues = std::data(CVs)
				};
				vkCmdBeginRenderPass(CB, &RPBI, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS); {
					const std::array SCBs = { SCB };
					vkCmdExecuteCommands(CB, static_cast<uint32_t>(std::size(SCBs)), std::data(SCBs));
				} vkCmdEndRenderPass(CB);				
			}
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CB));
	}
	virtual void UpdateViewProjectionBuffer() override {
		const auto Count = (std::min)(static_cast<size_t>(LenticularBuffer.TileX * LenticularBuffer.TileY), _countof(ViewProjectionBuffer.ViewProjection));
		for (auto i = 0; i < Count; ++i) {
			ViewProjectionBuffer.ViewProjection[i] = ProjectionMatrices[i] * ViewMatrices[i];
		}
	}

protected:
	struct VIEW_PROJECTION_BUFFER {
		glm::mat4 ViewProjection[64]; //!< 64 もあれば十分 [64 will be enough]
	};
	VIEW_PROJECTION_BUFFER ViewProjectionBuffer;
};