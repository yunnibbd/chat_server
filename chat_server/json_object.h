#pragma once
#include <sstream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using ptree = boost::property_tree::ptree;

/**
 * @brief ½«jsonĞ´Èëstringstream
 * @param
 * @return
 */
inline std::string ptree_to_json_string(const ptree &tree) {
	std::stringstream ss;
	boost::property_tree::write_json(ss, tree, true);
	return ss.str();
}
