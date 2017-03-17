/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
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
			   			  std::shared_ptr<TaskGraphSpec> app_spec, bool first)
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
    if(first)
    {
        const char* name = package->Attribute("name");
        app_spec_->name = name ? name : "<not defined>";

        parseLogConfig(package->FirstChildElement("log"));
    }

    parsePaths(package->FirstChildElement("paths"));

    COCO_DEBUG("XmlParser") << "Parsing includes";
    parseIncludes(package->FirstChildElement("includes"));

    COCO_DEBUG("XmlParser") << "Parsing Components";
    parseComponents(package->FirstChildElement("components"), nullptr);
	
	COCO_DEBUG("XmlParser") << "Parsing Connections";
    parseConnections(package->FirstChildElement("connections"));

	COCO_DEBUG("XmlParser") << "Parsing Activities";
    parseActivities(package->FirstChildElement("activities"));

    // as long as parse is reused the paths are not overwritten
    app_spec_->resources_paths = resources_paths_;

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
                if (type == "ERR" || type == "err" || type == "error")
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

static bool isAbsolutePath(std::string x)
{
#ifdef _WIN32
    return x.size() >= 3 && ((x[2] == '\\' && x[1] == ':') || (x[0]=='\\' && x[1]=='\\')); // X:\ or \\...
#else
    return x.size() >= 1 && (x[0] == '/' || x[0] == '~');
#endif
}

void XmlParser::parsePaths(tinyxml2::XMLElement *paths)
{
	using namespace tinyxml2;

    if (!paths)
    {
        return;
    }

    /* Collect the path in an tmp vector, they will be checked to see if they are relative or global */
    XMLElement *path_ele = paths->FirstChildElement("path");
    std::vector<std::string> relresources_paths;
    while (path_ele)
    {
        std::string path = path_ele->GetText();
        if (path[path.size() - 1] != DIRSEP)
            path += DIRSEP;
        if(isAbsolutePath(path))
        {
            resources_paths_.push_back(path);            
        }
        else
        {
            relresources_paths.push_back(path);
        }
        path_ele = path_ele->NextSiblingElement("path");
    }

    /* COCO_PREFIX_PATH contains all the prefix for the specific platform divided by a : or ; depending on system */
    const char* prefix = std::getenv("COCO_PREFIX_PATH");
    if (prefix)
    {
        for(auto p: coco::util::string_splitter(prefix,PATHSEP))
        {
            if(p.empty())
            {
                continue;
            }
            // append terminator
            if(p.back() != DIRSEP)
            {
                p += DIRSEP;
            }
            /* Paths in COCO_PREFIX_PATH are considered not only prefix, but also complete paths */
            resources_paths_.push_back(p);
            /* Concatenate relative paths with PREFIX_PATH */
            for (auto &path : relresources_paths)
            {
                resources_paths_.push_back(p + path);
            }
        }
    }
    for (auto &path : resources_paths_)
    {
        std::cout << "RP: " << path << std::endl;
    }
}

void XmlParser::parseIncludes(tinyxml2::XMLElement *includes)
{
    if (!includes)
        return;

    using namespace tinyxml2;

    for(; includes; includes = includes->NextSiblingElement("include"))
    {    
        XMLElement *include = includes->FirstChildElement("include");
        while(include)
        {
            parseInclude(include);
            include = include->NextSiblingElement("include");
        }
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
                                TaskSpec * task_ownerx)
{
	using namespace tinyxml2;
    for(;components; components = components->NextSiblingElement("components"))
    {
        TaskSpec * task_owner = task_ownerx;

        // new component lookup
        auto * p = components->Attribute("extends",0);
        if(p)
        {
            if(task_owner)
            {
                COCO_FATAL() << "Cannot extend components inside the component declaration";
                return;
            }
            else
            {
                auto it = app_spec_->tasks.find(p);
                if(it != app_spec_->tasks.end())
                {
                    task_owner = it->second.get();
                }
                else
                {
                    COCO_FATAL() << "Cannot find the requested instance" << p;
                    return;
                }
            }
        }

    	for(XMLElement *component = components->FirstChildElement("component"); component; component = component->NextSiblingElement("component"))
    	{
            auto * extendsname  = component->Attribute("extends",0);
            if(extendsname)
            {
                parseExtendComponent(extendsname,component,task_owner);
            }
            else
            {
        		parseComponent(component, task_owner);		
            }
    	}
    }
}


void XmlParser::parseExtendComponent(std::string name, tinyxml2::XMLElement *component,
                               TaskSpec * task_owner)
{
    auto it = app_spec_->tasks.find(name);
    if(it == app_spec_->tasks.end())
    {
        COCO_FATAL() << "Not found instance " << name;
        return;
    }
    auto & task_spec = it->second;

    auto attributes = component->FirstChildElement("attributes");
    if(attributes)  
        parseAttribute(attributes, task_spec.get());

    parseContents(component, task_spec.get());

    parseComponents(component->FirstChildElement("components"), task_owner);

}

void XmlParser::parseComponent(tinyxml2::XMLElement *component,
                               TaskSpec * task_owner)
{
	using namespace tinyxml2;

    TaskSpec task_spec;
    task_spec.is_peer = task_owner != nullptr ? true : false;

    XMLElement *task = component->FirstChildElement("task");
    if (!task)
    {
        COCO_FATAL() << "No <task> tag in component\n";
    }

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
    						<< " with instance name: " << instance_name;


	/* Looking for the library if it exists */
    auto * p = component->FirstChildElement("library");
	const char* library_name = p ? p->GetText() : "";

    if(library_name[0] != 0)
    {
    	/* Checking if the library is present in the path */
        std::string library = checkResource(library_name, true);

    	if (library.empty())
        {
    		COCO_FATAL() << "Failed to find library with name: " << library_name;
        }

    	task_spec.library_name = library;	
    }

	/* Parsing Attributes */
    COCO_DEBUG("XmlParser") << "Parsing attributes";
    XMLElement *attributes = component->FirstChildElement("attributes");
    if(attributes)  
        parseAttribute(attributes, &task_spec);

    parseContents(component, &task_spec);

	/* Parsing Peers */
    auto xp = component->FirstChildElement("components");
    if(xp)
    {
        COCO_DEBUG("XmlParser") << "Parsing peers";    
        parseComponents(xp, &task_spec);
    }



    auto  task_ptr = std::make_shared<TaskSpec>(task_spec);
    if (task_owner)
        task_owner->peers.push_back(task_ptr);

    if (app_spec_->tasks.find(instance_name) != app_spec_->tasks.end())
        COCO_FATAL() << "A component with name: " << instance_name << " already exists";

    app_spec_->tasks.insert({instance_name, task_ptr});
}	

static void CopyNode(tinyxml2::XMLNode *p_dest_parent, const tinyxml2::XMLNode *p_src, tinyxml2::XMLDocument * p_doc, bool childrenonly)
{
    // Protect from evil
    if (p_src == NULL || p_doc == NULL)
    {
        return;
    }

    tinyxml2::XMLNode *p_copy = 0;

    // real clone vs only children
    if(!childrenonly)
    {
        // Make the copy
        p_copy = p_src->ShallowClone(p_doc);
        if (p_copy == NULL)
        {
            // Error handling required (e.g. throw)
            return;
        }

        if(!p_dest_parent)
        {
            p_doc->InsertFirstChild(p_copy);
        }
        else
        {
            p_dest_parent->InsertEndChild(p_copy);
        }
    }
    else
    {
        if(!p_dest_parent)
        {
            return;
        }
        p_copy = p_dest_parent;
    }

    // Add the grandkids
    for (const tinyxml2::XMLNode *p_node = p_src->FirstChild(); p_node != NULL; p_node = p_node->NextSibling())
    {
        CopyNode(p_copy, p_node,p_doc,false);
    }
}

void XmlParser::parseContents(tinyxml2::XMLElement *parent,
                               TaskSpec * task_spec)
{
    using namespace tinyxml2;
    
    for (XMLElement *cc  = parent->FirstChildElement("content"); cc; cc = cc->NextSiblingElement("content"))
    {
        const std::string cc_name  = cc->Attribute("name");
        std::string type  = cc->Attribute("type");
        if(type.empty()) type = "xml";

        if(type == "xml")
        {
            /*
            Clone Based
            */
            XMLDocument doc;
            CopyNode(0,cc,&doc,false);

            XMLPrinter printer;
            doc.Accept( &printer );
            

            // visiting based, hackish 
            /*
            XMLPrinter printer;
            printer.VisitEnter(*cc,cc->FirstAttribute());
            printer.VisitExit(*cc);
            std::cerr << "LOADED " << printer.CStr() << std::endl;
            */
            task_spec->contents[cc_name] = printer.CStr();
        }
        else
        {
            task_spec->contents[cc_name] = cc->GetText();            
        }

    }
}


void XmlParser::parseAttribute(tinyxml2::XMLElement *attributes,
                               TaskSpec * task_spec)
{
    using namespace tinyxml2;
    
    for (XMLElement *attribute  = attributes->FirstChildElement("attribute"); attribute; attribute = attribute->NextSiblingElement("attribute"))
    {
        const std::string attr_name  = attribute->Attribute("name");
        std::string attr_value = attribute->Attribute("value");
        const char *attr_type  = attribute->Attribute("type");        
        if (attr_type)
        {
            std::string type = attr_type;
            std::transform(type.begin(), type.end(), type.begin(), ::tolower);
            if (type == "file")
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
    }
}

std::string XmlParser::checkResource(const std::string &resource, bool is_library)
{
    std::string value = is_library ? DLLPREFIX + resource + DLLEXT : resource;
    if(is_library)
    {
        std::cout << "Library lookup " << value << std::endl;
    }
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

    for(;connections; connections = connections->NextSiblingElement("connections"))
    {
        for (XMLElement * connection = connections->FirstChildElement("connection"); connection; connection = connection->NextSiblingElement("connection"))
        {
            parseConnection(connection);
        }   
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

    for (; activities ; activities = activities->NextSiblingElement("activities"))
    {
        for(XMLElement *activity = activities->FirstChildElement("activity"); activity; activity = activity->NextSiblingElement("activity"))
        {
            parseActivity(activity);            
        }

        for(XMLElement *pipeline = activities->FirstChildElement("pipeline"); pipeline; pipeline = pipeline->NextSiblingElement("pipeline"))
        {
            parsePipeline(pipeline);            
        }

        for(XMLElement *farm = activities ->FirstChildElement("farm"); farm; farm = farm->NextSiblingElement("farm"))
        {
            parseFarm(farm);
            farm = farm->NextSiblingElement("farm");
        }
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

/*
 * How the schedule works:
 * Always mandatory field: activity, type
 * If type == periodic -> period
 * If realtime == FIFO || RR -> priority
 * If realtime == DEADLINE -> runtime && type == periodic
 * affinity and exclusive_affinity are always optional and correct
 */
void XmlParser::parseSchedule(tinyxml2::XMLElement *schedule_policy,
                              SchedulePolicySpec &policy, bool &is_parallel)
{
    using namespace tinyxml2;
    for(; schedule_policy; schedule_policy = schedule_policy->NextSiblingElement("activities"))
    {
        const char *activity = schedule_policy->Attribute("activity");
        const char *activation_type = schedule_policy->Attribute("type");
        const char *value = schedule_policy->Attribute("period");
        const char *realtime= schedule_policy->Attribute("realtime");
        const char *priority = schedule_policy->Attribute("priority");
        const char *runtime = schedule_policy->Attribute("runtime");
        const char *affinity = schedule_policy->Attribute("affinity");
        const char *exclusive_affinity =  schedule_policy->Attribute("exclusive_affinity");

        if (!activity)
        {
            activity = "parallel";
        }

        if (strcasecmp(activity, "parallel") == 0)
        {
            is_parallel = true;
        }
        else if (strcasecmp(activity, "sequential") == 0)
        {
            is_parallel = false;
        }
        else
        {
            COCO_FATAL() << "Schedule policy: " << activity << " is not know\n" <<
                            "Possibilities are: parallel, sequential";
        }

        if (strcasecmp(activation_type, "triggered") == 0)
        {
            policy.type = "triggered";
        }
        else if (strcasecmp(activation_type, "periodic") == 0)
        {
            if (!value)
            {
                COCO_FATAL() << "Activity scheduled as periodic but no period provided";
            }
            else
            {
                policy.period = std::atoi(value);
            }
            policy.type = "periodic";
        }
        else
        {
            COCO_FATAL() << "Schduele policy type: " << activation_type << " is not know\n" <<
                            "Possibilities are: triggered, periodic";
        }

        if (realtime)
        {
            if (strcasecmp(realtime, "FIFO") == 0)
            {
                policy.realtime = "fifo";
            }
            else if (strcasecmp(realtime, "RR") == 0)
            {
                policy.realtime = "rr";
            }
            else if (strcasecmp(realtime, "DEADLINE") == 0)
            {
                if (policy.type == "triggered")
                {
                    COCO_FATAL() << "Triggered activity cannot be realtime DEADLINE."
                                 << " If you want to use realtime, use FIFO or RR";
                }
                policy.realtime = "deadline";
                policy.runtime = 0;
            }
            else
            {
                COCO_ERR() << "Invalid type " << realtime
                           << " for attribute realitime in tag schedule."
                           << " Real time set to NONE";
                policy.realtime = "none";
            }
        }
        else
        {
            policy.realtime = "none";
        }

        if (priority)
        {
            if (policy.realtime != "none" && policy.realtime != "deadline")
            {
                policy.priority = std::atoi(priority);
            }
            else
            {
                COCO_FATAL() << "Cannot set a priority to an activity that is not realtime FIFO or RR";
            }
        }
        else
        {
            if (policy.realtime == "fifo" || policy.realtime == "rr")
            {
                COCO_DEBUG("XmlParser") << "Activity set as realtime but no priorit set. Priority automatically set at 1";
                policy.priority = 1;
            }
        }

        if (runtime)
        {
            if (policy.realtime == "deadline")
            {
                policy.runtime = std::atoi(runtime);
            }
            else
            {
                COCO_FATAL() << "Cannot set a runtime value to an activity that is not DEADLINE realtime";
            }
        }

        if (policy.realtime == "deadline" && policy.runtime == 0)
        {
            COCO_FATAL() << "Realtime DEADLINE needs attribute runtime to be specified";
        }

        policy.affinity = -1;
        policy.exclusive = false;
        if (affinity)
        {
            policy.affinity = std::atoi(affinity);
            policy.exclusive = false;
        }
        else if (exclusive_affinity)
        {
                policy.affinity = std::atoi(exclusive_affinity);
                policy.exclusive = true;
        }
    }
}



tinyxml2::XMLElement* XmlParser::xmlNodeTxt(tinyxml2::XMLElement * parent,
                                            const std::string &tag,
                                            const std::string text)
{
    using namespace tinyxml2;
    XMLElement *xml_task = xml_doc_.NewElement(tag.c_str());
    XMLText *task_text = xml_doc_.NewText(text.c_str());
    xml_task->InsertEndChild(task_text);
    parent->InsertEndChild(xml_task);
    return xml_task;
}

bool XmlParser::createXML(const std::string &xml_file,
                          std::shared_ptr<TaskGraphSpec> app_spec)
{
    app_spec_ = app_spec;
    //xml_doc_ = tinyxml2::XMLDocument();

    auto package = xml_doc_.NewElement("package");
    xml_doc_.InsertEndChild(package);

    // TODO should I need to insert log and paths?

    auto components = xml_doc_.NewElement("components");
    package->InsertEndChild(components);

    for (auto &task : util::values_iteration(app_spec_->tasks))
    {
        createComponent(task, components);
    }

    auto connections = xml_doc_.NewElement("connectitons");
    package->InsertEndChild(connections);

    for (auto &connection : app_spec_->connections)
    {
        createConnection(connection, connections);
    }

    auto result = xml_doc_.SaveFile(xml_file.c_str());
    
    return result == tinyxml2::XML_NO_ERROR;
}

void XmlParser::createComponent(std::shared_ptr<TaskSpec> task_spec,
                                tinyxml2::XMLElement *components)
{
    auto component = xml_doc_.NewElement("component");
    components->InsertEndChild(component);

    xmlNodeTxt(component, "task", task_spec->name);
    xmlNodeTxt(component, "name", task_spec->instance_name);
    xmlNodeTxt(component, "library", task_spec->library_name);

    auto attributes = xml_doc_.NewElement("attributes");
    component->InsertEndChild(attributes);

    for (auto &attribute_spec : task_spec->attributes)
    {
        auto attribute = xml_doc_.NewElement("attribute");
        attribute->SetAttribute("name", attribute_spec.name.c_str());
        attribute->SetAttribute("value",attribute_spec.value.c_str());
        attributes->InsertEndChild(attribute);
    }

    auto peers = xml_doc_.NewElement("components");
    component->InsertEndChild(peers);

    for (auto &peer : task_spec->peers)
    {
        createComponent(peer, peers);
    }
}

void XmlParser::createConnection(std::unique_ptr<ConnectionSpec> &connection_spec,
                                 tinyxml2::XMLElement *connections)
{
    auto connection = xml_doc_.NewElement("connection");
    connections->InsertEndChild(connection);

    connection->SetAttribute("data", connection_spec->policy.data.c_str());
    connection->SetAttribute("policy", connection_spec->policy.policy.c_str());
    connection->SetAttribute("transport", connection_spec->policy.transport.c_str());
    connection->SetAttribute("buffersize", connection_spec->policy.buffersize.c_str());

    auto src = xml_doc_.NewElement("src");
    connection->InsertEndChild(src);
    src->SetAttribute("task", (connection_spec->src_task ? connection_spec->src_task->instance_name.c_str() : ""));
    src->SetAttribute("port", connection_spec->src_port.c_str());
    auto dest = xml_doc_.NewElement("dest");
    connection->InsertEndChild(dest);
    dest->SetAttribute("task", (connection_spec->dest_task ? connection_spec->dest_task->instance_name.c_str() : ""));
    dest->SetAttribute("port", connection_spec->dest_port.c_str());
}

}
