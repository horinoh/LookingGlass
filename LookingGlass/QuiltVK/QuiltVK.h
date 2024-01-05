#pragma once

#include "resource.h"

#include "../VKImage.h"
#include "../Holo.h"

class QuiltVK : public VKImage, public Holo
{
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) override {
		//!< Looking Glass ウインドウのサイズを取得してから
		Holo::SetHoloWindow(hWnd, hInstance);
		//!< VK の準備を行う
		VK::OnCreate(hWnd, hInstance, Title);
	}

	virtual void CreateGeometry() override {
		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;
		const auto CB = CommandBuffers[0];
		constexpr VkDrawIndirectCommand DIC = {
			.vertexCount = 4, 
			.instanceCount = 1, 
			.firstVertex = 0, 
			.firstInstance = 0
		};
		IndirectBuffers.emplace_back().Create(Device, PDMP, DIC).SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, sizeof(DIC), &DIC);
	}
	virtual void CreateUniformBuffer() override {
		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;
		//for (size_t i = 0; i < size(SwapchainImages); ++i) {
		//	UniformBuffers.emplace_back().Create(Device, PDMP, sizeof(*LenticularBuffer));
		//}
		UniformBuffers.emplace_back().Create(Device, PDMP, sizeof(*LenticularBuffer));
	}
	//!< キルト画像は dds 形式にして Asset フォルダ内へ配置しておく
	virtual void CreateTexture() override {
		const auto PDMP = CurrentPhysicalDeviceMemoryProperties;
		const auto CB = CommandBuffers[0];
		//example01.dds // 4x8
		//mar1_qs5x8.dds 
		//dedouze_qs5x9.dds
		//inventorbench_jayhowse_qs5x9.dds
		//j_smf_lightfield_qs5x9.dds
		//timestar_40.dds // 8x6
		//soccerballquilt.dds // 8x6
		//Jane_Guan_Space_Nap_qs8x6.dds //https://docs.lookingglassfactory.com/keyconcepts/quilts
		GLITextures.emplace_back().Create(Device, PDMP, std::filesystem::path("..") / "Asset" / "Jane_Guan_Space_Nap_qs8x6.dds").SubmitCopyCommand(Device, PDMP, CB, GraphicsQueue, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		if (nullptr != LenticularBuffer) {
			const auto& Extent = GLITextures.back().GetGliTexture().extent(0);
			//!< キルト画像の分割に合わせて 引数 Column, Row を指定すること
			Holo::UpdateLenticularBuffer(8, 6, Extent.x, Extent.y);

			//for (auto i : UniformBuffers) {
			//	CopyToHostVisibleDeviceMemory(Device, i.DeviceMemory, 0, sizeof(*LenticularBuffer), LenticularBuffer);
			//}
			auto& UB = UniformBuffers[0];
			CopyToHostVisibleDeviceMemory(Device, UB.DeviceMemory, 0, sizeof(*LenticularBuffer), LenticularBuffer);
		}
	}
	virtual void CreateImmutableSampler() override {
		CreateImmutableSampler_LinearRepeat();
	}
	virtual void CreatePipelineLayout() override {
		const std::array ISs = { Samplers[0] };
		CreateDescriptorSetLayout(DescriptorSetLayouts.emplace_back(), 0, {
			VkDescriptorSetLayoutBinding({.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = static_cast<uint32_t>(size(ISs)), .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .pImmutableSamplers = data(ISs) }),
			VkDescriptorSetLayoutBinding({.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .pImmutableSamplers = nullptr }),
		});
		VK::CreatePipelineLayout(PipelineLayouts.emplace_back(), DescriptorSetLayouts, {});
	}
	virtual void CreateRenderPass() override { CreateRenderPass_None(); }
	virtual void CreatePipeline() override {
		const std::array SMs = {
			VK::CreateShaderModule(std::filesystem::path(".") / "QuiltVK.vert.spv"),
			VK::CreateShaderModule(std::filesystem::path(".") / "QuiltVK.frag.spv"),
		};
		const std::array PSSCIs = {
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = SMs[0], .pName = "main", .pSpecializationInfo = nullptr }),
			VkPipelineShaderStageCreateInfo({.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr, .flags = 0, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = SMs[1], .pName = "main", .pSpecializationInfo = nullptr }),
		};
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
		CreatePipeline_VsFs(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0, PRSCI, VK_FALSE, PSSCIs);

		for (auto i : SMs) { vkDestroyShaderModule(Device, i, GetAllocationCallbacks()); }
	}
	virtual void CreateDescriptor() override {
		const auto DescCount = 1;//static_cast<uint32_t>(size(SwapchainImages));
		VK::CreateDescriptorPool(DescriptorPools.emplace_back(), 0, {
			VkDescriptorPoolSize({.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1}),
			VkDescriptorPoolSize({.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = DescCount }),
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
		for (size_t i = 0; i < DescCount; ++i) {
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
		for (size_t i = 0; i < DescCount; ++i) {
			const DescriptorUpdateInfo DUI = {
				VkDescriptorImageInfo({.sampler = VK_NULL_HANDLE, .imageView = GLITextures[0].View, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }),
				VkDescriptorBufferInfo({.buffer = UniformBuffers[i].Buffer, .offset = 0, .range = VK_WHOLE_SIZE}),
			};
			vkUpdateDescriptorSetWithTemplate(Device, DescriptorSets[i], DUT, &DUI);
		}
		vkDestroyDescriptorUpdateTemplate(Device, DUT, GetAllocationCallbacks());
	}
	virtual void PopulateCommandBuffer(const size_t i) override {
		const auto RP = RenderPasses[0];
		const auto FB = Framebuffers[i];
		const auto PLL = PipelineLayouts[0];
		const auto PL = Pipelines[0];

		const auto DescIndex = 0;//i;

		//!< セカンダリコマンドバッファ
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
		const auto SCB = SecondaryCommandBuffers[i];
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(SCB, &SCBBI)); {
			vkCmdSetViewport(SCB, 0, static_cast<uint32_t>(size(Viewports)), data(Viewports));
			vkCmdSetScissor(SCB, 0, static_cast<uint32_t>(size(ScissorRects)), data(ScissorRects));

			const std::array DSs = { DescriptorSets[DescIndex]};
			constexpr std::array<uint32_t, 0> DynamicOffsets = {};
			vkCmdBindDescriptorSets(SCB, VK_PIPELINE_BIND_POINT_GRAPHICS, PLL, 0, static_cast<uint32_t>(size(DSs)), data(DSs), static_cast<uint32_t>(size(DynamicOffsets)), data(DynamicOffsets));

			vkCmdBindPipeline(SCB, VK_PIPELINE_BIND_POINT_GRAPHICS, PL);

			vkCmdDrawIndirect(SCB, IndirectBuffers[0].Buffer, 0, 1, 0);
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(SCB));

		//!< コマンドバッファ
		constexpr VkCommandBufferBeginInfo CBBI = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};
		const auto CB = CommandBuffers[i];
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CB, &CBBI)); {
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
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CB));
	}

	virtual void Camera(const int i)
	{
	}
};
