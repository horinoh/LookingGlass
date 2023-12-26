#pragma once

#include <Windows.h>

#include <vector>
#include <array>
#include <cassert>
#include <iterator>
#include <numeric>
#include <bitset>
#include <algorithm>
#include <format>
#include <thread>

#define VK_USE_PLATFORM_WIN32_KHR
#pragma warning(push)
#include <vulkan/vulkan.h>
#pragma warning(pop)

//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#pragma warning(push)
//#pragma warning(disable : 4201)
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/quaternion.hpp>
//#include <glm/gtc/type_ptr.hpp>
//#include <glm/gtx/quaternion.hpp>
//#pragma warning(pop)

namespace Colors
{
	constexpr VkClearColorValue Black = { 0.0f, 0.0f, 0.0f, 1.0f };
	constexpr VkClearColorValue Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	constexpr VkClearColorValue Brown = { 0.647058845f, 0.164705887f, 0.164705887f, 1.0f };
	constexpr VkClearColorValue Gray = { 0.501960814f, 0.501960814f, 0.501960814f, 1.0f };
	constexpr VkClearColorValue Green = { 0.0f, 0.501960814f, 0.0f, 1.0f };
	constexpr VkClearColorValue Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };
	constexpr VkClearColorValue Orange = { 1.0f, 0.647058845f, 0.0f, 1.0f };
	constexpr VkClearColorValue Pink = { 1.0f, 0.752941251f, 0.796078503f, 1.0f };
	constexpr VkClearColorValue Purple = { 0.501960814f, 0.0f, 0.501960814f, 1.0f };
	constexpr VkClearColorValue Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	constexpr VkClearColorValue SkyBlue = { 0.529411793f, 0.807843208f, 0.921568692f, 1.0f };
	constexpr VkClearColorValue Transparent = { 0.0f, 0.0f, 0.0f, 0.0f };
	constexpr VkClearColorValue White = { 1.0f, 1.0f, 1.0f, 1.0f };
	constexpr VkClearColorValue Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
}

#define VERIFY_SUCCEEDED(x) (x)

#include "Common.h"

class VK
{
public:
	virtual void OnCreate(HWND hWnd, HINSTANCE hInstance, LPCWSTR Title) {
		GetClientRect(hWnd, &Rect);
		const auto W = Rect.right - Rect.left, H = Rect.bottom - Rect.top;
		LOG(data(std::format("Rect = {} x {}\n", W, H)));

		LoadVulkanLibrary();

		CreateInstance();
		SelectPhysicalDevice(Instance);
		CreateDevice(hWnd, hInstance);
		CreateFence(Device);
		CreateSwapchain();
		AllocateCommandBuffer();
		CreateGeometry();
		CreateUniformBuffer();
		CreateTexture();
		CreateImmutableSampler();
		CreatePipelineLayout();
		CreateRenderPass();
		CreatePipeline();
		CreateFramebuffer();
		CreateDescriptor();
		CreateShaderBindingTable();

		OnExitSizeMove(hWnd, hInstance);
	}
	virtual void OnExitSizeMove(HWND hWnd, HINSTANCE hInstance) {
#define TIMER_ID 1000 //!< ���ł��ǂ�
		KillTimer(hWnd, TIMER_ID);
		{
			GetClientRect(hWnd, &Rect);

			//Swapchain->ResizeBuffers() �Ń��T�C�Y
			//���̑��̃��\�[�X�͂قڍ�蒼��
			LOG("OnExitSizeMove\n");

			const auto W = Rect.right - Rect.left, H = Rect.bottom - Rect.top;
			CreateViewport(static_cast<const FLOAT>(W), static_cast<const FLOAT>(H));

			for (auto i = 0; i < size(CommandBuffers); ++i) {
				PopulateCommandBuffer(i);
			}
		}
		SetTimer(hWnd, TIMER_ID, 1000 / 60, nullptr);
	}
	virtual void OnTimer(HWND hWnd, HINSTANCE hInstance) { SendMessage(hWnd, WM_PAINT, 0, 0); }
	virtual void OnPaint(HWND hWnd, HINSTANCE hInstance) { Draw(); }
	virtual void OnDestroy(HWND hWnd, HINSTANCE hInstance);

public:
	[[nodiscard]] static const VkAllocationCallbacks* GetAllocationCallbacks() { return nullptr; /*&VkAllocationCallbacks*/ }
	virtual void LoadVulkanLibrary();
	virtual void CreateInstance(const std::vector<const char*>& AdditionalLayers = {}, const std::vector<const char*>& AdditionalExtensions = {});
	virtual void SelectPhysicalDevice(VkInstance Inst);
	virtual void CreateDevice([[maybe_unused]] HWND hWnd, HINSTANCE hInstance, void* pNext = nullptr, const std::vector<const char*>& AdditionalExtensions = {});
	virtual void CreateFence(VkDevice Dev);

	[[nodiscard]] virtual VkSurfaceFormatKHR SelectSurfaceFormat(VkPhysicalDevice PD, VkSurfaceKHR Surface);
	[[nodiscard]] virtual VkPresentModeKHR SelectSurfacePresentMode(VkPhysicalDevice PD, VkSurfaceKHR Surface); 
	virtual void CreateSwapchain(VkPhysicalDevice PD, VkSurfaceKHR Sfc, const uint32_t Width, const uint32_t Height, const VkImageUsageFlags AdditionalUsage = 0);
	virtual void GetSwapchainImages();
	virtual void CreateSwapchain() {
		CreateSwapchain(CurrentPhysicalDevice, Surface, Rect.right - Rect.left, Rect.bottom - Rect.top);
		GetSwapchainImages();
	}
	//virtual void ResizeSwapchain(const uint32_t Width, const uint32_t Height);

	virtual void AllocatePrimaryCommandBuffer();
	virtual void AllocateSecondaryCommandBuffer();
	virtual void AllocateCommandBuffer() {
		AllocatePrimaryCommandBuffer();
		AllocateSecondaryCommandBuffer();
	}

	virtual void CreateGeometry() {}
	virtual void CreateUniformBuffer() {}
	virtual void CreateTexture() {}
	virtual void CreateImmutableSampler() {}

	virtual void CreateDescriptorSetLayout(VkDescriptorSetLayout& DSL, const VkDescriptorSetLayoutCreateFlags Flags, const std::vector<VkDescriptorSetLayoutBinding>& DSLBs) {
		const VkDescriptorSetLayoutCreateInfo DSLCI = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = Flags,
			.bindingCount = static_cast<uint32_t>(size(DSLBs)), .pBindings = data(DSLBs)
		};
		VERIFY_SUCCEEDED(vkCreateDescriptorSetLayout(Device, &DSLCI, GetAllocationCallbacks(), &DSL));
	}
	virtual void CreatePipelineLayout(VkPipelineLayout& PL, const std::vector<VkDescriptorSetLayout>& DSLs, const std::vector<VkPushConstantRange>& PCRs) {
		const VkPipelineLayoutCreateInfo PLCI = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = static_cast<uint32_t>(size(DSLs)), .pSetLayouts = data(DSLs),
			.pushConstantRangeCount = static_cast<uint32_t>(size(PCRs)), .pPushConstantRanges = data(PCRs)
		};
		VERIFY_SUCCEEDED(vkCreatePipelineLayout(Device, &PLCI, GetAllocationCallbacks(), &PL));
	}
	virtual void CreatePipelineLayout() {}

	virtual void CreateRenderPass(VkRenderPass& RP, const std::vector<VkAttachmentDescription>& ADs, const std::vector<VkSubpassDescription>& SDs, const std::vector<VkSubpassDependency>& Deps);
	virtual void CreateRenderPass() { CreateRenderPass_Clear(); }
	
	[[nodiscard]] virtual VkShaderModule CreateShaderModule(const std::filesystem::path& Path) const {
		VkShaderModule SM = VK_NULL_HANDLE;
		std::ifstream In(data(Path.string()), std::ios::in | std::ios::binary);
		if (!In.fail()) {
			In.seekg(0, std::ios_base::end);
			const auto Size = In.tellg();
			if (Size) {
				In.seekg(0, std::ios_base::beg);
				std::vector<std::byte> Code(Size);
				In.read(reinterpret_cast<char*>(data(Code)), size(Code));
				const VkShaderModuleCreateInfo SMCI = {
					.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.codeSize = static_cast<size_t>(size(Code)), .pCode = reinterpret_cast<uint32_t*>(data(Code))
				};
				VERIFY_SUCCEEDED(vkCreateShaderModule(Device, &SMCI, GetAllocationCallbacks(), &SM));
			}
			In.close();
		}
		return SM;
	}
	static void CreatePipelineVsFsTesTcsGs(VkPipeline& PL,
		const VkDevice Dev, const VkPipelineLayout PLL, const VkRenderPass RP,
		const VkPrimitiveTopology Topology, const uint32_t PatchControlPoints,
		const VkPipelineRasterizationStateCreateInfo& PRSCI,
		const VkPipelineDepthStencilStateCreateInfo& PDSSCI,
		const VkPipelineShaderStageCreateInfo* VS, const VkPipelineShaderStageCreateInfo* FS, const VkPipelineShaderStageCreateInfo* TES, const VkPipelineShaderStageCreateInfo* TCS, const VkPipelineShaderStageCreateInfo* GS,
		const std::vector<VkVertexInputBindingDescription>& VIBDs, const std::vector<VkVertexInputAttributeDescription>& VIADs,
		const std::vector<VkPipelineColorBlendAttachmentState>& PCBASs);
	virtual void CreatePipeline() {}

	virtual void CreateFramebuffer(VkFramebuffer& FB, const VkRenderPass RP, const uint32_t Width, const uint32_t Height, const uint32_t Layers, const std::vector<VkImageView>& IVs);
	virtual void CreateFramebuffer() {
		const auto RP = RenderPasses.front();
		for (auto i : SwapchainImageViews) {
			VK::CreateFramebuffer(Framebuffers.emplace_back(), RP, SurfaceExtent2D.width, SurfaceExtent2D.height, 1, { i });
		}
	}

	virtual void CreateDescriptorPool(VkDescriptorPool& DP, const VkDescriptorPoolCreateFlags Flags, const std::vector<VkDescriptorPoolSize>& DPSs) {
		uint32_t MaxSets = 0;
		for (const auto& i : DPSs) {
			MaxSets = (std::max)(MaxSets, i.descriptorCount);
		}
		const VkDescriptorPoolCreateInfo DPCI = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = Flags,
			.maxSets = MaxSets,
			.poolSizeCount = static_cast<uint32_t>(size(DPSs)), .pPoolSizes = data(DPSs)
		};
		VERIFY_SUCCEEDED(vkCreateDescriptorPool(Device, &DPCI, GetAllocationCallbacks(), &DP));
	}
	virtual void CreateDescriptorUpdateTemplate(VkDescriptorUpdateTemplate& DUT, const VkPipelineBindPoint PBP, const std::vector<VkDescriptorUpdateTemplateEntry>& DUTEs, const VkDescriptorSetLayout DSL) {
		const VkDescriptorUpdateTemplateCreateInfo DUTCI = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.descriptorUpdateEntryCount = static_cast<uint32_t>(size(DUTEs)), .pDescriptorUpdateEntries = data(DUTEs),
			.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET,
			.descriptorSetLayout = DSL,
			.pipelineBindPoint = PBP,
			.pipelineLayout = VK_NULL_HANDLE, .set = 0
		};
		VERIFY_SUCCEEDED(vkCreateDescriptorUpdateTemplate(Device, &DUTCI, GetAllocationCallbacks(), &DUT));
	}
	virtual void CreateDescriptor() {}

	virtual void CreateShaderBindingTable() {}

	virtual void CreateViewport(const FLOAT Width, const FLOAT Height, const FLOAT MinDepth = 0.0f, const FLOAT MaxDepth = 1.0f);
	virtual void PopulateCommandBuffer(const size_t i) {
		const auto CB = CommandBuffers[i];
		constexpr VkCommandBufferBeginInfo CBBI = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CB, &CBBI)); {
			vkCmdSetViewport(CB, 0, static_cast<uint32_t>(size(Viewports)), data(Viewports));
			vkCmdSetScissor(CB, 0, static_cast<uint32_t>(size(ScissorRects)), data(ScissorRects));

			constexpr std::array CVs = { VkClearValue({.color = Colors::SkyBlue }) };
			const VkRenderPassBeginInfo RPBI = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.pNext = nullptr,
				.renderPass = RenderPasses[0],
				.framebuffer = Framebuffers[i],
				.renderArea = VkRect2D({.offset = VkOffset2D({.x = 0, .y = 0 }), .extent = SurfaceExtent2D }),
				.clearValueCount = static_cast<uint32_t>(size(CVs)), .pClearValues = data(CVs)
			};
			vkCmdBeginRenderPass(CB, &RPBI, VK_SUBPASS_CONTENTS_INLINE); {
			} vkCmdEndRenderPass(CB);

		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CB));
	}

	static void WaitForFence(VkDevice Device, VkFence Fence);
	virtual void DrawFrame([[maybe_unused]] const UINT i) {}
	virtual void SubmitGraphics(const uint32_t i);
	virtual void Present();
	virtual void Draw();

protected:
	[[nodiscard]] static uint32_t GetMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& PDMP, const uint32_t TypeBits, const VkMemoryPropertyFlags MPF) {
		for (uint32_t i = 0; i < PDMP.memoryTypeCount; ++i) {
			if (TypeBits & (1 << i)) {
				if ((PDMP.memoryTypes[i].propertyFlags & MPF) == MPF) {
					return i;
				}
			}
		}
		return 0xffff;
	}
	static void CreateBufferMemory(VkBuffer* Buffer, VkDeviceMemory* DeviceMemory, const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const size_t Size, const VkBufferUsageFlags BUF, const VkMemoryPropertyFlags MPF, const void* Source = nullptr) {
		constexpr std::array<uint32_t, 0> QFI = {};
		const VkBufferCreateInfo BCI = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = Size,
			.usage = BUF,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = static_cast<uint32_t>(size(QFI)), .pQueueFamilyIndices = data(QFI)
		};
		VERIFY_SUCCEEDED(vkCreateBuffer(Device, &BCI, GetAllocationCallbacks(), Buffer));

		VkMemoryRequirements MR;
		vkGetBufferMemoryRequirements(Device, *Buffer, &MR);
		constexpr VkMemoryAllocateFlagsInfo MAFI = { 
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO, 
			.pNext = nullptr, 
			.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, 
			.deviceMask = 0 
		};
		const VkMemoryAllocateInfo MAI = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = (VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT & BUF) ? &MAFI : nullptr,
			.allocationSize = MR.size,
			.memoryTypeIndex = GetMemoryTypeIndex(PDMP, MR.memoryTypeBits, MPF)
		};
		VERIFY_SUCCEEDED(vkAllocateMemory(Device, &MAI, GetAllocationCallbacks(), DeviceMemory));

		VERIFY_SUCCEEDED(vkBindBufferMemory(Device, *Buffer, *DeviceMemory, 0));

		if (nullptr != Source) {
			constexpr auto MapSize = VK_WHOLE_SIZE;
			void* Data;
			VERIFY_SUCCEEDED(vkMapMemory(Device, *DeviceMemory, 0, MapSize, static_cast<VkMemoryMapFlags>(0), &Data)); {
				memcpy(Data, Source, Size);
				if (!(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & BUF)) {
					const std::array MMRs = {
						VkMappedMemoryRange({.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, .pNext = nullptr, .memory = *DeviceMemory, .offset = 0, .size = MapSize }),
					};
					VERIFY_SUCCEEDED(vkFlushMappedMemoryRanges(Device, static_cast<uint32_t>(size(MMRs)), data(MMRs)));
				}

			} vkUnmapMemory(Device, *DeviceMemory);
		}
	}
	static void CreateImageMemory(VkImage* Image, VkDeviceMemory* DM, const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const VkImageCreateFlags ICF, const VkImageType IT, const VkFormat Format, const VkExtent3D& Extent, const uint32_t Levels, const uint32_t Layers, const VkImageUsageFlags IUF) {
		constexpr std::array<uint32_t, 0> QueueFamilyIndices = {};
		const VkImageCreateInfo ICI = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = ICF,
			.imageType = IT,
			.format = Format,
			.extent = Extent,
			.mipLevels = Levels,
			.arrayLayers = Layers,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = IUF,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = static_cast<uint32_t>(size(QueueFamilyIndices)), .pQueueFamilyIndices = data(QueueFamilyIndices),
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
		VERIFY_SUCCEEDED(vkCreateImage(Device, &ICI, GetAllocationCallbacks(), Image));

		VkMemoryRequirements MR;
		vkGetImageMemoryRequirements(Device, *Image, &MR);
		const VkMemoryAllocateInfo MAI = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = MR.size,
			.memoryTypeIndex = VK::GetMemoryTypeIndex(PDMP, MR.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		};
		VERIFY_SUCCEEDED(vkAllocateMemory(Device, &MAI, GetAllocationCallbacks(), DM));
		VERIFY_SUCCEEDED(vkBindImageMemory(Device, *Image, *DM, 0));
	}
	static void SubmitAndWait(const VkQueue Queue, const VkCommandBuffer CB) {
		const std::array<VkSemaphore, 0> WaitSems = {};
		const std::array<VkPipelineStageFlags, 0> StageFlags = {};
		const std::array CBs = { CB };
		const std::array<VkSemaphore, 0> SignalSems = {};
		const std::array SIs = {
			VkSubmitInfo({
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.pNext = nullptr,
				.waitSemaphoreCount = static_cast<uint32_t>(size(WaitSems)), .pWaitSemaphores = data(WaitSems), .pWaitDstStageMask = data(StageFlags),
				.commandBufferCount = static_cast<uint32_t>(size(CBs)), .pCommandBuffers = data(CBs),
				.signalSemaphoreCount = static_cast<uint32_t>(size(SignalSems)), .pSignalSemaphores = data(SignalSems)
			})
		};
		VERIFY_SUCCEEDED(vkQueueSubmit(Queue, static_cast<uint32_t>(size(SIs)), data(SIs), VK_NULL_HANDLE));
		VERIFY_SUCCEEDED(vkQueueWaitIdle(Queue));
	}

	static void PopulateCopyBufferToImageCommand(const VkCommandBuffer CB, const VkBuffer Src, const VkImage Dst, const VkAccessFlags AF, const VkImageLayout IL, const VkPipelineStageFlags PSF, const std::vector<VkBufferImageCopy>& BICs, const uint32_t Levels, const uint32_t Layers) {
		constexpr std::array<VkMemoryBarrier, 0> MBs = {};
		constexpr std::array<VkBufferMemoryBarrier, 0> BMBs = {};
		assert(!empty(BICs) && "BufferImageCopy is empty");
		const VkImageSubresourceRange ISR = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0, .levelCount = Levels,
			.baseArrayLayer = 0, .layerCount = Layers
		};
		{
			const std::array IMBs = {
				VkImageMemoryBarrier({
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.pNext = nullptr,
					.srcAccessMask = 0, .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = Dst,
					.subresourceRange = ISR
				})
			};
			vkCmdPipelineBarrier(CB, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				static_cast<uint32_t>(size(MBs)), data(MBs),
				static_cast<uint32_t>(size(BMBs)), data(BMBs),
				static_cast<uint32_t>(size(IMBs)), data(IMBs));
		}
		vkCmdCopyBufferToImage(CB, Src, Dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(size(BICs)), data(BICs));
		{
			const std::array IMBs = {
				VkImageMemoryBarrier({
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.pNext = nullptr,
					.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, .dstAccessMask = AF,
					.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, .newLayout = IL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = Dst,
					.subresourceRange = ISR
				})
			};
			vkCmdPipelineBarrier(CB, VK_PIPELINE_STAGE_TRANSFER_BIT, PSF, 0,
				static_cast<uint32_t>(size(MBs)), data(MBs),
				static_cast<uint32_t>(size(BMBs)), data(BMBs),
				static_cast<uint32_t>(size(IMBs)), data(IMBs));
		}
	}

	//!< �����_�[�p�X�ł̉�ʃN���A���s��
	void CreateRenderPass_Clear() {
		constexpr std::array<VkAttachmentReference, 0> InpAtts = {};
		constexpr std::array ColAtts = { VkAttachmentReference({.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }), };
		constexpr std::array ResAtts = { VkAttachmentReference({.attachment = VK_ATTACHMENT_UNUSED, .layout = VK_IMAGE_LAYOUT_UNDEFINED }), };
		constexpr std::array<uint32_t, 0> PreAtts = {};
		VK::CreateRenderPass(RenderPasses.emplace_back(),
			{
				VkAttachmentDescription({
					.flags = 0,
					.format = ColorFormat,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE, //!< �u�J�n���ɃN���A�v�u�I�����ɕۑ��v
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
				}),
			},
			{
				VkSubpassDescription({
					.flags = 0,
					.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
					.inputAttachmentCount = static_cast<uint32_t>(size(InpAtts)), .pInputAttachments = data(InpAtts),
					.colorAttachmentCount = static_cast<uint32_t>(size(ColAtts)), .pColorAttachments = data(ColAtts), .pResolveAttachments = data(ResAtts),
					.pDepthStencilAttachment = nullptr,
					.preserveAttachmentCount = static_cast<uint32_t>(size(PreAtts)), .pPreserveAttachments = data(PreAtts)
				}),
			},
			{});
	}
	void CreatePipeline_VsFs_Input(const VkPrimitiveTopology PT, const uint32_t PatchControlPoints, const VkPipelineRasterizationStateCreateInfo& PRSCI, const VkBool32 DepthEnable, const std::vector<VkVertexInputBindingDescription>& VIBDs, const std::vector<VkVertexInputAttributeDescription>& VIADs, const std::array<VkPipelineShaderStageCreateInfo, 2>& PSSCIs) {
		const VkPipelineDepthStencilStateCreateInfo PDSSCI = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthTestEnable = DepthEnable, .depthWriteEnable = DepthEnable, .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = VkStencilOpState({
				.failOp = VK_STENCIL_OP_KEEP,		//!< �X�e���V���e�X�g���s��
				.passOp = VK_STENCIL_OP_KEEP,		//!< �X�e���V���e�X�g�����A�f�v�X�e�X�g���s��
				.depthFailOp = VK_STENCIL_OP_KEEP,	//!< �X�e���V���e�X�g�����A�f�v�X�e�X�g������
				.compareOp = VK_COMPARE_OP_NEVER,	//!< �����̃X�e���V���l�Ƃ̔�r���@
				.compareMask = 0, .writeMask = 0, .reference = 0
			}),
			.back = VkStencilOpState({
				.failOp = VK_STENCIL_OP_KEEP,
				.passOp = VK_STENCIL_OP_KEEP,
				.depthFailOp = VK_STENCIL_OP_KEEP,
				.compareOp = VK_COMPARE_OP_ALWAYS,
				.compareMask = 0, .writeMask = 0, .reference = 0
			}),
			.minDepthBounds = 0.0f, .maxDepthBounds = 1.0f
		};

		//!< �u�����h (Blend)
		//!< ��) 
		//!< �u�����h	: Src * A + Dst * (1 - A)	= Src:VK_BLEND_FACTOR_SRC_ALPHA, Dst:VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, Op:VK_BLEND_OP_ADD
		//!< ���Z		: Src * 1 + Dst * 1			= Src:VK_BLEND_FACTOR_ONE, Dst:VK_BLEND_FACTOR_ONE, Op:VK_BLEND_OP_ADD
		//!< ��Z		: Src * 0 + Dst * Src		= Src:VK_BLEND_FACTOR_ZERO, Dst:VK_BLEND_FACTOR_SRC_COLOR, Op:VK_BLEND_OP_ADD
		const std::vector PCBASs = {
			VkPipelineColorBlendAttachmentState({
				.blendEnable = VK_FALSE,
				.srcColorBlendFactor = VK_BLEND_FACTOR_ONE, .dstColorBlendFactor = VK_BLEND_FACTOR_ONE, .colorBlendOp = VK_BLEND_OP_ADD, //!< �u�����h 
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE, .alphaBlendOp = VK_BLEND_OP_ADD, //!< �A���t�@
				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
			}),
		};

		std::vector<std::thread> Threads;
		//!< �����o�֐����X���b�h�Ŏg�p�������ꍇ�́A�ȉ��̂悤��this�������Ɏ��`�����g�p����΂悢
		//std::thread::thread(&VKExt::Func, this, Arg0, Arg1,...);
		Threads.emplace_back(std::thread::thread(VK::CreatePipelineVsFsTesTcsGs, std::ref(Pipelines.emplace_back()), Device, PipelineLayouts[0], RenderPasses[0], PT, PatchControlPoints, PRSCI, PDSSCI, &PSSCIs[0], &PSSCIs[1], nullptr, nullptr, nullptr, VIBDs, VIADs, PCBASs));

		for (auto& i : Threads) { i.join(); }
	}
	void CreatePipeline_VsFs(const VkPrimitiveTopology PT, const uint32_t PatchControlPoints, const VkPipelineRasterizationStateCreateInfo& PRSCI, const VkBool32 DepthEnable, const std::array<VkPipelineShaderStageCreateInfo, 2>& PSSCIs) { 
		CreatePipeline_VsFs_Input(PT, PatchControlPoints, PRSCI, DepthEnable, {}, {}, PSSCIs);
	}

	template<typename T> class Scoped : public T
	{
	public:
		Scoped(VkDevice Dev) : T(), Device(Dev) {}
		virtual ~Scoped() { if (VK_NULL_HANDLE != Device) { T::Destroy(Device); } }
	private:
		VkDevice Device = VK_NULL_HANDLE;
	};
	class DeviceMemoryBase
	{
	public:
		VkDeviceMemory DeviceMemory = VK_NULL_HANDLE;
		virtual void Destroy(const VkDevice Device) {
			if (VK_NULL_HANDLE != DeviceMemory) { vkFreeMemory(Device, DeviceMemory, GetAllocationCallbacks()); }
		}
	};
	class BufferMemory : public DeviceMemoryBase
	{
	private:
		using Super = DeviceMemoryBase;
	public:
		VkBuffer Buffer = VK_NULL_HANDLE;
		BufferMemory& Create(const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const size_t Size, const VkBufferUsageFlags BUF, const VkMemoryPropertyFlags MPF, const void* Source = nullptr) {
			VK::CreateBufferMemory(&Buffer, &DeviceMemory, Device, PDMP, Size, BUF, MPF, Source);
			return *this;
		}
		virtual void Destroy(const VkDevice Device) override {
			Super::Destroy(Device);
			if (VK_NULL_HANDLE != Buffer) { vkDestroyBuffer(Device, Buffer, GetAllocationCallbacks()); }
		}
	};
	class DeviceLocalBuffer : public BufferMemory
	{
	private:
		using Super = BufferMemory;
	public:
		DeviceLocalBuffer& Create(const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const size_t Size, const VkBufferUsageFlags BUF) {
			Super::Create(Device, PDMP, Size, BUF, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			return *this;
		}
		void PopulateCopyCommand(const VkCommandBuffer CB, const size_t Size, const VkBuffer Staging, const VkAccessFlags AF, const VkPipelineStageFlagBits PSF) {
			constexpr std::array<VkMemoryBarrier, 0> MBs = {};
			constexpr std::array<VkImageMemoryBarrier, 0> IMBs = {};
			{
				const std::array BMBs = {
					VkBufferMemoryBarrier({
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.pNext = nullptr,
						.srcAccessMask = 0, .dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.buffer = Buffer, .offset = 0, .size = VK_WHOLE_SIZE
					}),
				};
				vkCmdPipelineBarrier(CB, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					static_cast<uint32_t>(size(MBs)), data(MBs),
					static_cast<uint32_t>(size(BMBs)), data(BMBs),
					static_cast<uint32_t>(size(IMBs)), data(IMBs));
			}
			const std::array BCs = { VkBufferCopy({.srcOffset = 0, .dstOffset = 0, .size = Size }), };
			vkCmdCopyBuffer(CB, Staging, Buffer, static_cast<uint32_t>(size(BCs)), data(BCs));
			{
				const std::array BMBs = {
					VkBufferMemoryBarrier({
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.pNext = nullptr,
						.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT, .dstAccessMask = AF,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.buffer = Buffer, .offset = 0, .size = VK_WHOLE_SIZE
					}),
				};
				vkCmdPipelineBarrier(CB, VK_PIPELINE_STAGE_TRANSFER_BIT, PSF, 0,
					static_cast<uint32_t>(size(MBs)), data(MBs),
					static_cast<uint32_t>(size(BMBs)), data(BMBs),
					static_cast<uint32_t>(size(IMBs)), data(IMBs));
			}
		}
		void SubmitCopyCommand(const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const VkCommandBuffer CB, const VkQueue Queue, const size_t Size, const void* Source, const VkAccessFlags AF, const VkPipelineStageFlagBits PSF) {
			VK::Scoped<StagingBuffer> Staging(Device);
			Staging.Create(Device, PDMP, Size, Source);
			constexpr VkCommandBufferBeginInfo CBBI = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .pNext = nullptr, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, .pInheritanceInfo = nullptr };
			VERIFY_SUCCEEDED(vkBeginCommandBuffer(CB, &CBBI)); {
				PopulateCopyCommand(CB, Size, Staging.Buffer, AF, PSF);
			} VERIFY_SUCCEEDED(vkEndCommandBuffer(CB));
			VK::SubmitAndWait(Queue, CB);
		}
	};
	class HostVisibleBuffer : public BufferMemory
	{
	private:
		using Super = BufferMemory;
	public:
		HostVisibleBuffer& Create(const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const size_t Size, const VkBufferUsageFlags BUF, const void* Source = nullptr) {
			Super::Create(Device, PDMP, Size, BUF, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT/*| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT*/, Source);
			return *this;
		}
	};
	class StagingBuffer : public HostVisibleBuffer
	{
	private:
		using Super = HostVisibleBuffer;
	public:
		StagingBuffer& Create(const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const size_t Size, const void* Source = nullptr) {
			Super::Create(Device, PDMP, Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, Source);
			return *this;
		}
	};
	class IndirectBuffer : public DeviceLocalBuffer
	{
	private:
		using Super = DeviceLocalBuffer;
	protected:
		IndirectBuffer& Create(const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const size_t Size) {
			Super::Create(Device, PDMP, Size, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
			return *this;
		}
	public:
		IndirectBuffer& Create(const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const VkDrawIndexedIndirectCommand& DIIC) { return Create(Device, PDMP, sizeof(DIIC)); }
		IndirectBuffer& Create(const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const VkDrawIndirectCommand& DIC) { return Create(Device, PDMP, sizeof(DIC)); }
		IndirectBuffer& Create(const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const VkDispatchIndirectCommand& DIC) { return Create(Device, PDMP, sizeof(DIC)); }
		void PopulateCopyCommand(const VkCommandBuffer CB, const size_t Size, const VkBuffer Staging) {
			Super::PopulateCopyCommand(CB, Size, Staging, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
		}
		void SubmitCopyCommand(const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const VkCommandBuffer CB, const VkQueue Queue, const size_t Size, const void* Source) {
			Super::SubmitCopyCommand(Device, PDMP, CB, Queue, Size, Source, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
		}
	};
	class UniformBuffer : public BufferMemory
	{
	private:
		using Super = BufferMemory;
	public:
		UniformBuffer& Create(const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const size_t Size, const void* Source = nullptr) {
			VK::CreateBufferMemory(&Buffer, &DeviceMemory, Device, PDMP, Size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, Source);
			return *this;
		}
	};
	class Texture : public DeviceMemoryBase
	{
	private:
		using Super = DeviceMemoryBase;
	public:
		VkImage Image = VK_NULL_HANDLE;
		VkImageView View = VK_NULL_HANDLE;
		Texture& Create(const VkDevice Device, const VkPhysicalDeviceMemoryProperties PDMP, const VkFormat Format, const VkExtent3D& Extent, const uint32_t MipLevels = 1, const uint32_t ArrayLayers = 1, const VkImageUsageFlags Usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, const VkImageAspectFlags IAF = VK_IMAGE_ASPECT_COLOR_BIT) {
			VK::CreateImageMemory(&Image, &DeviceMemory, Device, PDMP, 0, VK_IMAGE_TYPE_2D, Format, Extent, MipLevels, ArrayLayers, Usage);
			const VkImageViewCreateInfo IVCI = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = Image,
				//!< ��{ TYPE_2D, _TYPE_2D_ARRAY �Ƃ��Ĉ����A����ȊO�Ŏg�p����ꍇ�͖����I�ɏ㏑�����Ďg��
				.viewType = ArrayLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY,
				.format = Format,
				.components = VkComponentMapping({.r = VK_COMPONENT_SWIZZLE_R, .g = VK_COMPONENT_SWIZZLE_G, .b = VK_COMPONENT_SWIZZLE_B, .a = VK_COMPONENT_SWIZZLE_A }),
				.subresourceRange = VkImageSubresourceRange({.aspectMask = IAF, .baseMipLevel = 0, .levelCount = VK_REMAINING_MIP_LEVELS, .baseArrayLayer = 0, .layerCount = VK_REMAINING_ARRAY_LAYERS })
			};
			VERIFY_SUCCEEDED(vkCreateImageView(Device, &IVCI, GetAllocationCallbacks(), &View));
			return *this;
		}
		virtual void Destroy(const VkDevice Device) override {
			Super::Destroy(Device);
			if (VK_NULL_HANDLE != Image) { vkDestroyImage(Device, Image, GetAllocationCallbacks()); }
			if (VK_NULL_HANDLE != View) { vkDestroyImageView(Device, View, GetAllocationCallbacks()); }
		}

		//!< ���C�A�E�g�� GENERAL �ɂ��Ă����K�v������ꍇ�Ɏg�p
		//!< ��) �p�ɂɃ��C�A�E�g�̕ύX��߂����K�v�ɂȂ�悤�ȏꍇ UNDEFINED �ւ͖߂��Ȃ��̂ŁA�߂��郌�C�A�E�g�ł��� GENERAL ���������C�A�E�g�Ƃ��Ă���
		void PopulateSetLayoutCommand(const VkCommandBuffer CB, const VkImageLayout Layout) {
			const std::array IMBs = {
				//!< UNDEFINED -> Layout
				VkImageMemoryBarrier({
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.pNext = nullptr,
					.srcAccessMask = 0, .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = Layout,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = Image,
					.subresourceRange = VkImageSubresourceRange({.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 })
				}),
			};
			vkCmdPipelineBarrier(CB, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(size(IMBs)), data(IMBs));
		}
		void SubmitSetLayoutCommand(const VkCommandBuffer CB, const VkQueue Queue, const VkImageLayout Layout = VK_IMAGE_LAYOUT_GENERAL) {
			constexpr VkCommandBufferBeginInfo CBBI = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .pNext = nullptr, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, .pInheritanceInfo = nullptr };
			VERIFY_SUCCEEDED(vkBeginCommandBuffer(CB, &CBBI)); {
				PopulateSetLayoutCommand(CB, Layout);
			} VERIFY_SUCCEEDED(vkEndCommandBuffer(CB));
			VK::SubmitAndWait(Queue, CB);
		}
	};

protected:
	RECT Rect;

	HMODULE VulkanLibrary = nullptr;
#define VK_PROC_ADDR(proc) static PFN_vk ## proc vk ## proc;
#include "VKGlobalProcAddr.h"
#include "VKInstanceProcAddr.h"
#include "VKDeviceProcAddr.h"
#undef VK_PROC_ADDR

	VkInstance Instance = VK_NULL_HANDLE;
	VkSurfaceKHR Surface = VK_NULL_HANDLE;
	std::vector<VkPhysicalDevice> PhysicalDevices;
	VkPhysicalDevice CurrentPhysicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceMemoryProperties CurrentPhysicalDeviceMemoryProperties;
	VkDevice Device = VK_NULL_HANDLE;
	VkQueue GraphicsQueue = VK_NULL_HANDLE;
	VkQueue PresentQueue = VK_NULL_HANDLE;
	uint32_t GraphicsQueueFamilyIndex = UINT32_MAX;
	uint32_t PresentQueueFamilyIndex = UINT32_MAX;
	
	VkFence GraphicsFence = VK_NULL_HANDLE;
	VkSemaphore NextImageAcquiredSemaphore = VK_NULL_HANDLE;
	VkSemaphore RenderFinishedSemaphore = VK_NULL_HANDLE;
	VkSemaphore ComputeSemaphore = VK_NULL_HANDLE;

	VkExtent2D SurfaceExtent2D;
	VkFormat ColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> SwapchainImages;
	uint32_t SwapchainImageIndex = 0;
	std::vector<VkImageView> SwapchainImageViews;

	std::vector<VkCommandPool> CommandPools;
	std::vector<VkCommandBuffer> CommandBuffers;
	std::vector<VkCommandPool> SecondaryCommandPools;
	std::vector<VkCommandBuffer> SecondaryCommandBuffers;

	std::vector<IndirectBuffer> IndirectBuffers;
	std::vector<UniformBuffer> UniformBuffers;

	//std::vector<Texture> Textures;
	//std::vector<DepthTexture> DepthTextures;
	//std::vector<RenderTexture> RenderTextures;
	std::vector<VkSampler> Samplers;

	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
	std::vector<VkPipelineLayout> PipelineLayouts;

	std::vector<VkRenderPass> RenderPasses;
	std::vector<VkPipeline> Pipelines;
	std::vector<VkFramebuffer> Framebuffers;

	std::vector<VkDescriptorPool> DescriptorPools;
	std::vector<VkDescriptorSet> DescriptorSets;
	std::vector<VkDescriptorUpdateTemplate> DescriptorUpdateTemplates;

	std::vector<VkViewport> Viewports;
	std::vector<VkRect2D> ScissorRects;
};
