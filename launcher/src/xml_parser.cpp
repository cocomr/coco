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

#include "coco/util/accesses.hpp"
#include "tinyxml2/tinyxml2.h"
#include "graph_spec.h"
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
    if (!app_spec)
    {
        std::cerr << "Error. TaskGraphSpec pointer need to be initialized!" << std::endl;
        return false;
    }
    app_spec_ = app_spec;

	using namespace tinyxml2;
	XMLError error = xml_doc_.LoadFile(config_file.c_str());
	if (error != XML_NO_ERROR)
    {
        std::cerr << "Error: " << error << std::endl <<
                     "While loading XML file: " << config_file << std::endl;
        return false;
    }

    XMLElement *package = xml_doc_.FirstChildElement("package");
    if (package == 0)
    {
        std::cerr << "Invalid xml configuration file " << config_file
        		  << ", doesn't start withthe package block" << std::endl;
        return false;
    }
    const char* name = package->Attribute("name");
    app_spec_->name = name ? name : "<not defined>";

    parseLogConfig(package->FirstChildElement("log"));

    parsePaths(package->FirstChildElement("paths"));

    COCO_DEBUG("XmlParser") << "Parsing includes";
    parseIncludes(package->FirstChildElement("includes"));

    COCO_DEBUG("XmlParser") << "Parsing Components";
    parseComponents(package->FirstChildElement("components"), nullptr);
	
	COCO_DEBUG("XmlParser") << "Parsing Connections";
    parseConnections(package->FirstChildElement("connections"));

	COCO_DEBUG("XmlParser") << "Parsing Activities";
    parseActivities(package->FirstChildElement("activities"));

    return true;
}
	
void XmlParser::parseLogConfig(tinyxml2::XMLElement *logconfig)
{
	using namespace tinyxml2;
	
	/* Default initialization */
	if (!logconfig) 
    {
        COCO_INIT_LOG();
        COCO_LOG_INFO();
        return;
    }

    XMLElement *file = logconfig->FirstChildElement("file");
    if (file)
    {
        COCO_INIT_LOG(file->GetText());
        COCO_LOG_INFO();
        return;
    }

    COCO_INIT_LOG();
    XMLElement *levels_ele = logconfig->FirstChildElement("levels");
    if (levels_ele)
    {
        std::unordered_set<int> levels_set;
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
        util::LoggerManager::instance()->setLevels(levels_set);
    }
    XMLElement *type_ele = logconfig->FirstChildElement("types");
    if (type_ele)
    {
        std::unordered_set<util::Type, util::enum_hash> types_set;
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
                    types_set.insert(coco::util::Type::DEBUG);
                if (type == "ERR" || type == "err")
                    types_set.insert(coco::util::Type::ERR);
            }
        }
        types_set.insert(coco::util::Type::FATAL);
        coco::util::LoggerManager::instance()->setTypes(types_set);
    }
    XMLElement *out_file_ele = logconfig->FirstChildElement("outfile");
    if (out_file_ele)
    {
        auto text = out_file_ele->GetText();
        if (text)
            coco::util::LoggerManager::instance()->setOutLogFile(text);
    }
    COCO_LOG_INFO()
}

void XmlParser::parsePaths(tinyxml2::XMLElement *paths)
{
	using namespace tinyxml2;

    if (!paths)
        return;

    /* Collect the path in an tmp vector, they will be checked to see if they are relative or global */
    XMLElement *path_ele = paths->FirstChildElement("path");
    std::vector<std::string> resources_paths;
    while (path_ele)
    {
        std::string path = path_ele->GetText();
        if (path[path.size() - 1] != DIRSEP)
            path += DIRSEP;
        resources_paths.push_back(path);
        path_ele = path_ele->NextSiblingElement("path");
    }

    /* Push back absolute path */
    for (auto &path : resources_paths)
#ifdef WIN32 // if path is *:\ (C:\)
        if (path[1] == ':' && path[2] == '\'')
#else
        if (path[0] == DIRSEP || path[0] == '~')
#endif
            resources_paths_.push_back(path);
    /* COCO_PREFIX_PATH contains all the prefix for the specific platform divided by a : */
    const char* prefix = std::getenv("COCO_PREFIX_PATH");
    if (prefix)
    {
        for(auto p: coco::util::string_splitter(prefix,':'))
        {
            if(p.empty())
                continue;
            if(p.back() != DIRSEP)
                p += DIRSEP;
            /* Paths in COCO_PREFIX_PATH are considered not only prefix, but also complete paths */
            resources_paths_.push_back(p);
            /* Concatenate relative paths with PREFIX_PATH */
            for (auto &path : resources_paths)
            {
                 /* Checks wheter the path providied in the xml are absolute or relative.
					The decision is made wheter the first character is a / or the ~.
					This is clearly not compatible with Windows */
#ifdef WIN32 // if path is *:\ (C:\)
                if (path[1] != ':' || path[2] != '\'')
#else
                if (path[0] != DIRSEP && path[0] != '~')
#endif
                    resources_paths_.push_back(p + path);
            }

        }
    }
    app_spec_->resources_paths = resources_paths_;
}

void XmlParser::parseIncludes(tinyxml2::XMLElement *includes)
{
    if (!includes)
        return;

    using namespace tinyxml2;

    XMLElement *include = includes->FirstChildElement("include");
    while(include)
    {
        parseInclude(include);
        include = include->NextSiblingElement("include");
    }
}

void XmlParser::parseInclude(tinyxml2::XMLElement *include)
{
    using namespace tinyxml2;

    std::string file = include->GetText();
    std::string config_file = checkResource(file);
    if (config_file.empty())
    {
        COCO_ERR() << "Failed to find xml file: " << file;
        return;
    }
    XMLDocument xml_doc;
    XMLError error = xml_doc.LoadFile(config_file.c_str());
    if (error != XML_NO_ERROR)
    {
        COCO_ERR() << "Error while parsing XML include file: " << config_file << std::endl
                   << "Error " << error << std::endl;
        return;
    }

    XMLElement *package = xml_doc.FirstChildElement("package");
    if (package == 0)
    {
        COCO_ERR() << "Invalid include xml configuration file " << config_file
                  << ", doesn't start with the package block" << std::endl;
        return;
    }

    parsePaths(package->FirstChildElement("paths"));

    COCO_DEBUG("XmlParser") << "Include: Parsing includes";
    parseIncludes(package->FirstChildElement("includes"));

    COCO_DEBUG("XmlParser") << "Include: Parsing Components";
    parseComponents(package->FirstChildElement("components"), nullptr);

    COCO_DEBUG("XmlParser") << "Include: Parsing Connections";
    parseConnections(package->FirstChildElement("connections"));
    COCO_DEBUG("XmlParser") << "Include: Parsing Activities";
    parseActivities(package->FirstChildElement("activities"));
}

void XmlParser::parseComponents(tinyxml2::XMLElement *components,
                                TaskSpec * task_owner)
{
	using namespace tinyxml2;
    if (!components)
	{
		if (!task_owner)
			COCO_DEBUG("XmlParser") << "No components in this configuration file";

		return;
	}
	
	XMLElement *component = components->FirstChildElement("component");
	while(component)
	{
		parseComponent(component, task_owner);
		component = component->NextSiblingElement("component");
	}
}

void XmlParser::parseComponent(tinyxml2::XMLElement *component,
                               TaskSpec * task_owner)
{
	using namespace tinyxml2;

	if (!component)
        return;

    TaskSpec task_spec;
    task_spec.is_peer = task_owner != nullptr ? true : false;

    XMLElement *task = component->FirstChildElement("task");
    if (!task)
        COCO_FATAL() << "No <task> tag in component\n";

    const char* task_name = task->GetText();

    // Looking if it has been specified a name different from the task name
	XMLElement *name = component->FirstChildElement("name");
	std::string instance_name;
	if (name)
	{
		instance_name = name->GetText();
		if (instance_name.empty())
			instance_name = task_name;
	}
	else
    {
		instance_name = task_name;
    }
    task_spec.instance_name = instance_name;
    task_spec.name = task_name;

    COCO_DEBUG("XmlParser") << "Parsing " << (task_owner ? "peer" : "task") <<  ": " << task_name
    						<< " with istance name: " << instance_name;


	/* Looking for the library if it exists */
	const char* library_name = component->FirstChildElement("library")->GetText();
	/* Checking if the library is present in the path */
    std::string library = checkResource(library_name, true);

	if (library.empty())
		COCO_FATAL() << "Failed to find library with name: " << library_name;

	task_spec.library_name = library;	

	/* Parsing Attributes */
    COCO_DEBUG("XmlParser") << "Parsing attributes";
    XMLElement *attributes = component->FirstChildElement("attributes");
    parseAttribute(attributes, &task_spec);

	/* Parsing Peers */
    COCO_DEBUG("XmlParser") << "Parsing possible peers";    
    XMLElement *peers = component->FirstChildElement("components");
    parseComponents(peers, &task_spec);


    auto  task_ptr = std::make_shared<TaskSpec>(task_spec);
    if (task_owner)
        task_owner->peers.push_back(task_ptr);

    if (app_spec_->tasks.find(instance_name) != app_spec_->tasks.end())
        COCO_FATAL() << "A component with name: " << instance_name << " already exists";

    app_spec_->tasks.insert({instance_name, task_ptr});
}	

void XmlParser::parseAttribute(tinyxml2::XMLElement *attributes,
                               TaskSpec * task_spec)
{
    using namespace tinyxml2;
    
    if (!attributes)
        return;
    
    XMLElement *attribute  = attributes->FirstChildElement("attribute");
    while (attribute)
    {
        const std::string attr_name  = attribute->Attribute("name");
        std::string attr_value = attribute->Attribute("value");
        const char *attr_type  = attribute->Attribute("type");        
        if (attr_type)
        {
            std::string type = attr_type;
            if (type == "file" ||
                type == "File" ||
                type == "FILE")
            {
                std::string value = checkResource(attr_value);
                if (value.empty())
                {
                    COCO_ERR() << "Cannot find resource: " << attr_value
                               << " of attribute: " << attr_name;
                }
                attr_value = value;
            }
        }
        AttributeSpec attr_spec;
        attr_spec.name = attr_name;
        attr_spec.value = attr_value;
        task_spec->attributes.push_back(attr_spec);

        attribute = attribute->NextSiblingElement("attribute");
    }
}

std::string XmlParser::checkResource(const std::string &resource, bool is_library)
{
    std::string value = is_library ? DLLPREFIX + resource + DLLEXT : resource;

    std::ifstream stream;
    stream.open(value);
    if (stream.is_open())
    {
        return value;
    }
    else
    {
        for (auto & path : resources_paths_)
        {
            std::string tmp = path + value;
            stream.open(tmp);
            if (stream.is_open())
            {
                return tmp;
            }
        }
    }
    stream.close();
    return "";
}

void XmlParser::parseConnections(tinyxml2::XMLElement *connections)
{
    using namespace tinyxml2;

    if (connections)
    {
        XMLElement *connection  = connections->FirstChildElement("connection");
        while (connection)
        {
            parseConnection(connection);
            connection = connection->NextSiblingElement("connection");
        }   
    }
    else
    {
        COCO_DEBUG("XmlParser") << "No connection found";
    }
}

void XmlParser::parseConnection(tinyxml2::XMLElement *connection)
{
    using namespace tinyxml2;
    
    std::unique_ptr<ConnectionSpec> connection_spec(new ConnectionSpec);

    connection_spec->policy.data = connection->Attribute("data");
    connection_spec->policy.policy = connection->Attribute("policy");
    connection_spec->policy.transport = connection->Attribute("transport");
    connection_spec->policy.buffersize = connection->Attribute("buffersize");


    std::string src_task = connection->FirstChildElement("src")->Attribute("task");
    auto src = app_spec_->tasks.find(src_task);
    if (src != app_spec_->tasks.end())
        connection_spec->src_task = src->second;
    else
        COCO_FATAL() << "Source component with name: " << src_task << " doesn't exist";
    connection_spec->src_port = connection->FirstChildElement("src")->Attribute("port");
    

    std::string dest_task = connection->FirstChildElement("dest")->Attribute("task");
        auto dst = app_spec_->tasks.find(dest_task);
    if (dst != app_spec_->tasks.end())
        connection_spec->dest_task = dst->second;
    else
        COCO_FATAL() << "Destination component with name: " << dest_task << " doesn't exist";
    connection_spec->dest_port = connection->FirstChildElement("dest")->Attribute("port");

    COCO_DEBUG("XmlParser") << "Parsed connection: (" << src_task << " : " << connection_spec->src_port << ") ("
                            << dest_task << " : " << connection_spec->dest_port << ")";

    app_spec_->connections.push_back(std::move(connection_spec));
}

void XmlParser::parseActivities(tinyxml2::XMLElement *activities)
{
    using namespace tinyxml2;

    if (activities)
    {
        XMLElement *activity = activities->FirstChildElement("activity");
        while(activity)
        {
            parseActivity(activity);
            activity = activity->NextSiblingElement("activity");
        }

        XMLElement *pipeline = activities->FirstChildElement("pipeline");
        while(pipeline)
        {
            parsePipeline(pipeline);
            pipeline = pipeline->NextSiblingElement("pipeline");
        }

        XMLElement *farm = activities->FirstChildElement("farm");
        while(farm)
        {
            parseFarm(farm);
            farm = farm->NextSiblingElement("farm");
        }


    }
    else
    {
        // TODO decide how to hande in case of include
        COCO_FATAL() << "Missing Activities";
    }
}

void XmlParser::parseActivity(tinyxml2::XMLElement *activity)
{
    using namespace tinyxml2;
    std::unique_ptr<ActivitySpec> act_spec(new ActivitySpec);

    parseSchedule(activity->FirstChildElement("schedule"),
                  act_spec->policy, act_spec->is_parallel);

    XMLElement * components = activity->FirstChildElement("components");
    if (!components)
        COCO_FATAL() << "Activity missing components";
    
    XMLElement * component = components->FirstChildElement("component");
    while (component)
    {
        const char * task_name = component->Attribute("name");
        if (!task_name)
            COCO_FATAL() << "Specify a name for the component inside activity";

        auto task = app_spec_->tasks.find(task_name);
        if (task != app_spec_->tasks.end())
            act_spec->tasks.push_back(task->second);
        else
            COCO_FATAL() << "Failed to parse activity, task with name: " << task_name << " doesn't exist";

        component = component->NextSiblingElement("component");
    }

    app_spec_->activities.push_back(std::move(act_spec));
}

void XmlParser::parsePipeline(tinyxml2::XMLElement *pipeline)
{
    using namespace tinyxml2;
    std::unique_ptr<PipelineSpec> pipe_spec(new PipelineSpec);

    XMLElement *schedule = pipeline->FirstChildElement("schedule");
    if (!schedule)
        COCO_FATAL() << "Tag pipeline doesn't have <schedule> tag";

    std::string type = schedule->Attribute("activity");
    if (type != "parallel" && type !="sequential")
        COCO_FATAL() << "Tag pipeline support only attributes: parallel, sequential";
    pipe_spec->parallel = (type == "parallel");

    XMLElement *components = pipeline->FirstChildElement("components");
    if (!components)
        COCO_FATAL() << "Tag pipeline must include <components>";
    XMLElement *component = components->FirstChildElement("component");
    if (!component)
        COCO_FATAL() << "Tag pipeline must have at least one <component>";
    unsigned comp_count = 0;
    while(component)
    {
        std::string name = component->Attribute("name");
        auto task = app_spec_->tasks.find(name);
        if (task == app_spec_->tasks.end())
            COCO_FATAL() << "Component " << name << " in pipeline tag doesn't exist";
        pipe_spec->tasks.push_back(task->second);

        auto in = component->Attribute("in");
        if (in)
            pipe_spec->in_ports.push_back(in);
        else
            COCO_FATAL() << "For component " << name << " in pipeline tag, specify the input port";

        auto out = component->Attribute("out");
        if (out)
            pipe_spec->out_ports.push_back(out);
        else
            COCO_FATAL() << "For component " << name << " in pipeline tag, specify the output port";

        component = component->NextSiblingElement("component");
        ++comp_count;
    }

    if (comp_count < 2)
        COCO_FATAL() << "Pipeline tag must have at least 2 components";

    app_spec_->addPipelineConnections(pipe_spec);

    app_spec_->pipelines.push_back(std::move(pipe_spec));
}

void XmlParser::parseFarm(tinyxml2::XMLElement *farm)
{
    using namespace tinyxml2;
    COCO_DEBUG("XmlParser") << "Parsing a Farm";
    std::unique_ptr<FarmSpec> farm_spec(new FarmSpec);
    // parse schedule
    auto schedule = farm->FirstChildElement("schedule");
    if (!schedule)
        COCO_FATAL() << "Farm tag must have a schedule tag, where to specify the number of workers";
    auto workers = schedule->Attribute("workers");
    if (!workers)
        COCO_FATAL() << "Schedule tag in Farm tag must have workers attribute "
                     << "where the number of workers is specifyed";
    farm_spec->num_workers = static_cast<unsigned int>(std::atoi(workers));


    // Parse source
//    <source>
//        <schedule type="periodic" value="500" exclusive_affinity="3" />
//        <component name="" out="" />
//    </source>
    auto source = farm->FirstChildElement("source");
    if (!source)
        COCO_FATAL() << "Farm tag must have a source tag";
    schedule = source->FirstChildElement("schedule");
    if (!schedule)
        COCO_FATAL() << "Source tag in Farm tag must have a schedule tag";

    bool is_parallel;
    parseSchedule(schedule, farm_spec->source_task_schedule, is_parallel);

    auto component = source->FirstChildElement("component");
    if (!component)
        COCO_FATAL() << "Source tag in Farm tag must have a component tag";

    auto name = component->Attribute("name");
    if (!name)
        COCO_FATAL() << "Component tag in Source tag in Farm tag must have a name attribute";
    auto source_task = app_spec_->tasks.find(name);
    if (source_task == app_spec_->tasks.end())
        COCO_FATAL() << "Component " << name << " as Source in Farm tag doesn't exist";
    farm_spec->source_task = source_task->second;

    auto out_port = component->Attribute("out");
    if (!out_port)
        COCO_FATAL() << "Component " << name << " in Source in Farm tag doesn't have a out attribute";
    farm_spec->source_port = out_port;

    // Parse pipeline
    auto pipeline = farm->FirstChildElement("pipeline");
    if (!farm)
        COCO_FATAL() << "Farm tag must have a pipeline tag inside";
    parsePipeline(pipeline);
    farm_spec->pipelines.push_back(std::move(app_spec_->pipelines.back()));
    app_spec_->pipelines.pop_back();

    // Parse gather
    auto gather = farm->FirstChildElement("gather");
    if (!gather)
        COCO_FATAL() << "Farm tag must have a Gather tag";
    component = gather->FirstChildElement("component");
    if (!component)
        COCO_FATAL() << "Gather tag in Farm tag must have a component tag";
    name = component->Attribute("name");
    if (!name)
        COCO_FATAL() << "Component tag in Gather tag in Farm tag must have a name attribute";
    auto gather_task = app_spec_->tasks.find(name);
    if (gather_task == app_spec_->tasks.end())
        COCO_FATAL() << "Component " << name << " as Gather in Farm tag doesn't exist";
    farm_spec->gather_task = gather_task->second;

    auto in_port = component->Attribute("in");
    if (!in_port)
        COCO_FATAL() << "Component " << name << " in Gather in Farm tag doesn't have a out attribute";
    farm_spec->gather_port = in_port;

    app_spec_->farms.push_back(std::move(farm_spec));
}

void XmlParser::parseSchedule(tinyxml2::XMLElement *schedule_policy,
                              SchedulePolicySpec &policy, bool &is_parallel)
{
    using namespace tinyxml2;
    if (!schedule_policy)
        COCO_FATAL() << "No schedule policy found Activity";


    const char *activity = schedule_policy->Attribute("activity");
    if (!activity)
        activity = "parallel";

    if (strcmp(activity, "parallel") == 0 ||
        strcmp(activity, "Parallel") == 0 ||
        strcmp(activity, "PARALLEL") == 0)
    {
        is_parallel = true;
    }
    else if (strcmp(activity, "sequential") == 0 ||
             strcmp(activity, "Sequential") == 0 ||
             strcmp(activity, "SEQUENTIAL") == 0)
    {
        is_parallel = false;
    }
    else
    {
        COCO_FATAL() << "Schduele policy: " << activity << " is not know\n" <<
                        "Possibilities are: parallel, sequential";
    }
    
    const char *activation_type = schedule_policy->Attribute("type");
    const char *value = schedule_policy->Attribute("value");

    if (strcmp(activation_type, "triggered") == 0 ||
        strcmp(activation_type, "Triggered") == 0 ||
        strcmp(activation_type, "TRIGGERED") == 0)
    {
        policy.type = "triggered";
    }
    else if (strcmp(activation_type, "periodic") == 0 ||
             strcmp(activation_type, "Periodic") == 0 ||
             strcmp(activation_type, "PERIODIC") == 0)
    {
        if (!value)
            COCO_FATAL() << "Activity scheduled as periodic but no period provided";
        else
            policy.period = std::atoi(value);
        policy.type = "periodic";
    }
    else
    {
        COCO_FATAL() << "Schduele policy type: " << activation_type << " is not know\n" <<
                        "Possibilities are: triggered, periodic";
    }
    
    policy.affinity = -1;
    policy.exclusive = false;
    const char *affinity = schedule_policy->Attribute("affinity");
    if (affinity)
    {
        policy.affinity = std::atoi(affinity);
        policy.exclusive = false;
    }
    else
    {
        const char *exclusive_affinity =
            schedule_policy->Attribute("exclusive_affinity");
        if (exclusive_affinity)
        {
            policy.affinity = std::atoi(exclusive_affinity);
            policy.exclusive = true;
        }
    }
}

}
