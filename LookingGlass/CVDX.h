#pragma once

#ifdef USE_CV
#include "CV.h"

class DisplacementCVDX : public DisplacementDX, public CV
{
public:
	virtual Texture& Create(Texture& Tex, const cv::Mat CVImage, const DXGI_FORMAT Format) {
		return Tex.Create(COM_PTR_GET(Device), static_cast<UINT64>(CVImage.cols), static_cast<UINT>(CVImage.rows), 1, Format);
	}
	virtual Texture& Update(Texture& Tex, const cv::Mat CVImage, const D3D12_RESOURCE_STATES State) {
		DX::UpdateTexture(Tex, static_cast<uint32_t>(CVImage.elemSize()), CVImage.ptr(), State);
		return Tex;
	}
	virtual void Update2(Texture& Tex, const cv::Mat CVImage, const D3D12_RESOURCE_STATES State,
		Texture& Tex1, const cv::Mat CVImage1, const D3D12_RESOURCE_STATES State1) {
		DX::UpdateTexture2(Tex, static_cast<uint32_t>(CVImage.elemSize()), CVImage.ptr(), State,
			Tex1, static_cast<uint32_t>(CVImage1.elemSize()), CVImage1.ptr(), State1);
	}
};
#endif
