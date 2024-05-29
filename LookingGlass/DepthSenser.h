#pragma once

#pragma once

#include <format>
#include <algorithm>
#include <thread>

#include <asio.hpp>

#include "Common.h"

class DepthSenser
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
	
	DepthSenser() : Context(), SerialPort(Context) {}
	virtual ~DepthSenser() {
		if (SerialPort.is_open()) {
			SerialPort.close();
		}
	}

	virtual bool Open(const COM Com) {
		SerialPort.open(std::format("COM{}", static_cast<uint8_t>(Com)), ErrorCode);
		if (!VerifyError()) {
			SerialPort.set_option(asio::serial_port::baud_rate(115200));
			SerialPort.set_option(asio::serial_port::character_size(8));
			SerialPort.set_option(asio::serial_port::parity(asio::serial_port::parity::none));
			SerialPort.set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one));
			SerialPort.set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none));
			return true;
		}
		return false;
	}
	virtual void Update() {}
	virtual void OnDepthUpdate() {}

	virtual bool VerifyError() {
		if (ErrorCode) {
			LOG(std::data(std::format("{}\n", ErrorCode.message())));
			return true;
		}
		return false;
	}

protected:
	asio::io_context Context;
	asio::serial_port SerialPort;
	asio::error_code ErrorCode;
};

//!< MaixSense A010
class DepthSenserA010 : public DepthSenser
{
private:
	using Super = DepthSenser;

public:
	DepthSenserA010() {}
	virtual ~DepthSenserA010() {
		if (SerialPort.is_open()) {
		}
	}

	virtual bool Open(const COM Com) override {
		if (Super::Open(Com)) {
			return true;
		}
		return false;
	}

	virtual void Update() override {
		while (true) {
			uint16_t FrameBegin;
			SerialPort.read_some(asio::buffer(&FrameBegin, sizeof(FrameBegin)), ErrorCode); VerifyError();
			if (static_cast<uint16_t>(FRAME_FLAG::Begin) == FrameBegin) {
				//!< フレーム開始のヘッダ発見
				LOG(std::data(std::format("[A010] Frame hader = {:#x}\n", FrameBegin)));

				//!< データサイズ取得
				uint16_t FrameDataLen;
				SerialPort.read_some(asio::buffer(&FrameDataLen, sizeof(FrameDataLen)), ErrorCode); VerifyError();
				LOG(std::data(std::format("[A010] Fame data length = {}\n", FrameDataLen)));

				//!< フレームデータ取得
				SerialPort.read_some(asio::buffer(&Frame, FrameDataLen), ErrorCode); VerifyError();
				OnFrame();

				{
					//!< フレーム開始ヘッダ以降ここまでの「(バイト数ではなく)値」を加算したものの下位 8 ビットを求める
					const auto SumInit = (static_cast<uint16_t>(FRAME_FLAG::Begin) & 0xff) + (static_cast<uint16_t>(FRAME_FLAG::Begin) >> 8) + (FrameDataLen & 0xff) + (FrameDataLen >> 8);
					auto P = reinterpret_cast<const uint8_t*>(&Frame);
					const auto Sum = std::accumulate(P, P + FrameDataLen, SumInit) & 0xff;

					//!< チェックサム
					uint8_t CheckSum;
					SerialPort.read_some(asio::buffer(&CheckSum, sizeof(CheckSum)), ErrorCode); VerifyError();
					LOG(std::data(std::format("[A010] CheckSum = {} = {}\n", CheckSum, Sum)));
				}

				//!< エンドオブパケット
				uint8_t EOP;
				SerialPort.read_some(asio::buffer(&EOP, sizeof(EOP)), ErrorCode); VerifyError();
				LOG(std::data(std::format("[A010] EndOfPacket = {:#x} = {:#x}\n\n", static_cast<uint8_t>(FRAME_FLAG::End), EOP)));

				//break;
			}
		}
	}

	virtual void OnFrame() {
		LOG(std::data(std::format("[A010]\tReserved1 = {:#x}\n", Frame.Header.Reserved1)));
		LOG(std::data(std::format("[A010]\tOutputMode = {}\n", 0 == Frame.Header.OutputMode ? "Depth" :"Depth + IR")));
		LOG(std::data(std::format("[A010]\tSenserTemp = {}, DriverTemp = {}\n", Frame.Header.SenserTemp, Frame.Header.DriverTemp)));
		LOG(std::data(std::format("[A010]\tExposureTime = {}, {}, {}, {}\n", Frame.Header.ExposureTime[0], Frame.Header.ExposureTime[1], Frame.Header.ExposureTime[2], Frame.Header.ExposureTime[3])));
		LOG(std::data(std::format("[A010]\tErrorCode = {}\n", Frame.Header.ErrorCode)));
		LOG(std::data(std::format("[A010]\tReserved2 = {:#x}\n", Frame.Header.Reserved2)));
		LOG(std::data(std::format("[A010]\tCols x Rows = {} x {}\n", Frame.Header.Cols, Frame.Header.Rows)));
		LOG(std::data(std::format("[A010]\tFrameId = {}\n", Frame.Header.FrameId)));
		LOG(std::data(std::format("[A010]\tISPVersion = {}\n", Frame.Header.ISPVersion)));
		LOG(std::data(std::format("[A010]\tReserved3 = {:#x}\n", Frame.Header.Reserved3)));

		LOG(std::data(std::format("[A010]\t\tPayload = {}, {}, {}, {},...\n", Frame.Payload[0], Frame.Payload[1], Frame.Payload[2], Frame.Payload[3])));
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
	bool SetCmd(std::string_view Cmd, const uint8_t Arg) {
		if (SerialPort.is_open()) {
			const auto CmdStr = std::format("AT+{}={}\r", std::data(Cmd), Arg);
			LOG(std::data(std::format("[A010] {}\n", CmdStr)));
			SerialPort.write_some(asio::buffer(CmdStr)); VerifyError();
		}
		return false;
	}
	//!< オンオフ (Image Signal Processor)
	bool SetCmdISP(const ISP Arg) { return SetCmd("ISP", static_cast<const uint8_t>(Arg)); }
	//!< ビニング(解像度) (Binning)
	bool SetCmdBINN(const BINN Arg) { return SetCmd("BINN", static_cast<const uint8_t>(Arg)); }
	//!< 表示設定 (Display)
	bool SetCmdDISP(const DISP Arg) { return SetCmd("DISP", static_cast<const uint8_t>(Arg)); }
	//!< ボーレート(UART時) (Baudrate)
	bool SetCmdBAUD(const BAUD Arg) { return SetCmd("BAUD", static_cast<const uint8_t>(Arg)); }
	//!< 量子化ユニット (Quantization unit) 小さな値程、近距離が詳細になり、遠距離
	bool SetCmdUNIT(const uint8_t Arg) { return SetCmd("UNIT", (std::clamp)(Arg, uint8_t(0), uint8_t(10))); }
	//!< フレームレート (Frame per second)
	bool SetCmdFPS(const uint8_t Arg) { return SetCmd("FPS", (std::clamp)(Arg, uint8_t(1), uint8_t(19))); }

	bool WaitOK() {
		if (SerialPort.is_open()) {
			std::array<char, 3> Res;
			SerialPort.read_some(asio::buffer(std::data(Res), sizeof(Res)), ErrorCode); VerifyError();
			if ('O' == Res[0] && 'K' == Res[1] && '\r' == Res[2]) {
				LOG(std::data(std::format("[A010] {}{}\n", Res[0], Res[1])));
				return true;
			}
		}
		return false;
	}

	typedef struct {
		//!< 16 バイト
		uint8_t Reserved1;		//!< 0xff
		uint8_t OutputMode;		//!< 0:depth only, 1:depth+ir
		uint8_t SenserTemp;
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

	FRAME Frame;

protected:
};
