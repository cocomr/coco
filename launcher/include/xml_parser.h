/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#pragma once
#include <unordered_map>
#include <unordered_set>

#include "tinyxml2/tinyxml2.h"

#include "coco/register.h"
#include "graph_spec.h"

namespace coco
{
class XmlParser
{
public:
	bool parseFile(const std::string &config_file,
				   std::shared_ptr<TaskGraphSpec> app_spec, bool first = true);
	bool createXML(const std::string &xml_file,
				   std::shared_ptr<TaskGraphSpec> app_spec);
	
private:
	void parseLogConfig(tinyxml2::XMLElement *logconfig);
	void parsePaths(tinyxml2::XMLElement *paths);
    void parseIncludes(tinyxml2::XMLElement *includes);
    void parseInclude(tinyxml2::XMLElement *include);
	void parseComponents(tinyxml2::XMLElement *components,
                         TaskSpec * task_owner);
	void parseComponent(tinyxml2::XMLElement *component,
                        TaskSpec * task_owner);
    //std::string findLibrary(const std::string & library_name);
	void parseAttribute(tinyxml2::XMLElement *attributes,
                        TaskSpec * task_spec);
    std::string checkResource(const std::string &resource, bool is_library = false);
	void parseConnections(tinyxml2::XMLElement *connections);
	void parseConnection(tinyxml2::XMLElement *connection);
	void parseActivities(tinyxml2::XMLElement *activities);
	void parseSchedule(tinyxml2::XMLElement *schedule_policy,
                       SchedulePolicySpec &policy, bool &is_parallel);
	void parseActivity(tinyxml2::XMLElement *activity);
    void parsePipeline(tinyxml2::XMLElement *pipeline);
    void addPipelineConnections(std::unique_ptr<PipelineSpec> &pipe_spec);
    void parseFarm(tinyxml2::XMLElement *farm);

    tinyxml2::XMLElement* xmlNodeTxt(tinyxml2::XMLElement * parent,
                                     const std::string &tag,
                                     const std::string text);
    void createComponent(std::shared_ptr<TaskSpec> task,
    					 tinyxml2::XMLElement *components);
    void createConnection(std::unique_ptr<ConnectionSpec> &connection_spec,
                          tinyxml2::XMLElement *connections);

private:
	tinyxml2::XMLDocument xml_doc_;
	std::shared_ptr<TaskGraphSpec> app_spec_;

	std::vector<std::string> resources_paths_;
    std::vector<std::string> libraries_paths_;
};

}
