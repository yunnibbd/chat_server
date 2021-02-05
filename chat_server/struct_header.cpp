#include "struct_header.h"
#include "serialize_object.h"
#include "json_object.h"
#include "protocol.pb.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
using namespace std;

/**
 * @brief 序列化一个类
 * @param obj 类引用
 * @return string 序列化的结果
 */
template <typename T> std::string seriliaze(const T &obj) {
	std::stringstream ss;
	boost::archive::text_oarchive oa(ss);
	oa & obj;
	return ss.str();
}

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

/**
 * @brief 使用serialize将用户输入解析并存放到outbuffer中
 * @param input 用户输入
 * @param type 用户输入的类型
 * @param outbuffer 输出
 * @return bool 是否解析成功
 */
bool parse_message2(const std::string &input, int *type, std::string &outbuffer) {
	auto pos = input.find_first_of(" ");
	if (pos == std::string::npos)
		return false;
	if (pos == 0)
		return false;
	//"BindName ok" -> substr -> BindName
	auto command = input.substr(0, pos);
	if (command == "BindName") {
		std::string name = input.substr(pos + 1);
		if (name.size() > 32)
			return false;
		if (type)
			*type = MT_BIND_NAME;
		outbuffer = seriliaze(SBindName(std::move(name)));
		return true;
	}
	else if (command == "Chat") {
		std::string chat = input.substr(pos + 1);
		if (chat.size() > 256)
			return false;
		outbuffer = seriliaze(SChatInfo(std::move(chat)));
		if (type)
			*type = MT_CHAT_INFO;
		return true;
	}
	return false;
}

/**
 * @brief 使用ptree将用户输入解析为json并存放到outbuffer中
 * @param input 用户输入
 * @param type 用户输入的类型
 * @param outbuffer 输出
 * @return bool 是否解析成功
 */
bool parse_message3(const std::string &input, int *type, std::string &outbuffer) {
	auto pos = input.find_first_of(" ");
	if (pos == std::string::npos)
		return false;
	if (pos == 0)
		return false;
	//"BindName ok" -> substr -> BindName
	auto command = input.substr(0, pos);
	if (command == "BindName") {
		std::string name = input.substr(pos + 1);
		if (name.size() > 32)
			return false;
		if (type)
			*type = MT_BIND_NAME;
		ptree tree;
		tree.put("name", name);
		outbuffer = ptree_to_json_string(tree);
		return true;
	}
	else if (command == "Chat") {
		std::string chat = input.substr(pos + 1);
		if (chat.size() > 256)
			return false;
		ptree tree;
		tree.put("information", chat);
		outbuffer = ptree_to_json_string(tree);
		if (type)
			*type = MT_CHAT_INFO;
		return true;
	}
	return false;
}

/**
 * @brief 使用protobuf将用户输入解析为json并存放到outbuffer中
 * @param input 用户输入
 * @param type 用户输入的类型
 * @param outbuffer 输出
 * @return bool 是否解析成功
 */
bool parse_message4(const std::string &input, int *type, std::string &outbuffer) {
	auto pos = input.find_first_of(" ");
	if (pos == std::string::npos)
		return false;
	if (pos == 0)
		return false;
	//"BindName ok" -> substr -> BindName
	auto command = input.substr(0, pos);
	if (command == "BindName") {
		std::string name = input.substr(pos + 1);
		if (name.size() > 32)
			return false;
		if (type)
			*type = MT_BIND_NAME;
		PBindName bindName;
		bindName.set_name(name);
		auto ok = bindName.SerializeToString(&outbuffer);
		return ok;
	}
	else if (command == "Chat") {
		std::string chat = input.substr(pos + 1);
		if (chat.size() > 256)
			return false;
		PChat pchat;
		pchat.set_information(chat);
		auto ok = pchat.SerializeToString(&outbuffer);
		if (type)
			*type = MT_CHAT_INFO;
		return ok;
	}
	return false;
}
