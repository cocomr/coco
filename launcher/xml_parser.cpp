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

#include "xml_parser.h"

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	typedef HANDLE dlhandle;
	#define DLLEXT ".dll"
	#define DLLPREFIX "lib"
	#define dlopen(x,y) LoadLibrary(x)
	#define dlsym(x,y) GetProcAddress((HMODULE)x,y)
	#define dlerror() GetLastError()
	#define RTLD_NOW 0
	#define PATHSEP ';'
	#define DIRSEP '\\'
#else
	#include <dlfcn.h>
	typedef void* dlhandle;
	#define PATHSEP ':'
	#define DIRSEP '/'

	#ifdef __APPLE__
		#define DLLEXT ".dylib"
		#define DLLPREFIX "lib"
	#else
		#define DLLEXT ".so"
		#define DLLPREFIX "lib"
	#endif
#endif

namespace coco
{

bool XmlParser::parseFile(const std::string & config_file,
			   			  std::shared_ptr<TaskGraphSpec> app_spec)
{
	app_spec_ = app_spec;

	using namespace tinyxml2;
	XMLError error = xml_doc_.LoadFile(config_file_.c_str());
	if (error != XML_NO_ERROR)
    {
        std::cerr << "Error: " << error << std::endl <<
                     "While loading XML file: " << config_file << std::endl;
        return false;
    }

    XMLElement *package = xml_doc_.FirstChildElement("package");
    if (package == 0)
    {
        std::cerr << "Invalid xml configuration file " << config_file_
        		  << ", doesn't start withthe package block" std::endl;
        return false;
    }

    parseLogConfig(package->FirstChildElement("logconfig"));

    parsePaths(package->FirstChildElement("resourcespaths"));

    COCO_DEBUG("XmlParser") << "Parsing includes";
    parseIncludes(package->FirstChildElement("includes"));

    COCO_DEBUG("XmlParser") << "Parsing Components";
    parseComponents(package->FirstChildElement("components"), false);
	
	COCO_DEBUG("XmlParser") << "Parsing Connections";


	COCO_DEBUG("XmlParser") << "Parsing Activities";
}
	
void XmlParser::parseLogConfig(tinyxml2::XMLElement *logconfig)
{
	using namespace tinyxml2;
	
	/* Default initialization */
	if (!logconfig) 
    {
        COCO_INIT_LOG()
        COCO_LOG_INFO()
        return;
    }

    XMLElement *file = logconfig->FirstChildElement("file");
    if (file)
    {
        COCO_INIT_LOG(file->GetText())
        COCO_LOG_INFO()
        return;
    }

    COCO_INIT_LOG()
    XMLElement *levels_ele = logconfig->FirstChildElement("levels");
    if (levels_ele)
    {
        std::set<int> levels_set;
        std::string levels = levels_ele->GetText() != nullptr ? levels_ele->GetText() : "";
        if (!levels.empty())
        {
            std::stringstream ss_levels(levels);
            std::string level;
            char delim = ' ';
            while (std::getline(ss_levels, level, delim))
            {
                levels_set.insert((int)std::stoi(level.c_str()));
            }
        }
        coco::util::LoggerManager::getInstance()->setLevels(levels_set);
    }
    XMLElement *type_ele = logconfig->FirstChildElement("types");
    if (type_ele)
    {
        std::set<coco::util::Type> types_set;
        std::string types = type_ele->GetText() != nullptr ? type_ele->GetText() : "";
        if (!types.empty())
        {
            std::stringstream ss_types(types);
            std::string type;
            char delim = ' ';
            while (std::getline(ss_types, type, delim))
            {
                while (auto it = type.find(' ') != std::string::npos)
                    type.erase(it);
                if (type == "DEBUG" || type == "debug")
                    types_set.insert(coco::util::DEBUG);
                if (type == "ERR" || type == "err")
                    types_set.insert(coco::util::ERR);
            }
        }
        types_set.insert(coco::util::FATAL);
        coco::util::LoggerManager::getInstance()->setTypes(types_set);
    }
    XMLElement *out_file_ele = logconfig->FirstChildElement("outfile");
    if (out_file_ele)
    {
        auto text = out_file_ele->GetText();
        if (text)
            coco::util::LoggerManager::getInstance()->setOutLogFile(text);   
    }
    COCO_LOG_INFO()
}

void XmlParser::parsePaths(tinyxml2::XMLElement *paths)
{
	using namespace tinyxml2;

    if (!paths)
        return;

    /* Collect the lib path in an tmp vector, they will be checked to see if they are relative or global */
    XMLElement *libraries_path = paths->FirstChildElement("librariespath");
    std::vector<std::string> libraries_paths;
    while (libraries_path)
    {
        std::string path = libraries_path->GetText();
        if (path[path.size() - 1] != DIRSEP)
        	path.append(DIRSEP);
        libraries_paths.push_back(path);
        libraries_path = libraries_path->NextSiblingElement("librariespath");
    }
    /* Collect the path in an tmp vector, they will be checked to see if they are relative or global */
    XMLElement *path_ele = paths->FirstChildElement("path");
    std::vector<std::string> resources_paths;
    while (path)
    {
        std::string path = path_ele->GetText();
        if (path[path.size() - 1] != DIRSEP)
        	path.append(DIRSEP);
        resources_paths.push_back(path);
        path_ele = path_ele->NextSiblingElement("path");
    }

    /* COCO_PREFIX_PATH contains all the prefix for the specific platform divided by a : */
    const char* prefix = std::getenv("COCO_PREFIX_PATH");
    if (prefix)
    {
        for(auto p: coco::stringutil::splitter(prefix,':'))
        {
            if(p.empty())
                continue;
            if(p.back() != DIRSEP)
                p.append(DIRSEP);

            for (auto &path : resources_paths)
            {
                 /* Checks wheter the path providied in the xml are absolute or relative.
					The decision is made wheter the first character is a / or the ~.
					This is clearly not compatible with Windows */
				// TODO change for Windows
                if (path[0] != DIRSEP && path[0] != '~')
                    resources_paths_.push_back(p + path);
            }
            for (auto & lib_path : libraries_paths)
            {
                if (lib_path[0] != DIRSEP && lib_path[0] != '~')
                    libraries_paths_.push_back(p + lib_path);
            }
        }
    }

    app_spec_.resources_paths = resources_paths_;
}

void XmlParser::parseIncludes(tinyxml2::XMLElement *includes)
{

}

void XmlParser::parseComponents(tinyxml2::XMLElement *components, bool is_peer)
{
	if (!components)
	{
		if (!is_peer)
			COCO_DEBUG("XmlParser") << "No components in this configuration file";

		return;
	}
	
	XMLElement *component = components->FirstChildElement("component");
	while(component)
	{
		parseComponent(component, is_peer);
		component = component->NextSiblingElement("component");
	}
}

void XmlParser::parseComponent(tinyxml2::XMLElement *component, bool is_peer)
{
	using namespace tinyxml2;

	if (!component)
        return;

    std::shared_ptr<TaskSpec> task_spec(new TaskSpec);
    task_spec->is_peer = is_peer;

    XMLElement *task = component->FirstChildElement("task");
    if (!task)
        COCO_FATAL() << "No <task> tag in component\n";

    const char* task_name = task->GetText();

    // Looking if it has been specified a name different from the task name
	XMLElement *name = component->FirstChildElement("name");
	std::string instance_name;
	if (name)
	{
		istance_name = name->GetText();
		if (instance_name.empty())
			instance_name = task_name;
	}
	else
    {
		instance_name = task_name;
    }
    task_spec->instance_name = instance_name;
    task_spec->name = task_name;

    COCO_DEBUG("XmlParser") << "Parsing component: " << task_name
    						<< " with istance name: " << istance_name;


	/* Looking for the library if it exists */
	const char* library_name = component->FirstChildElement("library")->GetText();
	/* Checking if the library is present in the path */
	std::string library = findLibrary(library_name);

	if (library.is_empty())
		COCO_FATAL("XmlParser") << "Failed to find library with name: " << library_name;

	task_spec->library_name = library;	

	/* Parsing Attributes */


	/* Parsing Peers */
}	


std::string XmlParser::findLibrary(const std::string & library_name)
{
	boo found = false;
	std::string library;
	for (const auto & lib_path : libraries_paths_)
	{
		/* Path + "lib" + name + ".so/.dylib/.dll" */
		library = lib_path + DLLPREFIX + library_name + DLLEXT;
		if (dlopen(library.c_str(), RTLD_NOW))
		{
			found = true;
			break;
		}
	}
	if (found)
		return library;
	else
		return "";
}



}