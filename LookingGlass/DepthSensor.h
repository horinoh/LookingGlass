#pragma once

#pragma once

#include <format>
#include <algorithm>
#include <thread>
#include <random>

#include <asio.hpp>
#include <asio/use_awaitable.hpp>

#include "Common.h"

//!< (DEBUG ビルド時) 冗長にログを出力する、ログ出力の影響で画面がちらつく事がある
//#define LOG_VERBOSE

class Http 
{
public:
	Http(const std::string& Server, const std::string& Service) : Context(), Resolver(Context), Socket(Context) {
#if 1
		//!< 名前解決、ホスト情報
		LOG(std::data(std::format("Server={}, Service={}\n", Server, Service)));
		asio::ip::tcp::resolver::query Query(Server, Service);
		asio::ip::tcp::endpoint EndPoint(*Resolver.resolve(Query));
		//!< 接続
		Socket.connect(EndPoint);
#else
		//!< 名前解決、ホスト情報
		asio::ip::tcp::resolver::query Query(std::data(std::string(Server)), std::data(std::string(Service)));
		Resolver.async_resolve(Query, [&](const asio::error_code EC, const asio::ip::tcp::resolver::iterator& EndPoint) {
			if (!EC) {
				LOG(std::data(std::format("Resolve [OK]\n")));
				//!< 接続
				asio::async_connect(Socket, EndPoint, [&](const asio::error_code EC_Cnt, const asio::ip::tcp::resolver::iterator&) {
					if (!EC_Cnt) {
						LOG(std::data(std::format("Connect [OK]\n")));
					}
				});
			}
		});
#endif
	}

	void GetRequest(std::string_view Server, std::string_view Path) {
		if (Socket.is_open()) {
			//!< リクエスト書込
			std::ostream RequestStream(&Request);
			RequestStream << "GET " << Path << " HTTP/1.0\r\n";
			RequestStream << "Host: " << Server << "\r\n";
			//RequestStream << "Accept: */*\r\n";
			//RequestStream << "Connection: close";
			RequestStream << "\r\n\r\n";
			asio::write(Socket, Request);

			asio::read(Socket, Response, asio::transfer_all(), ErrorCode);
			if (asio::error::eof == ErrorCode) {
				LOG(std::data(std::format("\t{}\n", asio::buffer_cast<const char*>(Response.data()))));
			}
		}
	}
	void GetRequestAsync(std::string_view Server, std::string_view Path)
	{
		asio::ip::tcp::resolver::query Query(std::string(Server), "http");
		Resolver.async_resolve(Query, [&](const asio::error_code EC, const asio::ip::tcp::resolver::iterator& EndPoint) {
			if (!EC) {
				LOG(std::data(std::format("Resolve [OK]\n")));

				//!< 接続
				asio::async_connect(Socket, EndPoint, [&](const asio::error_code EC_Cnt, const asio::ip::tcp::resolver::iterator&) {
					if (!EC_Cnt) {
						LOG(std::data(std::format("Connect [OK]\n")));

						//!< リクエスト書込
						std::ostream RequestStream(&Request);
						RequestStream << "GET " << Path << " HTTP/1.0\r\n";
						RequestStream << "Host: " << Server << "\r\n";
						RequestStream << "Accept: */*\r\n";
						RequestStream << "Connection: close\r\n\r\n";
						asio::async_write(Socket, Request, [&](const asio::error_code EC_Req, const size_t ReqSize) {
							if (!EC_Req) {
								LOG(std::data(std::format("Write Request ({}Bytes) [OK]\n", ReqSize)));

								//!< レスポンス読込
								asio::async_read_until(Socket, Response, "\r\n", [&](const asio::error_code EC_Res, const size_t ResSize) {
									if (!EC_Res) {
										LOG(std::data(std::format("Read Response ({}Bytes) [OK]\n", ResSize)));

										//!< レスポンスが OK かどうか
										std::istream ResponseStream(&Response);
										std::string HttpVersion;
										ResponseStream >> HttpVersion;
										unsigned int StatusCode;
										ResponseStream >> StatusCode;
										std::string StatusMessage;
										std::getline(ResponseStream, StatusMessage);
										LOG(std::data(std::format("Response (StatusCode = {}, StatusMessage = {}) [OK]\n", StatusCode, StatusMessage)));
										if (ResponseStream && HttpVersion.substr(0, 5) == "HTTP/" && 200 == StatusCode) {
											//!< ヘッダ読込 (空行まで読み込む)
											asio::async_read_until(Socket, Response, "\r\n\r\n", [&](const asio::error_code EC_Hd, const size_t HdSize) {
												if (!EC_Hd) {
													LOG(std::data(std::format("Read Header ({}Bytes) [OK]\n", HdSize)));

													//!< レスポンスヘッダを処理
													LOG("Header\n");
													std::istream ResponseHeaderStream(&Response);
													std::string Header;
													while (std::getline(ResponseHeaderStream, Header) && Header != "\r") {
														LOG(std::data(std::format("\t{}\n", Header)));
													}
													LOG("\n");

													asio::error_code ErrorCode;
#if true
													//!< 残りのデータを読み込む
													if (asio::read(Socket, Response, asio::transfer_all(), ErrorCode)) {
														LOG(std::data(std::format("\t{}\n", asio::buffer_cast<const char*>(Response.data()))));
													}
#else
													//!< コンテンツ出力
													LOG("Contents\n");
													if (Response.size() > 0) {
														LOG(std::data(std::format("\t{}\n", asio::buffer_cast<const char*>(Response.data()))));
													}
													//!< 残りのデータを読み込む
													LOG("Remains\n");
													int count = 0;
													while (asio::read(Socket, Response, asio::transfer_at_least(1), ErrorCode)) {
														LOG(std::data(std::format("\t{}\n", asio::buffer_cast<const char*>(Response.data()))));
														++count;
													}
#endif
													if (asio::error::eof == ErrorCode) {
														LOG("[EOF]\n");
													}
												}
											});
										}
									}
								});
							}
						});
					}
				});
			}
		});
	}

	virtual void Update() {
		GetRequest("www.google.com", "/");
	}
	void ExitSocket() {
		if (Socket.is_open()) {
			Socket.close();
		}
	}
	virtual void Exit() {
		ExitSocket();
	}

protected:
	asio::io_context Context;
	asio::ip::tcp::resolver Resolver;
	asio::ip::tcp::socket Socket;
	asio::streambuf Request;
	asio::streambuf Response;
	asio::error_code ErrorCode;
};

class DepthSensorA075 : public Http
{
private:
	using Super = Http;

public:
	DepthSensorA075() : Super("192.168.233.1", "80") {}

	virtual void Update() override {
		GetRequest("192.168.233.1", "/getdeep");
	}
	virtual void UpdateAsyncStart() {
		if (!Thread.joinable()) {
			Thread = std::thread([&]() {
				//!< スレッドは外から終了させない、フラグだけ立ててスレッド自身に終了してもらう
				while (!IsExitThread) {
					Update();
					std::this_thread::sleep_for(std::chrono::microseconds(1000 / 20));
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
	virtual void Exit() override {		
		ExitThread();
		Super::Exit();
	}

	typedef struct {
		uint8_t TriggerMode;//(1)
		uint8_t	DeepMode; //!< (1) 0:16Bit, 1:8Bit
		uint8_t DeepShift; //(255)
		uint8_t IRMode; //!< (1) 0:16Bit, 1:8Bit
		uint8_t StatusMode; //!< (2) 0:16Bit, 0:2, 1:1/4, 2:1, 3:1/8
		uint8_t StatusMask; // (7)
		uint8_t RGBMode; // (1) 0:Raw, 1:Jpg
		uint8_t RGBRes;// (0)
		uint8_t ExposureTime[4]; //(0)
	} CONFIG;

	typedef struct {
		uint64_t FrameId;
		uint64_t StampMsec;
		CONFIG Config;
		uint32_t DeepDataSize;
		uint32_t RGBDataSize;
	} FRAME_HEADER;

	typedef struct {
		FRAME_HEADER Header;
	} FRAME;

	static const uint32_t ResX = 320, ResY = 240;
	static const uint32_t ResTotal = ResX * ResY;

	static const uint32_t ResRGBX = 640, ResRGBY = 480;
	static const uint32_t ResRGBTotal = ResRGBX * ResRGBY;

	static bool IsU16Deep(const CONFIG& Conf) { return 0 == Conf.DeepMode; }
	static bool IsU16IR(const CONFIG& Conf) { return 0 == Conf.IRMode; }
	static bool IsU16Status(const CONFIG& Conf) { return 0 == Conf.StatusMode; }

	static bool IsJpgRGB(const CONFIG& Conf) { return 1 == Conf.RGBMode; }

	static uint32_t GetSizeDeep(const CONFIG& Conf) { return IsU16Deep(Conf) ? (ResTotal << 1) : ResTotal; }
	static uint32_t GetSizeIR(const CONFIG& Conf) { return IsU16IR(Conf) ? (ResTotal << 1) : ResTotal; }
	static uint32_t GetSizeStatus(const CONFIG& Conf) {
		switch (Conf.StatusMode) {
		case 0: return  (ResTotal << 1);
		case 1: return (ResTotal >> 2);
		case 2: return ResTotal;
		default: return (ResTotal >> 3);
		}
	}
	static uint32_t GetSizeRGB(const FRAME_HEADER& FH) { return FH.RGBDataSize; }

	static uint32_t GetOffsetDeep() { return sizeof(FRAME_HEADER); }
	static uint32_t GetOffsetIR(const CONFIG& Conf) { return GetOffsetDeep() + GetSizeDeep(Conf); }
	static uint32_t GetOffsetStatus(const CONFIG& Conf) { return GetOffsetIR(Conf) + GetSizeIR(Conf); }
	static uint32_t GetOffsetRGB(const CONFIG& Conf) { return GetOffsetStatus(Conf) + GetSizeStatus(Conf); }

	static const std::byte* GetPtrDeep(const FRAME& Frame) { return reinterpret_cast<const std::byte*>(&Frame) + GetOffsetDeep(); }
	static const std::byte* GetPtrIR(const FRAME& Frame) { return reinterpret_cast<const std::byte*>(&Frame) + GetOffsetIR(Frame.Header.Config); }
	static const std::byte* GetPtrStatus(const FRAME& Frame) { return reinterpret_cast<const std::byte*>(&Frame) + GetOffsetStatus(Frame.Header.Config); }
	static std::byte* GetPtrRGB(FRAME& Frame) { return reinterpret_cast<std::byte*>(&Frame) + GetOffsetRGB(Frame.Header.Config); }

	static std::byte* Decode(FRAME& Frame) {
		auto Ptr = GetPtrRGB(Frame);
		if (IsJpgRGB(Frame.Header.Config)) {
#ifdef USE_CV
			const auto Decoded = cv::imdecode(cv::Mat(ResRGBX, ResRGBY, CV_8UC1, Ptr), cv::IMREAD_COLOR);
			std::memcpy(Ptr, Decoded.ptr(), Decoded.total() * Decoded.elemSize());
#else
			LOG("Need CV\n");
			__debugbreak();
#endif
		}
		return Ptr;
	}

protected:
	std::thread Thread;
	std::mutex Mutex;
	bool IsExitThread = false;
};

class SerialPort
{
public:
	enum class COM : uint8_t {
		COM1 = 1,
		COM2 = 2,
		COM3 = 3,
		COM4 = 4,
		COM5 = 5,
		//....
	};
	
	SerialPort() : Context(), Port(Context) {}

	virtual bool Open(const COM Com) {
		Port.open(std::format("COM{}", static_cast<uint8_t>(Com)), ErrorCode);
		if (!VerifyError()) {
			Port.set_option(asio::serial_port::baud_rate(115200));
			Port.set_option(asio::serial_port::character_size(8));
			Port.set_option(asio::serial_port::parity(asio::serial_port::parity::none));
			Port.set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one));
			Port.set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none));
			return true;
		}
		return false;
	}

	virtual void Update() {}
	
	void ExitSerialPort() {
		if (Port.is_open()) {
			Port.close();
		}
	}
	virtual void Exit() {
		ExitSerialPort();
	}

	virtual bool VerifyError() {
		if (ErrorCode) {
			LOG(std::data(std::format("{}\n", ErrorCode.message())));
			return true;
		}
		return false;
	}

protected:
	asio::io_context Context;
	asio::serial_port Port;
	asio::error_code ErrorCode;
};

//!< 要深度センサ MaixSense A010 (Need depth sensor MaixSense A010)
class DepthSensorA010 : public SerialPort
{
private:
	using Super = SerialPort;

public:
	DepthSensorA010() {
#if defined(USE_CV) && defined(_DEBUG)
		cv::imshow(WinNameCV, PreviewCV);
		//cv::createTrackbar("X", WinNameCV, &X, XMax, [](int Value, void* Userdata) { static_cast<XXX*>(Userdata)->Xxx(Value); }, this);
		//cv::createTrackbar("Y", WinNameCV, &Y, YMax, [](int Value, void* Userdata) { static_cast<YYY*>(Userdata)->Yyy(Value); }, this);
#endif
	}	

	virtual void UpdateAsyncStart() {
		if (!Thread.joinable()) {
			Thread = std::thread([&]() {
				//!< スレッドは外から終了させない、フラグだけ立ててスレッド自身に終了してもらう
				while (!IsExitThread) {
					Update();
					UpdateCV();
					std::this_thread::sleep_for(std::chrono::microseconds(1000 / 20));
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

	virtual bool Open(const SerialPort::COM Com) override {
		if (Super::Open(Com)) {
			SetCmdISP(ISP::Off);
			SetCmdDISP(DISP::LCD_USB);
			SetCmdBAUD(BAUD::Rate0115200);
			SetCmdUNIT(UNIT::Auto);
			SetCmdFPS(FPS::Max);
			SetCmdISP(ISP::On);
			return true;
		}
		return false;
	}

	virtual void Update() override {
		if (!Port.is_open()) {
			std::random_device RndDev;
			Mutex.lock(); {
				std::ranges::generate(Frame.Payload, [&]() { return RndDev(); });
#if defined(USE_CV) && defined(_DEBUG)
				DepthCV = cv::Mat(cv::Size(Frame.Header.Cols, Frame.Header.Rows), CV_8UC1, std::data(Frame.Payload), cv::Mat::AUTO_STEP);
#endif
			} Mutex.unlock();
		}
		else {
			while (!IsExitThread) {
				if (!Port.is_open()) { break; }

				uint16_t FrameBegin;
				Port.read_some(asio::buffer(&FrameBegin, sizeof(FrameBegin)), ErrorCode); VerifyError();
				if (static_cast<uint16_t>(FRAME_FLAG::Begin) == FrameBegin) {
					//!< フレーム開始のヘッダ発見
#ifdef LOG_VERBOSE
					LOG(std::data(std::format("[A010] Frame hader = {:#x}\n", FrameBegin)));
#endif
					//!< データサイズ取得
					uint16_t FrameDataLen;
					Port.read_some(asio::buffer(&FrameDataLen, sizeof(FrameDataLen)), ErrorCode); VerifyError();
#ifdef LOG_VERBOSE
					LOG(std::data(std::format("[A010] Fame data length = {}\n", FrameDataLen)));
#endif
					//!< フレームデータ取得
					Mutex.lock(); {
						Port.read_some(asio::buffer(&Frame, FrameDataLen), ErrorCode); VerifyError();
						//!< 黒白が凹凸となるように反転
						std::ranges::transform(Frame.Payload, std::begin(Frame.Payload), [](const uint8_t i) { return 0xff - i; });
#if defined(USE_CV) && defined(_DEBUG)
						DepthCV = cv::Mat(cv::Size(Frame.Header.Cols, Frame.Header.Rows), CV_8UC1, std::data(Frame.Payload), cv::Mat::AUTO_STEP);
#endif
					} Mutex.unlock();
					OnFrame();

					//!< チェックサム
#ifdef _DEBUG
					{
						//!< フレーム開始ヘッダ以降ここまでの「(バイト数ではなく)値」を加算したものの下位 8 ビットを求める
						const auto SumInit = (static_cast<uint16_t>(FRAME_FLAG::Begin) & 0xff) + (static_cast<uint16_t>(FRAME_FLAG::Begin) >> 8) + (FrameDataLen & 0xff) + (FrameDataLen >> 8);
						auto P = reinterpret_cast<const uint8_t*>(&Frame);
						const auto Sum = std::accumulate(P, P + FrameDataLen, SumInit) & 0xff;

						uint8_t CheckSum;
						Port.read_some(asio::buffer(&CheckSum, sizeof(CheckSum)), ErrorCode); VerifyError();
#ifdef LOG_VERBOSE
						LOG(std::data(std::format("[A010] CheckSum = {} = {}\n", CheckSum, Sum)));
#endif
					}
#else
					uint8_t CheckSum;
					Port.read_some(asio::buffer(&CheckSum, sizeof(CheckSum)), ErrorCode); VerifyError();
#endif

					//!< エンドオブパケット
					uint8_t EOP;
					Port.read_some(asio::buffer(&EOP, sizeof(EOP)), ErrorCode); VerifyError();
#ifdef LOG_VERBOSE
					LOG(std::data(std::format("[A010] EndOfPacket = {:#x} = {:#x}\n\n", static_cast<uint8_t>(FRAME_FLAG::End), EOP)));
#endif
					break;
				}
			}
		}
	}
	virtual void UpdateCV() {
#if defined(USE_CV) && defined(_DEBUG)
		//!< 複数画像をまとめて表示したい場合は cv::hconcat() や cv::vconcat() を使う
		cv::resize(DepthCV, DepthCV, PreviewCV.size());
		DepthCV.copyTo(PreviewCV);

		cv::imshow(WinNameCV, PreviewCV);
#endif
	}

	virtual void OnFrame() {
#ifdef LOG_VERBOSE
		LOG(std::data(std::format("[A010]\tReserved1 = {:#x}\n", Frame.Header.Reserved1)));
		LOG(std::data(std::format("[A010]\tOutputMode = {}\n", 0 == Frame.Header.OutputMode ? "Depth" :"Depth + IR")));
		LOG(std::data(std::format("[A010]\tSensorTemp = {}, DriverTemp = {}\n", Frame.Header.SensorTemp, Frame.Header.DriverTemp)));
		LOG(std::data(std::format("[A010]\tExposureTime = {}, {}, {}, {}\n", Frame.Header.ExposureTime[0], Frame.Header.ExposureTime[1], Frame.Header.ExposureTime[2], Frame.Header.ExposureTime[3])));
		LOG(std::data(std::format("[A010]\tErrorCode = {}\n", Frame.Header.ErrorCode)));
		LOG(std::data(std::format("[A010]\tReserved2 = {:#x}\n", Frame.Header.Reserved2)));
		LOG(std::data(std::format("[A010]\tCols x Rows = {} x {}\n", Frame.Header.Cols, Frame.Header.Rows)));
		LOG(std::data(std::format("[A010]\tFrameId = {}\n", Frame.Header.FrameId)));
		LOG(std::data(std::format("[A010]\tISPVersion = {}\n", Frame.Header.ISPVersion)));
		LOG(std::data(std::format("[A010]\tReserved3 = {:#x}\n", Frame.Header.Reserved3)));

		LOG(std::data(std::format("[A010]\t\tPayload = {}, {}, {}, {},...\n", Frame.Payload[0], Frame.Payload[1], Frame.Payload[2], Frame.Payload[3])));
#endif
	}

	virtual void Exit() override {
		ExitThread();

		SetCmdISP(ISP::Off);
		
		ExitSerialPort();
	}

	enum class FRAME_FLAG : uint16_t {
		Begin = 0xff00,
		End = 0xdd,
	};
	enum class ISP : uint8_t {
		Off = 0,
		On = 1,
	};
	enum class BINN : uint8_t {
		RESOLUTION_100x100 = 1,
		RESOLUTION_050x050 = 2,
		RESOLUTION_025x025 = 4,
	};
	enum class DISP : uint8_t {
		Off = 0,
		LCD = 1 << 0,
		USB = 1 << 1,
		UART = 1 << 2,
		LCD_USB = LCD | USB,
		LCD_UART = LCD | UART,
		USB_UART = USB | UART,
		LCD_USB_UART = LCD | USB | UART,
	};
	enum class BAUD : uint8_t {
		Rate0009600 = 0,
		Rate0057600 = 1,
		Rate0115200 = 2,
		Rate0230400 = 3,
		Rate0460800 = 4,
		Rate0921600 = 5,
		Rate1000000 = 6,
		Rate2000000 = 7,
		Rate3000000 = 8,
	};
	enum class UNIT : uint8_t {
		Min = 0,
		Auto = Min,
		Max = 10,
	};
	enum class FPS : uint8_t {
		Min = 1,
		Max = 19,
	};
	bool SetCmd(std::string_view Cmd, const uint8_t Arg) {
		if (Port.is_open()) {
			const auto CmdStr = std::format("AT+{}={}\r", std::data(Cmd), Arg);
			LOG(std::data(std::format("[A010] {}\n", CmdStr)));
			Port.write_some(asio::buffer(CmdStr)); VerifyError();

			WaitOK();
		}
		return false;
	}
	//!< オンオフ (Image Signal Processor)
	bool SetCmdISP(const ISP Arg) { return SetCmd("ISP", static_cast<const uint8_t>(Arg)); }
	//!< ビニング (解像度) (Binning)
	bool SetCmdBINN(const BINN Arg) { return SetCmd("BINN", static_cast<const uint8_t>(Arg)); }
	//!< 表示設定 (Display)
	bool SetCmdDISP(const DISP Arg) { return SetCmd("DISP", static_cast<const uint8_t>(Arg)); }
	//!< ボーレート (UART時) (Baudrate)
	bool SetCmdBAUD(const BAUD Arg) { return SetCmd("BAUD", static_cast<const uint8_t>(Arg)); }
	//!< 量子化ユニット (Quantization unit) 小さな値程、近距離が詳細になり、遠距離範囲が狭くなる
	bool SetCmdUNIT(const uint8_t Arg) { return SetCmd("UNIT", (std::clamp)(Arg, static_cast<uint8_t>(UNIT::Min), static_cast<uint8_t>(UNIT::Max))); }
	bool SetCmdUNIT(const UNIT Arg) { return SetCmdUNIT(static_cast<const uint8_t>(Arg)); }
	//!< フレームレート (Frame per second)
	bool SetCmdFPS(const uint8_t Arg) { return SetCmd("FPS", (std::clamp)(Arg, static_cast<uint8_t>(FPS::Min), static_cast<uint8_t>(FPS::Max))); }
	bool SetCmdFPS(const FPS Arg) { return SetCmdFPS(static_cast<const uint8_t>(Arg)); }

	bool WaitOK() {
		if (Port.is_open()) {
			while (true) {
				char Res;
				Port.read_some(asio::buffer(&Res, sizeof(Res)), ErrorCode); VerifyError();
				if ('O' == Res) {
					Port.read_some(asio::buffer(&Res, sizeof(Res)), ErrorCode); VerifyError();
					if ('K' == Res) {
						Port.read_some(asio::buffer(&Res, sizeof(Res)), ErrorCode); VerifyError();
						if ('\r' == Res) {
							LOG(std::data(std::format("[A010]\tOK\n")));
							return true;
						}
					}
				}
			}
		}
		return false;
	}

	typedef struct {
		//!< 16 バイト
		uint8_t Reserved1;		//!< 0xff
		uint8_t OutputMode;		//!< 0:depth only, 1:depth+ir
		uint8_t SensorTemp;
		uint8_t DriverTemp;
		uint8_t ExposureTime[4];
		uint8_t ErrorCode;
		uint8_t Reserved2;		//!< 0x00
		uint8_t Rows;
		uint8_t Cols;
		uint16_t FrameId;		//!< [0, 4095]
		uint8_t ISPVersion;
		uint8_t Reserved3;		//!< 0xff
	} FRAME_HEADER;

	typedef struct {
		FRAME_HEADER Header;
		std::array<uint8_t, 100 * 100> Payload;
	} FRAME;

protected:
	FRAME Frame = { .Header = { .Rows = 100, .Cols = 100 } };

	std::thread Thread;
	std::mutex Mutex;
	bool IsExitThread = false;

#if defined(USE_CV) && defined(_DEBUG)
	const cv::String WinNameCV = "Show";
	cv::Mat PreviewCV = cv::Mat(cv::Size(800, 800), CV_8UC3);
	cv::Mat DepthCV;
#endif
};
