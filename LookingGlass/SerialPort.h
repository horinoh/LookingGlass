#pragma once

#include <asio.hpp>
#include <asio/use_awaitable.hpp>

#include "Common.h"

class SerialPort
{
public:
	enum class COM_NO: uint8_t {
		COM1 = 1,
		COM2 = 2,
		COM3 = 3,
		COM4 = 4,
		COM5 = 5,
		//....
	};

	SerialPort() : Context(), Port(Context) {}

	virtual bool Open(const COM_NO ComNo) {
		Port.open(std::format("COM{}", static_cast<uint8_t>(ComNo)), ErrorCode);
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
