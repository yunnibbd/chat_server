#pragma once
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <string>

class SBindName {
public:
	SBindName(std::string name) : bind_name_(name) {}
	SBindName(){}
	
	const std::string &bind_name() const { return bind_name_; }

	friend class boost::serialization::access;
	//如果类 Archive 是一个输出存档，则操作符 & 被定义为 <<.  同样，如果类 Archive
	//是一个输入存档，则操作符 & 被定义为 >>.
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & bind_name_;
	}

private:
	std::string bind_name_;
};

class SChatInfo {
public:
	SChatInfo(std::string info) : chat_information_(info) {}
	SChatInfo() {}

	const std::string &chat_information() const { return chat_information_; }

	friend class boost::serialization::access;
	//如果类 Archive 是一个输出存档，则操作符 & 被定义为 <<.  同样，如果类 Archive
	//是一个输入存档，则操作符 & 被定义为 >>.
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & chat_information_;
	}
private:
	std::string  chat_information_;
};

class SRoomInfo {
public:
	SRoomInfo() {}
	SRoomInfo(std::string name, std::string info)
		:sbind_name_(std::move(name)), schat_info_(std::move(info)){}
	
	const std::string &name() const { return sbind_name_.bind_name(); }
	const std::string &info() const { return schat_info_.chat_information(); }

	friend class boost::serialization::access;
	//如果类 Archive 是一个输出存档，则操作符 & 被定义为 <<.  同样，如果类 Archive
	//是一个输入存档，则操作符 & 被定义为 >>.
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & sbind_name_;
		ar & schat_info_;
	}
private:
	SBindName sbind_name_;
	SChatInfo schat_info_;
};
