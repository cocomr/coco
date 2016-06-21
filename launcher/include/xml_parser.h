/**
Copyright 2015, Filippo Brizzi"
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

@Author 
Filippo Brizzi, PhD Student
fi.brizzi@sssup.it
Emanuele Ruffaldi, PhD
e.ruffaldi@sssup.it

PERCRO, (Laboratory of Perceptual Robotics)
Scuola Superiore Sant'Anna
via Luigi Alamanni 13D, San Giuliano Terme 56010 (PI), Italy
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
	bool parseFile(const std::string & config_file,
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
    void parseFarm(tinyxml2::XMLElement *farm);

	tinyxml2::XMLDocument xml_doc_;
	std::shared_ptr<TaskGraphSpec> app_spec_;

	std::vector<std::string> resources_paths_;
    std::vector<std::string> libraries_paths_;

    

};

}
