#pragma once

#include <Windows.h>

#include <vector>
#include <array>
#include <cassert>
#include <iterator>
#include <numeric>
#include <bitset>
#include <algorithm>

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

#ifdef _DEBUG
#define LOG(x) OutputDebugStringA((x))
#else
#define LOG(x)
#endif

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
#define TIMER_ID 1000 //!< 何でも良い
		KillTimer(hWnd, TIMER_ID);
		{
			GetClientRect(hWnd, &Rect);

			//Swapchain->ResizeBuffers() でリサイズ
			//その他のリソースはほぼ作り直し
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
	virtual void CreatePipelineLayout() {}
	virtual void CreateRenderPass(VkRenderPass& RP, const std::vector<VkAttachmentDescription>& ADs, const std::vector<VkSubpassDescription>& SDs, const std::vector<VkSubpassDependency>& Deps);
	virtual void CreateRenderPass() {
		constexpr std::array<VkAttachmentReference, 0> IAs = {};
		constexpr std::array CAs = { VkAttachmentReference({.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }), };
		constexpr std::array RAs = { VkAttachmentReference({.attachment = VK_ATTACHMENT_UNUSED, .layout = VK_IMAGE_LAYOUT_UNDEFINED }), };
		constexpr std::array<uint32_t, 0> PAs = {};

		VK::CreateRenderPass(RenderPasses.emplace_back(), {
			VkAttachmentDescription({
				.flags = 0,
				.format = ColorFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE, //!< 「開始時にクリア」「終了時に保存」
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			}),
			}, {
				VkSubpassDescription({
					.flags = 0,
					.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
					.inputAttachmentCount = static_cast<uint32_t>(size(IAs)), .pInputAttachments = data(IAs),
					.colorAttachmentCount = static_cast<uint32_t>(size(CAs)), .pColorAttachments = data(CAs), .pResolveAttachments = data(RAs),
					.pDepthStencilAttachment = nullptr,
					.preserveAttachmentCount = static_cast<uint32_t>(size(PAs)), .pPreserveAttachments = data(PAs)
				}),
			}, {});
	}
	virtual void CreatePipeline() {}
	virtual void CreateFramebuffer(VkFramebuffer& FB, const VkRenderPass RP, const uint32_t Width, const uint32_t Height, const uint32_t Layers, const std::vector<VkImageView>& IVs);
	virtual void CreateFramebuffer() {
		const auto RP = RenderPasses.front();
		for (auto i : SwapchainImageViews) {
			VK::CreateFramebuffer(Framebuffers.emplace_back(), RP, SurfaceExtent2D.width, SurfaceExtent2D.height, 1, { i });
		}
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

	std::vector<VkRenderPass> RenderPasses;
	std::vector<VkPipeline> Pipelines;
	std::vector<VkFramebuffer> Framebuffers;

	std::vector<VkViewport> Viewports;
	std::vector<VkRect2D> ScissorRects;
};
