#pragma once

#include <filesystem>
#include <Windows.h>

#pragma warning(push)
#pragma warning(disable : 4819)
#include <opencv2/opencv.hpp>
#pragma warning(pop)
#ifdef USE_CUDA
#include <opencv2/cudastereo.hpp>
#endif

#include "Common.h"

class CV
{
public:
	CV() {
		const auto DeviceCount = cv::cuda::getCudaEnabledDeviceCount();
		if (0 == DeviceCount) {
			LOG(std::data(std::format("CUDA not supported\n")));
		}
		else {
			LOG(std::data(std::format("CUDA supported\n")));
			for (auto i = 0; i < DeviceCount; ++i) {
				cv::cuda::printCudaDeviceInfo(i);
			}
		}
	}

	static auto GetOpenCVPath() {
		char* CVPath;
		size_t Size = 0;
		if (!_dupenv_s(&CVPath, &Size, "OPENCV_SDK_PATH")) {
			const auto Path = std::filesystem::path(CVPath);
			free(CVPath);
			return Path;
		}
		return std::filesystem::path();
	}
};

class StereoCV : public CV
{
public:
	static void StereoMatching(const cv::Mat& Left, const cv::Mat& Right, cv::Mat& Disparity)
	{
		constexpr int MinDisp = 0;
		constexpr int NumDisp = 16; //!< 16 �̔{��
		constexpr int BlockSize = 3; //!< �}�b�`���O�u���b�N�T�C�Y [3, 11] �̊
		constexpr int P1 = 0;// 8 * 3 * BlockSize * BlockSize;
		constexpr int P2 = 0;// 32 * 3 * BlockSize * BlockSize;
		constexpr int Disp12MaxDiff = 0; //!< �����`�F�b�N�ŋ��e�����ő卷 (0�͖���)
		constexpr int PreFilterCap = 0;
		auto Stereo = cv::StereoSGBM::create(MinDisp, NumDisp, BlockSize, P1, P2, Disp12MaxDiff, PreFilterCap);
		//!< StereoSGBM �̌v�Z���ʂ� CV_16S �Ŋi�[�����
		Stereo->compute(Left, Right, Disparity);

		//!< [0, 255] �ɂ��ăO���[�X�P�[���\��
		double MinVal, MaxVal;
		cv::minMaxLoc(Disparity, &MinVal, &MaxVal);
		const auto A = 255.0 / (MaxVal - MinVal);
		Disparity.convertTo(Disparity, CV_8UC1, A, -A * MinVal);
		//!< �������A�����ʂɂȂ�悤�ɔ��]
		Disparity = ~Disparity;
	}
};