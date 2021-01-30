#pragma once
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <cstring>
#include "struct_header.h"

class chat_message {
public:
	enum { header_length = sizeof(Header) };
	enum { max_body_length = 512 };
	chat_message() {}

	const char* data() const {
		return data_;
	}

	const int type() {
		return header_.type_;
	}

	char* data() {
		return data_;
	}

	const char* body() const {
		return data_ + header_length;
	}

	char* body() {
		return data_ + header_length;
	}

	size_t body_length() {
		return header_.body_size_;
	}

	void body_length(size_t new_length) {
		header_.body_size_ = new_length;
		if (header_.body_size_ > max_body_length) {
			header_.body_size_ = max_body_length;
		}
	}

	size_t length() const {
		return header_length + header_.body_size_;
	}

	/**
	 * @brief 将本类对象的消息类型，消息体，消息头都设置好，并将数据都存入data_里
	 * @param message_type 消息类型
	 * @param buffer 消息体指针
	 * @param buffer_size 消息体长度
	 * @return
	 */
	void set_message(int message_type, const void *buffer, size_t buffer_size) {
		assert(buffer_size <= max_body_length);
		header_.body_size_ = buffer_size;
		header_.type_ = message_type;
		memcpy(body(), buffer, buffer_size);
		memcpy(data(), &header_, header_length);
	}

	bool decode_header() {
		const Header *header = reinterpret_cast<const Header *>(data_);
		header_.body_size_ = header->body_size_;
		header_.type_ = header->type_;
		if (header_.body_size_ > max_body_length) {
			header_.body_size_ = 0;
			return false;
		}
		return true;
	}
private:
	Header header_;
	char data_[header_length + max_body_length] = { 0 };
};
