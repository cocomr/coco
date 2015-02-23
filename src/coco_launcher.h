#pragma once
#include "ezoro.hpp"

namespace coco
{

class CocoLauncher
{
public:
    CocoLauncher(const std::string &config_file)
    	: config_file_(config_file) {}
    void createApp();
    void startApp();
private:
	void parseComponent(tinyxml2::XMLElement *comoponent);
	void parseConnection(tinyxml2::XMLElement *connection);

	const std::string &config_file_;
	tinyxml2::XMLDocument doc_;
	std::map<std::string, TaskContext *> tasks_;
	std::list<std::string> peers_;
};

void printXMLSkeleton(std::string task_name, std::string task_library, std::string task_library_path,
					  bool adddoc = false, bool savefile=true);
} // end of namespace coco