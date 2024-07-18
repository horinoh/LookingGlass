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
	StereoCV() : Super(), Matcher(cv::StereoSGBM::create()) {
#ifdef _DEBUG	
		//!< [ cv::StereoMatcher ]
		//MinMinDisparity = GetMatcher()->getMinDisparity();
		NumDisparities = GetMatcher()->getNumDisparities(); //!< 16 の倍数
		BlockSize = GetMatcher()->getBlockSize(); //!< [3, 11] の奇数
		//SpeckleWindowSize = GetMatcher()->getSpeckleWindowSize();
		//SpeckleRange = GetMatcher()->getSpeckleRange();
		//Disp12MaxDiff = GetMatcher()->getDisp12MaxDiff();
	   
		//!< [ cv::StereoSGBM ]
		//, PreFilterCap(SGBM->getPreFilterCap())
		//, UniquenessRatio(SGBM->getUniquenessRatio())
		//, P1(SGBM->getP1()) //!< 8 * 3 * BlockSize * BlockSize
		//, P2(SGBM->getP2()) //!< 32 * 3 * BlockSize * BlockSize
		//, Mode(SGBM->getMode())
#endif

		//!< とりあえずデフォルト動作としてサンプルの左右画像を読み込んでおく
		//const auto CVPath = CV::GetOpenCVPath();
		//StereoImages[0] = cv::imread((CVPath / "sources" / "samples" / "data" / "aloeL.jpg").string());
		//StereoImages[1] = cv::imread((CVPath / "sources" / "samples" / "data" / "aloeR.jpg").string());
		const auto CVSamplePath = std::filesystem::path("..") / ".." / ".." / "opencv" / "samples" / "data";
		StereoImages[0] = cv::imread((CVSamplePath / "aloeL.jpg").string());
		StereoImages[1] = cv::imread((CVSamplePath / "aloeR.jpg").string());
		cv::resize(StereoImages[0], StereoImages[0], cv::Size(640, 240));
		cv::resize(StereoImages[1], StereoImages[1], cv::Size(640, 240));
		cv::cvtColor(StereoImages[0], StereoImages[0], cv::COLOR_BGR2GRAY);
		cv::cvtColor(StereoImages[1], StereoImages[1], cv::COLOR_BGR2GRAY);
		ShowStereo();
	}

	virtual void Compute() {
		Matcher->compute(StereoImages[0], StereoImages[1], DisparityImage);
		//!< 計算結果は CV_16S で格納されるのでグレースケールへ変換
		ToGrayScale(DisparityImage);

		ShowDisparity();
	}

	virtual void CreateUI() {
#ifdef _DEBUG
		const cv::String WinUI = "UI";
		cv::namedWindow(WinUI, cv::WINDOW_AUTOSIZE);
		{
			cv::createTrackbar("NumDisparities", WinUI, &NumDisparities, 10, [](int Pos, void* Userdata) {
				auto This = reinterpret_cast<StereoCV*>(Userdata);
				//!< 16 の倍数
				This->GetMatcher()->setNumDisparities((std::max)(Pos, 1) << 4);
				This->Compute();
				}, this);
			cv::createTrackbar("BlockSize", WinUI, &BlockSize, 11, [](int Pos, void* Userdata) {
				auto This = reinterpret_cast<StereoCV*>(Userdata);
				//!< 奇数
				This->GetMatcher()->setBlockSize((Pos & 1) ? Pos : (Pos + 1));
				This->Compute();
				}, this);
		}
		Compute();
#endif
	}

	void ShowStereo() {
#ifdef _DEBUG
		cv::Mat Concat;
		cv::hconcat(StereoImages[0], StereoImages[1], Concat);
		cv::imshow("Stereo", Concat);
		cv::pollKey();
#endif
	}
	void ShowDisparity() {
#ifdef _DEBUG
		cv::imshow("Disparity", DisparityImage);
#endif
	}

	static cv::Mat& ToGrayScale(cv::Mat& Disparity) {
		//!< [0, 255] にしてグレースケール表示
		double MinVal, MaxVal;
		cv::minMaxLoc(Disparity, &MinVal, &MaxVal);
		const auto Tmp = 255.0 / (MaxVal - MinVal);
		Disparity.convertTo(Disparity, CV_8UC1, Tmp, -Tmp * MinVal);
		
		//!< 黒が凹、白が凸になるように反転
		//Disparity = ~Disparity;
		
		return Disparity;
	}

	cv::Ptr <cv::StereoMatcher> GetMatcher() { return Matcher; }

protected:
#ifdef _DEBUG
	int NumDisparities, BlockSize;
#endif

	cv::Ptr<cv::StereoMatcher> Matcher;

	std::array<cv::Mat, 2> StereoImages;
	cv::Mat DisparityImage = cv::Mat(cv::Size(640, 240), CV_8UC1);
};

#ifdef USE_CUDA
class StereoGPUCV : public StereoCV
{
private:
	using Super = StereoCV;
public:
	StereoGPUCV() : Super() {
		//!< [ StereoBM ]
		Matcher = cv::cuda::createStereoBM();

		//!< [ StereoBeliefPropagation ]
#ifndef _DEBUG
		int NumDisparities, NumIters, NumLevels;
#endif
		cv::cuda::StereoBeliefPropagation::estimateRecommendedParams(640, 240, NumDisparities, NumIters, NumLevels);
		Matcher = cv::cuda::createStereoBeliefPropagation(NumDisparities, NumIters, NumLevels);

		//!< [ StereoConstantSpaceBP ]
#ifndef _DEBUG
		int NrPlane;
#endif
		cv::cuda::StereoConstantSpaceBP::estimateRecommendedParams(640, 240, NumDisparities, NumIters, NumLevels, NrPlane);
		Matcher = cv::cuda::createStereoConstantSpaceBP(NumDisparities, NumIters, NumLevels, NrPlane);
	}

	virtual void Compute() {
		if (HasCuda()) {
			cv::cuda::GpuMat LG, RG;
			cv::cuda::GpuMat DG(StereoImages[0].size(), CV_8U);

			LG.upload(StereoImages[0]);
			RG.upload(StereoImages[1]);
			{
				Matcher->compute(LG, RG, DG);
			}
			DG.download(DisparityImage);

			ToGrayScale(DisparityImage);

			ShowDisparity();
		}
		else {
			Super::Compute();
		}
	}

	virtual void CreateUI() {
#ifdef _DEBUG
		if (HasCuda()) {
			const cv::String WinUI = "UI";
			cv::namedWindow(WinUI, cv::WINDOW_AUTOSIZE);
			{
				cv::createTrackbar("NumDisparities", WinUI, &NumDisparities, 300, [](int Pos, void* Userdata) {
					auto This = reinterpret_cast<StereoGPUCV*>(Userdata);
					This->GetMatcher()->setNumDisparities((std::max)(Pos, 5));
					This->Compute();
					}, this);
				cv::createTrackbar("BlockSize", WinUI, &BlockSize, 20, [](int Pos, void* Userdata) {
					auto This = reinterpret_cast<StereoGPUCV*>(Userdata);
					This->GetMatcher()->setBlockSize(Pos);
					This->Compute();
					}, this);

				if (nullptr != static_cast<cv::cuda::StereoBeliefPropagation*>(GetMatcher().get())) {
					cv::createTrackbar("NumIters", WinUI, &NumIters, 20, [](int Pos, void* Userdata) {
						auto This = reinterpret_cast<StereoGPUCV*>(Userdata);
						static_cast<cv::cuda::StereoBeliefPropagation*>(This->GetMatcher().get())->setNumIters(Pos);
						This->Compute();
						}, this);
					cv::createTrackbar("NumLevels", WinUI, &NumLevels, 8, [](int Pos, void* Userdata) {
						auto This = reinterpret_cast<StereoGPUCV*>(Userdata);
						static_cast<cv::cuda::StereoBeliefPropagation*>(This->GetMatcher().get())->setNumLevels(Pos);
						This->Compute();
						}, this);
				}
				if (nullptr != static_cast<cv::cuda::StereoConstantSpaceBP*>(GetMatcher().get())) {
					cv::createTrackbar("NrPlane", WinUI, &NrPlane, 10, [](int Pos, void* Userdata) {
						auto This = reinterpret_cast<StereoGPUCV*>(Userdata);
						static_cast<cv::cuda::StereoConstantSpaceBP*>(This->GetMatcher().get())->setNrPlane(Pos);
						This->Compute();
						}, this);
				}
			}
			Compute();
		}
		else {
			Super::CreateUI();
		}
#endif
	}

protected:
#ifdef _DEBUG
	int NumIters, NumLevels, NrPlane;
#endif
};
#endif
