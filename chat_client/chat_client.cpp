#include <iostream>
#include <thread>
#include <cstdlib>
#include <deque>
#include <cstring>
#include <string>
#include <cassert>
#include <boost/asio.hpp>
#include "chat_message.h"
#pragma comment(lib, "libboost_exception-vc141-mt-gd-x32-1_72.lib")
using namespace std;
using namespace boost::asio::ip;

using chat_message_queue = deque<chat_message>;

class chat_client {
public:
	/**
	 * @brief 构造
	 * @param io_service
	 * @param endpoint_iterator 对端信息
	 * @return 本类对象
	 */
	chat_client(boost::asio::io_service &io_service,
				tcp::resolver::iterator endpoint_iterator):
				io_service_(io_service), socket_(io_service){
		do_connect(endpoint_iterator);
	}

	/**
	 * @brief 将消息往服务端发送
	 * @param msg 消息引用
	 * @return
	 */
	void write(const chat_message &msg) {
		io_service_.post([this, msg]() {
			bool write_in_progress = !write_msgs_.empty();
			write_msgs_.push_back(msg);
			if (!write_in_progress) {
				do_write();
			}
		});
	}
	
	/**
	 * @brief 关闭本客户端的socket
	 * @param
	 * @return
	 */
	void close() {
		io_service_.post([this]() {
			socket_.close();
		});
	}

private:
	/**
	 * @brief 异步连接服务端
	 * @param endpoint_iterator 对端信息
	 * @return
	 */
	void do_connect(tcp::resolver::iterator endpoint_iterator) {
		boost::asio::async_connect(
			socket_,
			endpoint_iterator,
			[this](boost::system::error_code ec, tcp::resolver::iterator) {
				if (!ec) {
					do_read_header();
				}
			}
		);
	}

	/**
	 * @brief 读取消息头
	 * @param
	 * @return
	 */
	void do_read_header() {
		boost::asio::async_read(
			socket_,
			boost::asio::buffer(read_msg_.data(), chat_message::header_length),
			[this](boost::system::error_code ec, size_t) {
				if (!ec && read_msg_.decode_header()) {
					do_read_body();
				}
				else {
					socket_.close();
				}
			}
		);
	}

	/**
	 * @brief 读取消息体
	 * @param
	 * @return
	 */
	void do_read_body() {
		boost::asio::async_read(
			socket_,
			boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
			[this](boost::system::error_code ec, size_t) {
				if (!ec) {
					if (read_msg_.body_length() == sizeof(RoomInfomation) && 
						read_msg_.type() == MT_ROOM_INFO) {
						const RoomInfomation *info = reinterpret_cast<RoomInfomation *>(read_msg_.body());
						cout << "client: '";
						cout.write(info->name_.name, info->name_.name_len);
						cout << "' say '";
						cout.write(info->chat_.infomation, info->chat_.infomation_len);
						cout << endl;
					}
					do_read_header();
				}
				else {
					socket_.close();
				}
			}
		);
	}

	/**
	 * @brief 将要发送的消息一直发送直至没有消息发送
	 * @param
	 * @return
	 */
	void do_write() {
		boost::asio::async_write(
			socket_,
			boost::asio::buffer(write_msgs_.front().data(), 
								write_msgs_.front().length()),
			[this](boost::system::error_code ec, size_t) {
				if (!ec) {
					write_msgs_.pop_front();
					if (!write_msgs_.empty()) {
						do_write();
					}
				}
				else {
					socket_.close();
				}
			}
		);
	}
private:
	boost::asio::io_service &io_service_;
	tcp::socket socket_;
	chat_message read_msg_;
	chat_message_queue write_msgs_;
};

int main(int argc, const char* const* argv) {
	const char* ip = "192.168.0.102";
	const char* port = "8000";
	if (argc > 2) {
		ip = argv[1];
	}
	if (argc > 3) {
		port = argv[2];
	}
	try {
		boost::asio::io_service io_service;
		tcp::resolver resolver(io_service);
		auto endpoint_iterator = resolver.resolve(ip, port);
		chat_client c(io_service, endpoint_iterator);
		this_thread::sleep_for(1000ms);
		thread t([&io_service]() { io_service.run(); });
		char line[chat_message::max_body_length + 1] = { 0 };
		while (std::cin.getline(line, chat_message::max_body_length + 1)) {
			chat_message msg;
			auto type = 0;
			string input(line, line + strlen(line));
			string output;
			if (parse_message(input, &type, output)) {
				msg.set_message(type, output.data(), output.size());
				c.write(msg);
				cout << "write message for server " << output.size() << endl;
			}
		}
		c.close();
		t.join();
	}
	catch (exception &e) {
		cerr << "Exception " << e.what() << endl;
	}
	return 0;
}
