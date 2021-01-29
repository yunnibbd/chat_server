#include "struct_header.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

/**
 * @brief 将用户输入解析并存放到outbuffer中
 * @param input 用户输入
 * @param type 用户输入的类型
 * @param outbuffer 输出
 * @return bool 是否解析成功
 */
bool parse_message(const std::string &input, int *type, std::string &outbuffer) {
	//input should be cmd body
	auto pos = input.find_first_of(" ");
	if (pos == std::string::npos)
		return false;
	if (pos == 0)
		return false;
	//"BindName ok" -> substr -> BindName
	auto command = input.substr(0, pos);
	if (command == "BindName") {
		//we try to bind name
		std::string name = input.substr(pos + 1);
		if (name.size() > 32)
			return false;
		if (type)
			*type = MT_BIND_NAME;
		BindName bindInfo;
		bindInfo.name_len = name.size();
		std::memcpy(&(bindInfo.name), name.data(), name.size());
		auto buffer = reinterpret_cast<const char *>(&bindInfo);
		outbuffer.assign(buffer, buffer + sizeof(bindInfo));
		return true;
	}
	else if (command == "Chat") {
		//we try to chat
		std::string chat = input.substr(pos + 1);
		if (chat.size() > 256)
			return false;
		ChatInformation info;
		info.infomation_len = chat.size();
		std::memcpy(&(info.infomation), chat.data(), chat.size());
		auto buffer = reinterpret_cast<const char *>(&info);
		outbuffer.assign(buffer, buffer + sizeof(info));
		if (type)
			*type = MT_CHAT_INFO;
		return true;
	}
	return false;
}
