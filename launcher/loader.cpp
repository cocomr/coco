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

#include "loader.h"
#include <cstring>
#include <sstream>
#include <locale>

namespace coco
{

CocoLauncher::CocoLauncher(const std::string &config_file)
    : config_file_(config_file)
{}

bool CocoLauncher::createApp(bool profiling)
{
    using namespace tinyxml2;
    XMLError error = doc_.LoadFile(config_file_.c_str());
    if (error != XML_NO_ERROR)
    {
        COCO_FATAL() << "Error: " << error << std::endl <<
                     "While loading XML file: " << config_file_;
    }

    ComponentRegistry::enableProfiling(profiling);

    parseFile(doc_,true);

    // Removing the peers from the task list
    for (auto it : peers_)
    {
        auto t = tasks_.find(it);
        if (t != tasks_.end())
          tasks_.erase(t);
    }


    return true;
}

bool CocoLauncher::parseFile(tinyxml2::XMLDocument & doc, bool top)
{
    using namespace tinyxml2;
    XMLElement *package = doc.FirstChildElement("package");
    if (package == 0)
    {
        std::cout << "Invalid xml configuration file " << config_file_ << std::endl;
        exit(-1);
    }

    // TBD: only one log specification for ALL execution
    // Option1: use top
    // Option2: collect value and use latest
    // Option3: ignore sub
    parseLogConfig(package->FirstChildElement("logconfig"));
    // TBD: only libraries_path_ ONCE
    parsePaths(package->FirstChildElement("resourcespaths"));

    COCO_DEBUG("Loader") << "Parsing includes";
    XMLElement *includes = package->FirstChildElement("includes");
    if (includes)
    {
        XMLElement *include = includes->FirstChildElement("include");
        while(include)
        {
            parseInclude(include);
            include = include->NextSiblingElement("include");
        }
    }
    else
    {
        COCO_DEBUG("Loader") << "No includes found.";
    }

    //COCO_DEBUG("Loader") << "Parsing Activities";
    COCO_DEBUG("Loader") << "Parsing Activities";
    XMLElement *activities = package->FirstChildElement("activities");
    if (activities)
    {
        XMLElement *activity = activities->FirstChildElement("activity");
        while(activity)
        {
            parseActivity(activity);
            activity = activity->NextSiblingElement("activity");
        }
    }
    else
    {
        COCO_FATAL() << "Missing Activities";
    }

    COCO_DEBUG("Loader") << "Parsing connections";
    XMLElement *connections = doc_.FirstChildElement("package")->FirstChildElement("connections");
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
        COCO_DEBUG("Loader") << "No connections found.";
    }
    return true;
}

void CocoLauncher::parseLogConfig(tinyxml2::XMLElement *logconfig)
{
    using namespace tinyxml2;
    if (!logconfig)
    {
        COCO_INIT_LOG()
        COCO_LOG_INFO()
        return;
    }
    
    const char *log_config_file = logconfig->GetText();
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
        std::string levels = levels_ele->GetText();
        std::stringstream ss_levels(levels);
        std::string level;
        char delim = ' ';
        while (std::getline(ss_levels, level, delim))
        {
            levels_set.insert((int)std::stoi(level.c_str()));
        }   
        coco::util::LoggerManager::getInstance()->setLevels(levels_set);
    }

    XMLElement *type_ele = logconfig->FirstChildElement("types");
    if (type_ele)
    {
        std::set<coco::util::Type> types_set;
        std::string types = type_ele->GetText();
        std::stringstream ss_types(types);
        std::string type;
        char delim = ' ';
        while (std::getline(ss_types, type, delim))
        {
            while (auto it = type.find(' ') != -1)
                type.erase(it);
            if (type == "DEBUG" || type == "debug")
                types_set.insert(coco::util::DEBUG);
            if (type == "ERR" || type == "err")
                types_set.insert(coco::util::ERR);
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

void CocoLauncher::parsePaths(tinyxml2::XMLElement *paths)
{
    using namespace tinyxml2;

    if (!paths)
        return;

    XMLElement *libraries_path = paths->FirstChildElement("librariespath");
    if (libraries_path)
        libraries_path_ = libraries_path->GetText();

    XMLElement *path = paths->FirstChildElement("path");
    while (path)
    {
        resources_paths_.push_back(path->GetText());
        path  = path->NextSiblingElement("path");
    }
}

void CocoLauncher::parseInclude(tinyxml2::XMLElement *include)
{
    if (!include)
        return;
    const char *path = include->Attribute("path");
    if(path)
    {
        tinyxml2::XMLDocument doc;
        tinyxml2::XMLError error = doc.LoadFile(path);
        if (error != tinyxml2::XML_NO_ERROR)
        {
            COCO_FATAL() << "Error: " << error << std::endl
                         << "While loading XML sub file: " << path;;
        }
        parseFile(doc, true);
    }
}

void CocoLauncher::parseActivity(tinyxml2::XMLElement *activity)
{
    using namespace tinyxml2;
    COCO_DEBUG("Loader") << "Parsing schedule policy";
    XMLElement *schedule_policy = activity->FirstChildElement("schedulepolicy");
    SchedulePolicy policy;
    bool is_parallel;
    if (schedule_policy)
        parseSchedule(schedule_policy, policy, is_parallel);
    else
        COCO_FATAL() << "No schedule policy found for activity";

    Activity *act = nullptr;
    if (is_parallel)
        act = new ParallelActivity(policy);
    else
        act = new SequentialActivity(policy);
    activities_.push_back(act);
    XMLElement *components = activity->FirstChildElement("components");
    if (components)
    {
        XMLElement *component = components->FirstChildElement("component");
        while (component)
        {
            parseComponent(component, act);
            component = component->NextSiblingElement("component");
        }
    }
    else
    {
        COCO_FATAL() << "Missing <components> tag\n";
    }
}

void CocoLauncher::parseSchedule(tinyxml2::XMLElement *schedule_policy, SchedulePolicy &policy, bool &is_parallel)
{
    using namespace tinyxml2;
    if (!schedule_policy)
        COCO_FATAL() << "No schedule policy found Activity";

    const char *activity = schedule_policy->Attribute("activity");
    if (!activity)
        COCO_FATAL() << "No activity attribute found in schedule policy specification";
    const char *activation_type = schedule_policy->Attribute("type");
    if (!activation_type)
        COCO_FATAL() << "No type attribute found in schedule policy specification";
    const char *value = schedule_policy->Attribute("value");

    if (strcmp(activation_type, "triggered") == 0 ||
        strcmp(activation_type, "Triggered") == 0 ||
        strcmp(activation_type, "TRIGGERED") == 0)
    {
        policy = SchedulePolicy(SchedulePolicy::TRIGGERED);
    }
    else if (strcmp(activation_type, "periodic") == 0 ||
             strcmp(activation_type, "Periodic") == 0 ||
             strcmp(activation_type, "PERIODIC") == 0)
    {
        if (!value)
            COCO_FATAL() << "Activity scheduled as periodic but no period provided";

        policy = SchedulePolicy(SchedulePolicy::PERIODIC, atoi(value));
    }
    else
    {
        COCO_FATAL() << "Schduele policy type: " << activation_type << " is not know\n" <<
                        "Possibilities are: triggered, periodic";
    }

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
}

void CocoLauncher::parseComponent(tinyxml2::XMLElement *component, Activity *activity, bool is_peer)
{
	using namespace tinyxml2;

    if (!component)
        return;

    XMLElement *task = component->FirstChildElement("task");
    if (!task)
        COCO_FATAL() << "No <task> tag in component\n";
    const char* task_name = task->GetText();

    // Looking if it has been specified a name different from the task name
	XMLElement *name = component->FirstChildElement("name");
	std::string component_name;
	if (name)
	{
		component_name = name->GetText();
		if (component_name.empty())
			component_name = task_name;
	}
	else
    {
		component_name = task_name;
    }
	COCO_DEBUG("Loader") << "Creating component: " << task_name << 
				   " with name: " << component_name;

    TaskContext * t = ComponentRegistry::create(task_name, component_name);
    if (!t)
    {
        COCO_DEBUG("Loader") << "Component " << task_name <<
                       " not found, trying to load from library";
        const char* library_name = component->
                FirstChildElement("library")->GetText();
        XMLElement *librarypath = component->FirstChildElement("librarypath");
        if (!ComponentRegistry::addLibrary(library_name,
                                           !librarypath ?
                                           libraries_path_ : librarypath->GetText()))
        {
            COCO_FATAL() << "Failed to load library: " << library_name;
            return;
        }
        t = ComponentRegistry::create(task_name, component_name);
        if (!t)
        {
            COCO_FATAL() << "Failed to create component: " << task_name;
            return;
        }
    }
    if (!is_peer)
    {
        t->setEngine(std::make_shared<ExecutionEngine>(t, 
                ComponentRegistry::profilingEnabled()));
        activity->addRunnable(t->engine());
        t->setActivity(activity);
    }

    //tasks_[component_name] = std::shared_ptr<LComponentBase>(new LRealComponent(t));
    t->setInstantiationName(component_name);
    tasks_[component_name] = t;

    COCO_DEBUG("Loader") << "Parsing attributes";
    XMLElement *attributes = component->FirstChildElement("attributes");
    parseAttribute(attributes, t);
    
    // Parsing possible peers
    COCO_DEBUG("Loader") << "Parsing possible peers";
    XMLElement *peers = component->FirstChildElement("components");
    parsePeers(peers, t);

    // TBD: better do that at the very end of loading process
    t->init();
}


void CocoLauncher::parseAttribute(tinyxml2::XMLElement *attributes, TaskContext *t)
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
                               << " of attribute: " << attr_name
                               << " of task " << t->name();
                }
                attr_value = value;
            }
        }
        if (t->getAttribute(attr_name))
            t->getAttribute(attr_name)->setValue(attr_value);
        else
            COCO_ERR() << "Attribute: " << attr_name << " doesn't exist";
        attribute = attribute->NextSiblingElement("attribute");
    }
}

std::string CocoLauncher::checkResource(const std::string &value)
{
    std::ifstream stream;
    stream.open(value);
    if (stream.is_open())
    {
        return value;
    }
    else
    {
        for (auto &rp : resources_paths_)
        {
            if (rp.back() != '/')
                rp += std::string("/");
            std::string tmp = rp + value;
            stream.open(tmp);
            if (stream.is_open())
            {
                return tmp;
            }
        }
    }
    return "";
}

void CocoLauncher::parsePeers(tinyxml2::XMLElement *peers, TaskContext *t)
{
    using namespace tinyxml2;
    if (peers)
    {
        XMLElement *peer = peers->FirstChildElement("component");
        while (peer)
        {
            COCO_DEBUG("Loader") << "Parsing peer";
            parseComponent(peer, 0, true);
            std::string peer_component = peer->FirstChildElement("task")->GetText();
            XMLElement *name_element = peer->FirstChildElement("name");
            std::string peer_name;
            if (name_element)
            {
                peer_name = name_element->GetText();
                if (peer_name.empty())
                    peer_name = peer_component;
            }
            else
                peer_name = peer_component;

            TaskContext *peer_task = tasks_[peer_name];
            if (peer_task)
                t->addPeer(peer_task);
            peers_.push_back(peer_name);
            peer = peer->NextSiblingElement("component");
        }
    }
}

void CocoLauncher::parseConnection(tinyxml2::XMLElement *connection)
{
    using namespace tinyxml2;
	ConnectionPolicy policy(connection->Attribute("data"),
							connection->Attribute("policy"),
							connection->Attribute("transport"),
							connection->Attribute("buffersize"));
	const std::string &task_out = connection->FirstChildElement("src")->Attribute("task");
	std::string task_out_port = connection->FirstChildElement("src")->Attribute("port");
    
    if (!tasks_[task_out])
        COCO_FATAL() << "Task with name: " << task_out << " doesn't exist.";
    	
    const std::string &task_in = connection->FirstChildElement("dest")->Attribute("task");
	std::string task_in_port  = connection->FirstChildElement("dest")->Attribute("port");
    
    if (!tasks_[task_in])
        COCO_FATAL() << "Task with name: " << task_in << " doesn't exist.";
    	
    COCO_DEBUG("Loader") << task_out << " " << task_out_port << " " << 
				   task_in << " " << task_in_port;

	if (tasks_[task_out])
	{
		PortBase * left = tasks_[task_out]->getPort(task_out_port);
        PortBase * right = tasks_[task_in]->getPort(task_in_port);
        if (left && right)
        {
            // TBD: do at the very end of the loading (first load then execute)
            left->connectTo(right, policy);
        }
		else
		{
			COCO_FATAL() << "Either Component src: " << task_out << " doesn't have port: " << task_out_port
						 << " or Component in: " << task_in << " doesn't have port: " << task_in_port;
		}
	}
	else
		COCO_FATAL() << "Component: " << task_out << " doesn't exist"; 
}

void CocoLauncher::startApp()
{
    COCO_DEBUG("Loader") << "Starting the Activities!";
    if (activities_.size() <= 0)
    {
        COCO_FATAL() << "No app created, first run createApp()";
    }
    std::vector<Activity *> seq_act_list;
    for (auto act : activities_)
    {
        if (dynamic_cast<SequentialActivity *>(act))
        {
            seq_act_list.push_back(act);
            continue;
        }
        act->start();
    }
    if (seq_act_list.size() > 0)
    {
        if (seq_act_list.size() > 1)
            COCO_ERR() << "Only one sequential activity per application is allowed.\
                           Only the first will be run!";
        seq_act_list[0]->start();
    }
}

void CocoLauncher::waitToComplete()
{
    for (auto activity : activities_)
        activity->join();
}

void CocoLauncher::killApp()
{
    // Call activity stop
    // join on activity thread
    for (auto activity : activities_)
        activity->stop();
    waitToComplete();
}

std::unordered_map<std::string, TaskContext *>
CocoLoader::addLibrary(std::string library_file_name)
{
    std::unordered_map<std::string, TaskContext *> task_map;
    if (!ComponentRegistry::addLibrary(library_file_name, ""))
    {
        COCO_ERR() << "Failed to load library: " << library_file_name;
        return task_map;
    }
    for (auto task_name : ComponentRegistry::componentsName())
    {
        if (tasks_.find(task_name) != tasks_.end())
            continue;
        
        TaskContext * rc = ComponentRegistry::create(task_name, task_name);
        if (!rc)
        {
            COCO_ERR() << "Failed to create component: " << task_name;
            task_map.clear();
            break;
        }
        tasks_[task_name] = rc;
        task_map[task_name] = rc;        
    }
    return task_map;
}

}
