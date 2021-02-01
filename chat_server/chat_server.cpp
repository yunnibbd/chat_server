#include <iostream>
#include <deque>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <sstream>
#include <cstdlib>
#include <boost/asio.hpp>
#include "serialize_object.h"
#include "chat_message.h"
#pragma comment(lib, "libboost_exception-vc141-mt-gd-x32-1_72.lib")
using namespace std;
using namespace boost::asio::ip;

using chat_message_queue = deque<chat_message>;

class chat_session;

//room
class chat_room {
public:
	using chat_session_ptr = shared_ptr<chat_session>;

	/**
	 * @brief 客户端加入事件
	 * @param cp 客户端智能指针
	 * @return
	 */
	void join(chat_session_ptr cp);

	/**
	 * @brief 客户端离开事件
	 * @param cp 客户端智能指针
	 * @return
	 */
	void leave(chat_session_ptr cp);

	/**
	 * @brief 给所有客户端分发消息
	 * @param msg 消息引用
	 * @return
	 */
	void deliver(const chat_message &msg);

private:
	set<chat_session_ptr> chat_sessions_;
	chat_message_queue recent_msgs_;
	enum { max_recent_msgs = 100 };
};

//client
class chat_session :
	public std::enable_shared_from_this<chat_session>{
public:
	chat_session(tcp::socket socket, chat_room &room)
		: socket_(std::move(socket)), room_(room){

	}

	/**
	 * @brief 触发客户端加入事件并读取消息头
	 * @param
	 * @return
	 */
	void start() {
		room_.join(shared_from_this());
		do_read_header();
	}

	/**
	 * @brief 将消息发送到服务端
	 * @param msg 消息引用
	 * @return
	 */
	void deliver(const chat_message &msg) {
		bool write_in_progress = !write_msgs_.empty();
		write_msgs_.push_back(msg);
		if (!write_in_progress) {
			//表示当前没有do_write在调用了
			do_write();
		}
	}

private:
	/**
	 * @brief 读取消息头
	 * @param
	 * @return
	 */
	void do_read_header() {
		auto self(shared_from_this());
		boost::asio::async_read(
			socket_,
			boost::asio::buffer(read_msg_.data(), chat_message::header_length),
			[this, self](boost::system::error_code ec, size_t) {
				if (!ec && read_msg_.decode_header()) {
					do_read_body();
				}
				else {
					room_.leave(shared_from_this());
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
		auto self(shared_from_this());
		boost::asio::async_read(
			socket_,
			boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
			[this, self](boost::system::error_code ec, size_t) {
				if (!ec) {
					//room_.deliver(read_msg_);
					handle_message();
					do_read_header();
				}
				else {
					room_.leave(shared_from_this());
				}
			}
		);
	}

	/**
	 * @brief 根据读到的消息反序列化程指定的类
	 * @param
	 * @return T 自定义类型
	 */
	template<typename T>
	T serialize_object() {
		T t;
		stringstream ss(string(read_msg_.body(), read_msg_.body() + read_msg_.body_length()));
		boost::archive::text_iarchive ia(ss);
		ia & t;
		return t;
	}

	/**
	 * @brief 根据消息类型处理消息
	 * @param
	 * @return
	 */
	void handle_message() {
		auto type = read_msg_.type();
		if (type == MT_BIND_NAME) {
			SBindName bind_name = serialize_object<SBindName>();
			bind_name_string_ = bind_name.bind_name();
		}
		else if (type == MT_CHAT_INFO) {
			SChatInfo chat = serialize_object<SChatInfo>();
			chat_information_string_ = chat.chat_information();
			auto rinfo = build_room_info();
			chat_message msg;
			msg.set_message(MT_ROOM_INFO, rinfo);
			room_.deliver(msg);
		}
		else {
			
		}
	}
	
	/**
	 * @brief 根据绑定好的名字构造一个聊天室信息
	 * @param
	 * @return string 返回一个序列化好了的聊天室信息
	 */
	string build_room_info() {
		SRoomInfo info(bind_name_string_, chat_information_string_);
		std::stringstream ss;
		boost::archive::text_oarchive oa(ss);
		oa & info;
		return ss.str();
	}

	/**
	 * @brief 重复发送消息队列中第一条,直至发送完成
	 * @param
	 * @return
	 */
	void do_write() {
		auto self(shared_from_this());
		boost::asio::async_write(
			socket_,
			boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
			[this, self](boost::system::error_code ec, size_t) {
				if (!ec) {
					write_msgs_.pop_front();
					if (!write_msgs_.empty()) {
						do_write();
					}
				}
				else {
					room_.leave(shared_from_this());
				}
			}
		);
	}
	
private:
	tcp::socket socket_;
	chat_room &room_;
	chat_message read_msg_;
	chat_message_queue write_msgs_;
	string bind_name_string_;
	string chat_information_string_;
};


/**
 * @brief 客户端加入事件
 * @param cp 客户端智能指针
 * @return
 */
void chat_room::join(chat_session_ptr cp) {
	chat_sessions_.insert(cp);
	for (const auto &msg : recent_msgs_)
		cp->deliver(msg);
}

/**
 * @brief 客户端离开事件
 * @param cp 客户端智能指针
 * @return
 */
void chat_room::leave(chat_session_ptr cp) {
	chat_sessions_.erase(cp);
}

/**
 * @brief 给所有客户端分发消息
 * @param msg 消息引用
 * @return
 */
void chat_room::deliver(const chat_message &msg) {
	recent_msgs_.push_back(msg);
	while (recent_msgs_.size() > max_recent_msgs)
		recent_msgs_.pop_front();

	for (auto &p : chat_sessions_)
		p->deliver(msg);
}

//chat server
class chat_server {
public:
	/**
	 * @brief 构造函数,并投递一个接受客户端的任务
	 * @param io_service
	 * @param endpoint 服务端协议和端口
	 * @param server_id 测试用,本服务的id
	 * @return 返回当前类对象
	 */
	chat_server(boost::asio::io_service &io_service,
		const tcp::endpoint &endpoint, int server_id = -1) 
		: acceptor_(io_service, endpoint), socket_(io_service), server_id_(server_id) {
		cout << "server " << server_id << " start!" << endl;
		do_accept();
	}

private:
	/**
	 * @brief 接受新客户端
	 * @param
	 * @return
	 */
	void do_accept() {
		acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
			if (!ec) {
				cout << socket_.remote_endpoint().address()
					 << ":" << socket_.remote_endpoint().port() << " join" << endl;
				auto session = make_shared<chat_session>(std::move(socket_), room_);
				session->start();
			}
			do_accept();
		});
	}
	
private:
	int server_id_ = -1;
	tcp::acceptor acceptor_;
	tcp::socket socket_;
	chat_room room_;
};

int main(int argc, const char *const *argv) {
	int server_port = 8000;
	int server_num = 2;
	if (argc >= 2) {
		server_port = atoi(argv[1]);
	}
	if (argc > 3) {
		server_num = atoi(argv[2]);
	}

	try {
		boost::asio::io_service io_service;
		list<chat_server> servers;
		for (int i = 0; i < server_num; ++i) {
			tcp::endpoint endpoint(tcp::v4(), server_port);
			servers.emplace_back(io_service, endpoint, i);
		}

		io_service.run();
	}
	catch (exception &e) {
		cerr << "Exception: " << e.what() << endl;
	}
	return 0;
}
