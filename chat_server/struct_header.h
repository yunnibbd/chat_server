#pragma once
#include <cstring>
#include <string>

struct Header {
	int body_size_;
	int type_;
};

enum MessageType {
	MT_BIND_NAME = 1,
	MT_CHAT_INFO = 2,
	MT_ROOM_INFO = 3,
};

struct BindName {
	char name[32];
	int name_len;
};

struct ChatInformation {
	char infomation[256];
	int infomation_len;
};

struct RoomInfomation {
	BindName name_;
	ChatInformation chat_;
};

/**
 * @brief 将用户输入解析并存放到outbuffer中
 * @param input 用户输入
 * @param type 用户输入的类型
 * @param outbuffer 输出
 * @return bool 是否解析成功
 */
bool parse_message(const std::string &input, int *type, std::string &outbuffer);
