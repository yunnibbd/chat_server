#include <iostream>
#include <deque>
#include <list>
#include <memory>
#include <set>
#include <cstdlib>
#include <boost/asio.hpp>
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
	 * @brief �ͻ��˼����¼�
	 * @param cp �ͻ�������ָ��
	 * @return
	 */
	void join(chat_session_ptr cp) {
		chat_sessions_.insert(cp);
		for (const auto &msg : recent_msgs_)
			cp->deliver(msg);
	}

	/**
	 * @brief �ͻ����뿪�¼�
	 * @param cp �ͻ�������ָ��
	 * @return
	 */
	void leave(chat_session_ptr cp) {
		chat_sessions_.erase(cp);
	}

	/**
	 * @brief �����пͻ��˷ַ���Ϣ
	 * @param msg ��Ϣ����
	 * @return
	 */
	void deliver(const chat_message &msg) {
		recent_msgs_.push_back(msg);
		while (recent_msgs_.size() > max_recent_msgs)
			recent_msgs_.pop_front();

		for (auto &p : chat_sessions_)
			p->deliver(msg);
	}

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
	 * @brief �����ͻ��˼����¼�����ȡ��Ϣͷ
	 * @param
	 * @return
	 */
	void start() {
		room_.join(shared_from_this());
		do_read_header();
	}

	/**
	 * @brief ����Ϣ���͵������
	 * @param msg ��Ϣ����
	 * @return
	 */
	void deliver(const chat_message &msg) {
		bool write_in_progress = !write_msgs_.empty();
		write_msgs_.push_back(msg);
		if (!write_in_progress) {
			//��ʾ��ǰû��do_write�ڵ�����
			do_write();
		}
	}

private:
	/**
	 * @brief ��ȡ��Ϣͷ
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
	 * @brief ��ȡ��Ϣ��
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
					room_.deliver(read_msg_);
					cout << "recv from client:" << read_msg_.body() << endl;
					do_read_header();
				}
				else {
					room_.leave(shared_from_this());
				}
			}
		);
	}

	
	/**
	 * @brief �ظ�������Ϣ�����е�һ��,ֱ���������
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
};

//chat server
class chat_server {
public:
	/**
	 * @brief ���캯��,��Ͷ��һ�����ܿͻ��˵�����
	 * @param io_service
	 * @param endpoint �����Э��Ͷ˿�
	 * @param server_id ������,�������id
	 * @return ���ص�ǰ�����
	 */
	chat_server(boost::asio::io_service &io_service,
		const tcp::endpoint &endpoint, int server_id = -1) 
		: acceptor_(io_service, endpoint), socket_(io_service), server_id_(server_id) {
		cout << "server " << server_id << " start!" << endl;
		do_accept();
	}

private:
	/**
	 * @brief �����¿ͻ���
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
