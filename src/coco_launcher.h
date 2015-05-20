/**
 * Compact Framework Core
 * 2014-2015 Emanuele Ruffaldi and Filippo Brizzi @ Scuola Superiore Sant'Anna, Pisa, Italy
 */
#pragma once
#include "ezoro.hpp"
#include <exception>

namespace coco
{
	/**
	 * Launcher class that takes a XML file and creates the network
	 */
	class CocoLauncher
	{
	public:
	    CocoLauncher(const std::string &config_file)
	    	: config_file_(config_file) {}

	    bool createApp();
	    void startApp();

	private:
        void parseLogConfig(tinyxml2::XMLElement *logconfig);
        void parsePaths(tinyxml2::XMLElement *paths);
        void parseComponent(tinyxml2::XMLElement *component, bool is_peer = false);
        void parseSchedule(tinyxml2::XMLElement *schedule_policy, TaskContext *t);
        void parseAttribute(tinyxml2::XMLElement *attributes, TaskContext *t);
        std::string checkResource(const std::string &value);
        void parsePeers(tinyxml2::XMLElement *peers, TaskContext *t);
        void parseConnection(tinyxml2::XMLElement *connection);

		const std::string &config_file_;
		tinyxml2::XMLDocument doc_;
		std::map<std::string, TaskContext *> tasks_;
		std::list<std::string> peers_;

        std::vector<std::string> resources_paths_;
        std::string libraries_path_ = "";
	};

    void printXMLSkeleton(std::string task_library, std::string task_library_path,
                          bool adddoc = false, bool savefile=true);
} // end of namespace coco
