#pragma once

#include <asio.hpp>
#include <asio/use_awaitable.hpp>

#include "Common.h"

class Http
{
public:
	//!< auto Ht = new Http("www.google.com", "http");
	Http(const std::string& Server, const std::string& Service) : Context(), Resolver(Context), Socket(Context), Server_(Server) {
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

#if 1
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
	void GetRequest(std::string_view Path) { GetRequest(Server_, Path); }
#else
	void GetRequestAsync(std::string_view Server, std::string_view Path) {
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
	void GetRequestAsync(std::string_view Path) { GetRequestAsync(Server_, Path); }
#endif

	virtual void Update() {
		GetRequest("/");
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

	std::string Server_;
};
