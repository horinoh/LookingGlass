#pragma once

#define USE_LEAP
#define USE_CV
#include "CV.h"

#ifdef USE_LEAP
#include <limits>
#include <array>
#include <thread>

#include<Windows.h>

#pragma warning(push)
#pragma warning(disable : 4201)
#include <LeapC.h>
#pragma warning(pop)

#include "Common.h"

class Leap
{
public:
	Leap() {
		if (eLeapRS_Success == LeapCreateConnection(nullptr, &LeapConnection)) {
			if (eLeapRS_Success == LeapOpenConnection(LeapConnection)) {
				//!< IMAGE_EVENT を発行する設定
				LeapSetPolicyFlags(LeapConnection, eLeapPolicyFlag_Images, 0);
				LeapCreateClockRebaser(&ClockRebaser);
			
				UpdateRebase();
			}
		}
	}
	virtual ~Leap() {
		if (Thread.joinable()) {
			Thread.join();
		}
		if (nullptr != ClockRebaser) {
			LeapDestroyClockRebaser(ClockRebaser);
		}
		if (nullptr != LeapConnection) {
			LeapCloseConnection(LeapConnection);
			LeapDestroyConnection(LeapConnection);
		}
	}

	virtual void UpdateAsyncStart() {
		if (!Thread.joinable()) {
			Thread = std::thread([&]() {
				//!< スレッドは外から終了させない、フラグだけ立ててスレッド自身に終了してもらう
				while (!IsExitThread) {
					LEAP_CONNECTION_MESSAGE Msg;
					if (eLeapRS_Success == LeapPollConnection(LeapConnection, 1000, &Msg)) {
						switch (Msg.type) {
						default: break;
						case eLeapEventType_Image:
							OnImageEvent(Msg.image_event);
							break;
						}
					}
					std::this_thread::sleep_for(std::chrono::microseconds(1000));
				}
			});
		}
	}
	void ExitThread() {
		IsExitThread = true;
		if (Thread.joinable()) {
			Thread.join();
		}
	}

	int64_t UpdateRebase() {
		//!< アプリタイムを取得
		LARGE_INTEGER AppTime;
		if (QueryPerformanceCounter(&AppTime)) {
			LARGE_INTEGER Freq;
			if (QueryPerformanceFrequency(&Freq)) {				
				const auto AppMicroSec = AppTime.QuadPart * 1000 * 1000 / Freq.QuadPart;
				int64_t LeapTime;
				//!< アプリタイムを Leap タイムに変換
				LeapRebaseClock(ClockRebaser, AppMicroSec, &LeapTime);
				//!< アプリタイムと Leap タイムを同期
				LeapUpdateRebase(ClockRebaser, AppMicroSec, LeapGetNow());
				
				return LeapTime;
			}
		}
		return 0;
	}

	virtual void OnImageEvent(const LEAP_IMAGE_EVENT* IE) {
		if (CurrentMatrixVersion != IE->image[0].matrix_version) {
			CurrentMatrixVersion = IE->image[0].matrix_version;
			//!< (回転等により) ディストーションマップが変化した
			LOG(std::data(std::format("Distortion map changed. MatrixVersion = {}\n", CurrentMatrixVersion)));
		}

		for (uint32_t i = 0; i < _countof(IE->image); ++i) {
			OnImage(i, IE);
			
			const auto& Image = IE->image[i];
			for (uint32_t j = 0; j < LEAP_DISTORTION_MATRIX_N; ++j) {
				for (uint32_t k = 0; k < LEAP_DISTORTION_MATRIX_N; ++k) {
					const auto& Dist = Image.distortion_matrix->matrix[j][k];
					Dist.x; Dist.y;
				}
			}
		}
	}
	virtual void OnImage(const uint32_t i, const LEAP_IMAGE_EVENT* IE) {}

protected:
	LEAP_CONNECTION LeapConnection = nullptr;
	LEAP_CLOCK_REBASER ClockRebaser = nullptr;
	uint64_t CurrentMatrixVersion = (std::numeric_limits<uint64_t>::max)();

	std::thread Thread;
	//std::mutex Mutex;
	bool IsExitThread = false;
};

class LeapCV : public Leap, public StereoCV
{
public:
	virtual void OnImage(const uint32_t i, const LEAP_IMAGE_EVENT* IE) override {
		const auto& Image = IE->image[i];
		const auto& Prop = Image.properties;
		StereoImages[i] = cv::Mat(cv::Size(Prop.width, Prop.height), CV_8UC1, reinterpret_cast<std::byte*>(Image.data) + Image.offset, cv::Mat::AUTO_STEP);

#ifdef _DEBUG
		if (1 == i) {
			cv::Mat Concat;
			cv::hconcat(StereoImages[0], StereoImages[1], Concat);
			cv::imshow(WinNameStereo, Concat);
		}
#endif
	}

#ifdef _DEBUG
	const cv::String WinNameStereo = "Stereo";
#endif
};

#endif