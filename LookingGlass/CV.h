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
		LOG(std::data(std::format("CUDA {} supported\n", HasCuda() ? "" : "not")));	
		for (auto i = 0; i < cv::cuda::getCudaEnabledDeviceCount(); ++i) {
			cv::cuda::printCudaDeviceInfo(i);
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
	static bool HasCuda() { return 0 < cv::cuda::getCudaEnabledDeviceCount(); }
};

class StereoCV : public CV
{
private:
	using Super = CV;
public:
	static void StereoMatching(const cv::Mat& Left, const cv::Mat& Right, cv::Mat& Disparity)
	{
		constexpr int MinDisp = 0;
		constexpr int NumDisp = 16; //!< 16 の倍数
		constexpr int BlockSize = 3; //!< マッチングブロックサイズ [3, 11] の奇数
		constexpr int P1 = 0;// 8 * 3 * BlockSize * BlockSize;
		constexpr int P2 = 0;// 32 * 3 * BlockSize * BlockSize;
		constexpr int Disp12MaxDiff = 0; //!< 視差チェックで許容される最大差 (0は無効)
		constexpr int PreFilterCap = 0;
		auto Stereo = cv::StereoSGBM::create(MinDisp, NumDisp, BlockSize, P1, P2, Disp12MaxDiff, PreFilterCap);
		//!< StereoSGBM の計算結果は CV_16S で格納される
		Stereo->compute(Left, Right, Disparity);

		ToGrayScale(Disparity);
	}
	static cv::Mat& ToGrayScale(cv::Mat& Disparity) {
		//!< [0, 255] にしてグレースケール表示
		double MinVal, MaxVal;
		cv::minMaxLoc(Disparity, &MinVal, &MaxVal);
		const auto Tmp = 255.0 / (MaxVal - MinVal);
		Disparity.convertTo(Disparity, CV_8UC1, Tmp, -Tmp * MinVal);
		//!< 黒が凹、白が凸になるように反転
		Disparity = ~Disparity;
		return Disparity;
	}

	StereoCV() : Super()
	{
#ifdef _DEBUG
		cv::namedWindow(WinNameUI, cv::WINDOW_AUTOSIZE);
		{
			cv::createTrackbar("NumDisparity", WinNameUI, &NumDisparity, 100, [](int Pos, void* Userdata) {});
			cv::createTrackbar("BlockSize", WinNameUI, &BlockSize, 51, [](int Pos, void* Userdata) {});
			cv::createTrackbar("PreFilter", WinNameUI, &PreFilter, static_cast<int>(cv::StereoBM::PREFILTER_XSOBEL), [](int Pos, void* Userdata) {});
			cv::createTrackbar("NumIters", WinNameUI, &NumIters, 100, [](int Pos, void* Userdata) {});
			cv::createTrackbar("NumLevels", WinNameUI, &NumLevels, 100, [](int Pos, void* Userdata) {});
		}
#endif
	}

protected:
	//auto SGBM = cv::StereoSGBM::create(0, NumDisparity, 3, 0, 0, 0, 0);
	//auto BM = cv::cuda::createStereoBM(NumDisparity);
	//auto BP = cv::cuda::createStereoBeliefPropagation(NumDisparity);
	//auto CSBP = cv::cuda::createStereoConstantSpaceBP(NumDisparity);
#ifdef _DEBUG
	const cv::String WinNameUI = "UI";

	int NumDisparity = 64; //!< [16, 64, 64, 128, ]
	int BlockSize = 19; //!< [1, 51]
	int PreFilter = static_cast<int>(cv::StereoBM::PREFILTER_NORMALIZED_RESPONSE); //!< [PREFILTER_NORMALIZED_RESPONSE, PREFILTER_XSOBEL]
	int NumIters = 5; //!< [1, ]
	int NumLevels = 5;  //!< [1, ]
#endif

	std::array<cv::Mat, 2> StereoImages;
	cv::Mat DisparityImage = cv::Mat(cv::Size(320, 240), CV_8UC1);
};