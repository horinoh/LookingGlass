#pragma once

#ifdef USE_CV
#include "CV.h"

class DisplacementCVDX : public DisplacementDX, public CV
{
public:
	virtual Texture& Create(Texture& Tex, const cv::Mat CVImage, const DXGI_FORMAT Format) {
		return Tex.Create(COM_PTR_GET(Device), static_cast<UINT64>(CVImage.cols), static_cast<UINT>(CVImage.rows), 1, Format);
	}
	virtual Texture& Update(Texture& Tex, const cv::Mat CVImage) {
		const auto CA = COM_PTR_GET(DirectCommandAllocators[0]);
		const auto CL = COM_PTR_GET(DirectCommandLists[0]);
		const auto RD = Tex.Resource->GetDesc();

		ResourceBase Upload;
		{
			Upload.Create(COM_PTR_GET(Device), CVImage.cols * CVImage.rows * CVImage.channels(), D3D12_HEAP_TYPE_UPLOAD, CVImage.ptr());
		}

		const std::vector PSFs = {
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT({
				.Offset = RoundUp(0, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT),
				.Footprint = D3D12_SUBRESOURCE_FOOTPRINT({
					.Format = RD.Format,
					.Width = static_cast<UINT>(RD.Width), .Height = RD.Height, .Depth = 1,
					.RowPitch = static_cast<UINT>(RoundUp(RD.Width * CVImage.channels(), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT))
				})
			})
		};
		VERIFY_SUCCEEDED(CL->Reset(CA, nullptr)); {
			PopulateCopyTextureRegionCommand(CL, COM_PTR_GET(Upload.Resource), COM_PTR_GET(Tex.Resource), PSFs, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		} VERIFY_SUCCEEDED(CL->Close());
		DX::ExecuteAndWait(COM_PTR_GET(GraphicsCommandQueue), CL, COM_PTR_GET(GraphicsFence));

		return Tex;
	}
};
#endif
