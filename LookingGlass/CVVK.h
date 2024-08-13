#pragma once

#ifdef USE_CV
#define USE_CUDA
#include "CV.h"

class DisplacementCVVK : public DisplacementVK, public CV
{
public:
	virtual Texture& Create(Texture& Tex, const cv::Mat CVImage, const VkFormat Format) {
		return Tex.Create(Device, SelectedPhysDevice.second.PDMP, Format, VkExtent3D({ .width = static_cast<uint32_t>(CVImage.cols), .height = static_cast<uint32_t>(CVImage.rows), .depth = 1 }));
	}
	virtual Texture& Update(Texture& Tex, const cv::Mat CVImage, const VkPipelineStageFlags Stage) {
		VK::UpdateTexture(Tex, CVImage.cols, CVImage.rows, static_cast<uint32_t>(CVImage.elemSize()), CVImage.ptr(), Stage);
		return Tex;
	}
	virtual void Update2(Texture& Tex, const cv::Mat CVImage, const VkPipelineStageFlags Stage,
		Texture& Tex1, const cv::Mat CVImage1, const VkPipelineStageFlags Stage1) {
		VK::UpdateTexture2(Tex, CVImage.cols, CVImage.rows, static_cast<uint32_t>(CVImage.elemSize()), CVImage.ptr(), Stage,
			Tex1, CVImage1.cols, CVImage1.rows, static_cast<uint32_t>(CVImage1.elemSize()), CVImage1.ptr(), Stage1);
	}
};
#endif