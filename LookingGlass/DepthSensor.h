#pragma once

#pragma once

#include <format>
#include <algorithm>
#include <thread>
#include <random>

#include <asio.hpp>
#include <asio/use_awaitable.hpp>

#include "Common.h"

//!< (DEBUG �r���h��) �璷�Ƀ��O���o�͂���A���O�o�͂̉e���ŉ�ʂ��������������
//#define LOG_VERBOSE

class Http 
{
public:
	Http(asio::io_context& Context, std::string_view Server, std::string_view Path) 
		: Resolver(Context), Socket(Context)
	{
		asio::ip::tcp::resolver::query Query(std::string(Server), "http");
		Resolver.async_resolve(Query, [&](const asio::error_code EC, const asio::ip::tcp::resolver::iterator& EndPoint) {
			if (!EC) {
				LOG(std::data(std::format("Resolve [OK]\n")));

				//!< �ڑ�
				asio::async_connect(Socket, EndPoint, [&](const asio::error_code EC_Cnt, const asio::ip::tcp::resolver::iterator&) {
					if (!EC_Cnt) {
						LOG(std::data(std::format("Connect [OK]\n")));

						//!< ���N�G�X�g����
						std::ostream RequestStream(&Request);
						RequestStream << "GET " << Path << " HTTP/1.0\r\n";
						RequestStream << "Host: " << Server << "\r\n";
						RequestStream << "Accept: */*\r\n";
						RequestStream << "Connection: close\r\n\r\n";
						asio::async_write(Socket, Request, [&](const asio::error_code EC_Req, const size_t ReqSize) {
							if (!EC_Req) {
								LOG(std::data(std::format("Write Request ({}Bytes) [OK]\n", ReqSize)));

								//!< ���X�|���X�Ǎ�
								asio::async_read_until(Socket, Response, "\r\n", [&](const asio::error_code EC_Res, const size_t ResSize) {
									if (!EC_Res) {
										LOG(std::data(std::format("Read Response ({}Bytes) [OK]\n", ResSize)));

										//!< ���X�|���X�� OK ���ǂ���
										std::istream ResponseStream(&Response);
										std::string HttpVersion;
										ResponseStream >> HttpVersion;
										unsigned int StatusCode;
										ResponseStream >> StatusCode;
										std::string StatusMessage;
										std::getline(ResponseStream, StatusMessage);
										if (ResponseStream && HttpVersion.substr(0, 5) == "HTTP/" && 200 == StatusCode) {
											LOG(std::data(std::format("Response (StatusCode = {}, StatusMessage = {}) [OK]\n", StatusCode, StatusMessage)));
											//!< �w�b�_�Ǎ� (��s�܂œǂݍ���)
											asio::async_read_until(Socket, Response, "\r\n\r\n", [&](const asio::error_code EC_Hd, const size_t HdSize) {
												if (!EC_Hd) {
													LOG(std::data(std::format("Read Header ({}Bytes) [OK]\n", HdSize)));

													//!< ���X�|���X�w�b�_������
													LOG("Header\n");
													std::istream ResponseHeaderStream(&Response);
													std::string Header;
													while (std::getline(ResponseHeaderStream, Header) && Header != "\r") {
														LOG(std::data(std::format("\t{}\n", Header)));
													}
													LOG(std::data(std::format("\n")));

													//!< �R���e���c�o��
													LOG("Contents\n");
													if (Response.size() > 0) {
														LOG(std::data(std::format("\t{}\n", asio::buffer_cast<const char*>(Response.data()))));
													}

													//!< �c��̃f�[�^�� EOF �܂œǂݍ���
													LOG("Remains\n");
													asio::error_code ErrorCode;
													int count = 0;
													while (asio::read(Socket, Response, asio::transfer_at_least(1), ErrorCode)) {
														LOG(std::data(std::format("\t{}\n", asio::buffer_cast<const char*>(Response.data()))));
														++count;
													}
													if (asio::error::eof == ErrorCode) {
														LOG(std::data(std::format("[EOF]\n")));
													}
													//asio::async_read(Socket, Response, asio::transfer_at_least(1), asio::use_awaitable);
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

	//asio::awaitable<void> OnContent(const asio::error_code EC, const size_t Size)
	//{
	//	if (!EC) {
	//		LOG(std::data(std::format("Read Rest {} [OK]\n", Size)));
	//		LOG(std::data(std::format("Response {}\n", reinterpret_cast<const char*>(Response.data().data()))));

	//		asio::async_read(Socket, Response, asio::transfer_at_least(1), asio::use_awaitable);
	//	}
	//	else if (asio::error::eof == EC) {
	//		LOG(std::data(std::format("[EOF]\n")));
	//	}
	//}

protected:
	asio::ip::tcp::resolver Resolver;
	asio::ip::tcp::socket Socket;
	asio::streambuf Request;
	asio::streambuf Response;
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

class DepthSensorSerialPort : public SerialPort
{
private:
	using Super = SerialPort;

public:
	DepthSensorSerialPort() {
#if defined(USE_CV) && defined(_DEBUG)
		cv::imshow(WinNameCV, PreviewCV);
		//cv::createTrackbar("X", WinNameCV, &X, XMax, [](int Value, void* Userdata) { static_cast<XXX*>(Userdata)->Xxx(Value); }, this);
		//cv::createTrackbar("Y", WinNameCV, &Y, YMax, [](int Value, void* Userdata) { static_cast<YYY*>(Userdata)->Yyy(Value); }, this);
#endif
	}

	virtual void UpdateAsyncStart() {
		if (!Thread.joinable()) {
			Thread = std::thread([&]() {
				//!< �X���b�h�͊O����I�������Ȃ��A�t���O�������ĂăX���b�h���g�ɏI�����Ă��炤
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
	virtual void Exit() override {
		ExitThread();
		ExitSerialPort();
	}

	virtual void UpdateCV() {
#if defined(USE_CV) && defined(_DEBUG)
		cv::imshow(WinNameCV, PreviewCV);
#endif
	}

protected:
	std::thread Thread;
	std::mutex Mutex;
	bool IsExitThread = false;

#if defined(USE_CV) && defined(_DEBUG)
	const cv::String WinNameCV = "Show";
	cv::Mat PreviewCV = cv::Mat(cv::Size(800, 800), CV_8UC3);
	cv::Mat DepthCV;
#endif
};

//!< �[�x�Z���T MaixSense A010
class DepthSensorA010 : public DepthSensorSerialPort
{
private:
	using Super = DepthSensorSerialPort;

public:
	DepthSensorA010() {}
	virtual ~DepthSensorA010() {}
	
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
					//!< �t���[���J�n�̃w�b�_����
#ifdef LOG_VERBOSE
					LOG(std::data(std::format("[A010] Frame hader = {:#x}\n", FrameBegin)));
#endif
					//!< �f�[�^�T�C�Y�擾
					uint16_t FrameDataLen;
					Port.read_some(asio::buffer(&FrameDataLen, sizeof(FrameDataLen)), ErrorCode); VerifyError();
#ifdef LOG_VERBOSE
					LOG(std::data(std::format("[A010] Fame data length = {}\n", FrameDataLen)));
#endif
					//!< �t���[���f�[�^�擾
					Mutex.lock(); {
						Port.read_some(asio::buffer(&Frame, FrameDataLen), ErrorCode); VerifyError();
						//!< ���������ʂƂȂ�悤�ɔ��]
						std::ranges::transform(Frame.Payload, std::begin(Frame.Payload), [](const uint8_t i) { return 0xff - i; });
#if defined(USE_CV) && defined(_DEBUG)
						DepthCV = cv::Mat(cv::Size(Frame.Header.Cols, Frame.Header.Rows), CV_8UC1, std::data(Frame.Payload), cv::Mat::AUTO_STEP);
#endif
					} Mutex.unlock();
					OnFrame();

					//!< �`�F�b�N�T��
#ifdef _DEBUG
					{
						//!< �t���[���J�n�w�b�_�ȍ~�����܂ł́u(�o�C�g���ł͂Ȃ�)�l�v�����Z�������̂̉��� 8 �r�b�g�����߂�
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

					//!< �G���h�I�u�p�P�b�g
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
	virtual void UpdateCV() override {
#if defined(USE_CV) && defined(_DEBUG)
		//!< �����摜���܂Ƃ߂ĕ\���������ꍇ�� cv::hconcat() �� cv::vconcat() ���g��
		cv::resize(DepthCV, DepthCV, PreviewCV.size());
		DepthCV.copyTo(PreviewCV);
#endif
		Super::UpdateCV();
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
	//!< �I���I�t (Image Signal Processor)
	bool SetCmdISP(const ISP Arg) { return SetCmd("ISP", static_cast<const uint8_t>(Arg)); }
	//!< �r�j���O (�𑜓x) (Binning)
	bool SetCmdBINN(const BINN Arg) { return SetCmd("BINN", static_cast<const uint8_t>(Arg)); }
	//!< �\���ݒ� (Display)
	bool SetCmdDISP(const DISP Arg) { return SetCmd("DISP", static_cast<const uint8_t>(Arg)); }
	//!< �{�[���[�g (UART��) (Baudrate)
	bool SetCmdBAUD(const BAUD Arg) { return SetCmd("BAUD", static_cast<const uint8_t>(Arg)); }
	//!< �ʎq�����j�b�g (Quantization unit) �����Ȓl���A�ߋ������ڍׂɂȂ�A�������͈͂������Ȃ�
	bool SetCmdUNIT(const uint8_t Arg) { return SetCmd("UNIT", (std::clamp)(Arg, static_cast<uint8_t>(UNIT::Min), static_cast<uint8_t>(UNIT::Max))); }
	bool SetCmdUNIT(const UNIT Arg) { return SetCmdUNIT(static_cast<const uint8_t>(Arg)); }
	//!< �t���[�����[�g (Frame per second)
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
		//!< 16 �o�C�g
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
};
