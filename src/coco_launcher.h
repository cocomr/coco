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
        void parseComponent(tinyxml2::XMLElement *component);
        void parseAttribute(tinyxml2::XMLElement *attributes, TaskContext *t);
        void parsePeers(tinyxml2::XMLElement *peers, TaskContext *t);
        void parseConnection(tinyxml2::XMLElement *connection);
        /*
        tinyxml2::XMLElement * findChild(tinyxml2::XMLElement * ,
                                         const char * child, bool required = true);
        const char * findChildText(tinyxml2::XMLElement * ,
                                   const char * child, bool required = true);
        */
		const std::string &config_file_;
		tinyxml2::XMLDocument doc_;
		std::map<std::string, TaskContext *> tasks_;
		std::list<std::string> peers_;
	};

    void printXMLSkeleton(std::string task_library, std::string task_library_path,
                          bool adddoc = false, bool savefile=true);
} // end of namespace coco
