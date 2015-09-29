/**
 * Compact Framework Core
 * 2014-2015 Emanuele Ruffaldi and Filippo Brizzi @ Scuola Superiore Sant'Anna, Pisa, Italy
 */
#include "coco/coco_launcher.h"

namespace coco
{

bool CocoLauncher::parseFile(tinyxml2::XMLDocument & doc, bool top)
{
    using namespace tinyxml2;
    XMLElement *package = doc.FirstChildElement("package");

    // TBD: only one log specification for ALL execution
    // Option1: use top
    // Option2: collect value and use latest
    // Option3: ignore sub
    parseLogConfig(package->FirstChildElement("logconfig"));

    // TBD: only libraries_path_ ONCE
    parsePaths(package->FirstChildElement("resourcespaths"));

    COCO_LOG(1) << "Parsing includes";
    XMLElement *includes = package->FirstChildElement("includes");
    if(includes)
    {
        XMLElement *include = includes->FirstChildElement("include");
        while (include)
        {
            const char *path        = include->Attribute("path");
            if(path)
            {
                tinyxml2::XMLDocument doc2;
                XMLError error = doc2.LoadFile(path);
                if (error != XML_NO_ERROR)
                {
                    std::cerr << "Error: " << error << std::endl <<
                                 "While loading XML sub file: " << path << std::endl;
                    exit(1);
                }
                parseFile(doc2,true);
            }
            include = include->NextSiblingElement("include");
        }
    }

    COCO_LOG(1) << "Parsing components";
    XMLElement *components = package->FirstChildElement("components");
    if (!components)
        COCO_FATAL() << "Missing <components> tag\n";
    XMLElement *component = components->FirstChildElement("component");
    while (component)
    {
        parseComponent(component);
        component = component->NextSiblingElement("component");
    }

    COCO_LOG(1) << "Parsing connections";
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
        COCO_LOG(1) << "No connections found.";
    }
    return true;
}

bool CocoLauncher::createApp()
{
	using namespace tinyxml2;
    XMLError error = doc_.LoadFile(config_file_.c_str());
    if (error != XML_NO_ERROR)
    {
        std::cerr << "Error: " << error << std::endl <<
                     "While loading XML file: " << config_file_ << std::endl;
        exit(1);
    }
    parseFile(doc_,true);

    /* Removing the peers from the task list */
	for (auto it : peers_)
	{
        auto t = tasks_.find(it);
        if (t != tasks_.end())
		  tasks_.erase(t);
	}
	return true;
}

void CocoLauncher::parseLogConfig(tinyxml2::XMLElement *logconfig)
{
    if (logconfig)
    {
        const char *log_config_file = logconfig->GetText();
        if (log_config_file[0] != 0)
        {
            COCO_INIT_LOG(log_config_file);
        }
        else
        {
            COCO_INIT_LOG();
        }
    }
    else
    {
        COCO_INIT_LOG();
    }
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

void CocoLauncher::parseComponent(tinyxml2::XMLElement *component, bool is_peer)
{
	using namespace tinyxml2;

    XMLElement *task = component->FirstChildElement("task");
    if (!task)
        COCO_FATAL() << "No <task> tag in component\n";
    const char* task_name = task->GetText();

    /* Looking if it has been specified a name different from the task name */
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
	COCO_LOG(1) << "Creating component: " << task_name << 
				   " with name: " << component_name;

	tasks_[component_name] = ComponentRegistry::create(task_name, component_name);			


    if (tasks_[component_name] == 0)
    {
        COCO_LOG(1) << "Component " << task_name <<
                       " not found, trying to load from library";
        const char* library_name = component->
                FirstChildElement("library")->GetText();
        XMLElement *librarypath = component->FirstChildElement("librarypath");
        if (!ComponentRegistry::addLibrary(library_name,
                                           !librarypath ?
                                           libraries_path_ : librarypath->GetText()))
        {
            COCO_FATAL() << "Failed to load library: " << library_name;
        }
        tasks_[component_name] = ComponentRegistry::create(task_name, component_name);
        if (tasks_[component_name] == 0)
        {
            COCO_FATAL() << "Failed to create component: " << task_name;
        }
    }

    TaskContext *t = tasks_[component_name];
    t->setInstantiationName(component_name);
    if (!is_peer)
    {
        COCO_LOG(1) << "Parsing schedule policy";
        XMLElement *schedule_policy = component->FirstChildElement("schedulepolicy");
        parseSchedule(schedule_policy, t);
    }

    COCO_LOG(1) << "Parsing attributes";
    XMLElement *attributes = component->FirstChildElement("attributes");
    parseAttribute(attributes, t);

    // Parsing possible peers
    COCO_LOG(1) << "Parsing possible peers";
    XMLElement *peers = component->FirstChildElement("components");
    parsePeers(peers, t);

    // TBD: better do that at the very end of loading process
    t->init();
}

void CocoLauncher::parseSchedule(tinyxml2::XMLElement *schedule_policy, TaskContext *t)
{
    using namespace tinyxml2;
    //coco::SchedulePolicy policy(coco::SchedulePolicy::TRIGGERED);
    //this->setActivity(createParallelActivity(policy, engine_));
    if (!schedule_policy)
        COCO_FATAL() << "No schedule policy found for task " << t->name();

    const char *activity        = schedule_policy->Attribute("activity");
    if (!activity)
        COCO_FATAL() << "No activity attribute found in schedule policy specification";
    const char *activation_type = schedule_policy->Attribute("type");
    if (!activation_type)
        COCO_FATAL() << "No type attribute found in schedule policy specification";
    const char *value           = schedule_policy->Attribute("value");

    SchedulePolicy *policy;
    if (strcmp(activation_type, "triggered") == 0 ||
        strcmp(activation_type, "Triggered") == 0 ||
        strcmp(activation_type, "TRIGGERED") == 0)
    {
        policy = new SchedulePolicy(SchedulePolicy::TRIGGERED);
    }
    else if (strcmp(activation_type, "periodic") == 0 ||
             strcmp(activation_type, "Periodic") == 0 ||
             strcmp(activation_type, "PERIODIC") == 0)
    {
        if (!value)
            COCO_FATAL() << "Task " << t->name() << " scheduled as periodic but no period provided";

        policy = new SchedulePolicy(SchedulePolicy::PERIODIC, atoi(value));
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
        t->setActivity(createParallelActivity(*policy, t->engine()));
    }
    else if (strcmp(activity, "sequential") == 0 ||
             strcmp(activity, "Sequential") == 0 ||
             strcmp(activity, "SEQUENTIAL") == 0)
    {
        t->setActivity(createSequentialActivity(*policy, t->engine()));
    }
    else
    {
        COCO_FATAL() << "Schduele policy: " << activity << " is not know\n" <<
                        "Possibilities are: parallel, sequential";
    }
    delete policy;

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
            //if (strcmp(&rp.back(), "/") != 0)
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
            COCO_LOG(1) << "Parsing peer";
            parseComponent(peer, true);
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
    
    task_out_port = tasks_[task_out]->name() + "_" + task_out_port;
	const std::string &task_in = connection->FirstChildElement("dest")->Attribute("task");
	std::string task_in_port  = connection->FirstChildElement("dest")->Attribute("port");
    
    if (!tasks_[task_in])
        COCO_FATAL() << "Task with name: " << task_in << " doesn't exist.";
    
    task_in_port = tasks_[task_in]->name() + "_" + task_in_port;
	
    COCO_LOG(1) << task_out << " " << task_out_port << " " << 
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
	COCO_LOG(1) << "Starting " << tasks_.size() << " components";
	if (tasks_.size() == 0)
	{
		COCO_FATAL() << "No app created, first runn createApp()";
		return; 
	}
#ifdef __APPLE__
	TaskContext *graphix_task = nullptr;
    for (auto &itr : tasks_)
	{
		if (itr.first != "GLManagerTask")
		{
			COCO_LOG(1) << "Starting component: " << itr.first;
			itr.second->start();
		}
        else
            graphix_task = itr.second;
	}
	COCO_LOG(1) << "Starting component: GLManagerTask";
    if (graphix_task)
	   graphix_task->start();
#else
	for (auto &itr : tasks_)
	{
		COCO_LOG(1) << "Starting component: " << itr.first;
		itr.second->start();	
	}
#endif
}

struct scopedxml
{
	scopedxml(tinyxml2::XMLDocument *xml_doc, tinyxml2::XMLElement * apa, const std::string &atag): pa(apa)
	{
		e = xml_doc->NewElement(atag.c_str());
	}

	operator tinyxml2::XMLElement * ()  { return e; }

	tinyxml2::XMLElement *operator ->() { return e; }

	~scopedxml()
	{
		pa->InsertEndChild(e);
	}

	tinyxml2::XMLElement * e;
	tinyxml2::XMLElement * pa;
};

static tinyxml2::XMLElement* xmlnodetxt(tinyxml2::XMLDocument *xml_doc,tinyxml2::XMLElement * pa, const std::string &tag, const std::string text)
{
	using namespace tinyxml2;
	XMLElement *xml_task = xml_doc->NewElement(tag.c_str());
	XMLText *task_text = xml_doc->NewText(text.c_str());
	xml_task->InsertEndChild(task_text);
	pa->InsertEndChild(xml_task);
	return xml_task;
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
        
        tasks_[task_name] = ComponentRegistry::create(task_name, task_name);
        if (tasks_[task_name] == 0)
        {
            COCO_ERR() << "Failed to create component: " << task_name;
            task_map.clear();
            break;
        }
        task_map[task_name] = tasks_[task_name];
    }
    return task_map;
}

static bool subprintXMLSkeleton(std::string task_name,std::string task_library,std::string task_library_path, bool adddoc, bool savefile) 
{
	using namespace tinyxml2;
    
	TaskContext *task = ComponentRegistry::create(task_name, task_name);
	if(!task)
    {
        std::cout << "Cannot create " << task_name << " " << task_library << std::endl;
		return false; 
    }

	tinyxml2::XMLDocument *xml_doc = new tinyxml2::XMLDocument();
	XMLElement *xml_package = xml_doc->NewElement("package");
	xml_doc->InsertEndChild(xml_package);
	{
		scopedxml xml_components(xml_doc,xml_package,"components");
		scopedxml xml_component(xml_doc,xml_components,"component");
		
		XMLElement *xml_task = xmlnodetxt(xml_doc,xml_component,"task",task_name);
		if(adddoc && !task->doc().empty())
			XMLElement *xml_doca = xmlnodetxt(xml_doc,xml_component,"info",task->doc());

		XMLElement *xml_lib = xmlnodetxt(xml_doc,xml_component,"library",task_library);
		XMLElement *xml_libpath = xmlnodetxt(xml_doc,xml_component,"librarypath",task_library_path);
		{
			scopedxml xml_attributes(xml_doc,xml_component,"attributes");
			for (auto itr : task->getAttributes()) 
			{
				scopedxml xml_attribute(xml_doc,xml_attributes,"attribute");
				xml_attribute->SetAttribute("name", itr->name().c_str());
				xml_attribute->SetAttribute("value", "");
				if(adddoc)
                    xml_attribute->SetAttribute("type",itr->assig().name());
				if(adddoc && !itr->doc().empty())
				{
					XMLElement *xml_doca = xmlnodetxt(xml_doc,xml_attribute,"doc",itr->doc());
				}
			}
		}
		if(adddoc)
		{
			{
				scopedxml xml_ports(xml_doc,xml_component,"ports");
				for(auto itr : task->getPorts()) 
				{
					scopedxml xml_port(xml_doc,xml_ports,"ports");
					xml_port->SetAttribute("name", itr->name().c_str());
					xml_port->SetAttribute("type", itr->getTypeInfo().name());
					if(adddoc && !itr->doc().empty())
					{
						XMLElement *xml_doca = xmlnodetxt(xml_doc,xml_port,"doc",itr->doc());						
					}
				}
			}

			{
				scopedxml xml_ops(xml_doc,xml_component,"operations");
				for(auto itr: task->getOperations())
				{
					scopedxml xml_op(xml_doc,xml_ops,"operation");
					xml_op->SetAttribute("name", itr->name().c_str());
					xml_op->SetAttribute("type", itr->assig().name());
					if(adddoc && !itr->doc().empty())
					{
						XMLElement *xml_doca = xmlnodetxt(xml_doc,xml_op,"doc",itr->doc());						
					}
				}
			}
		}
	} // components

	if(!adddoc)
	{

		// Adding connections
        std::cout << "Inserting connections\n";
        //COCO_LOG(1) << "Inserting connections";

		scopedxml xml_connections(xml_doc,xml_package,"connections");

		for(auto itr : task->getPorts()) 
		{
			scopedxml xml_connection(xml_doc,xml_connections,"connection");
			xml_connection->SetAttribute("data", "");
			xml_connection->SetAttribute("policy", "");
			xml_connection->SetAttribute("transport", "");
			xml_connection->SetAttribute("buffersize", "");
			scopedxml xml_src(xml_doc,xml_connection,"src");
			scopedxml xml_dest(xml_doc,xml_connection,"dest");

			if (itr->isOutput()) {
				xml_src->SetAttribute("task", task_name.c_str());
				xml_src->SetAttribute("port", itr->name().c_str());
				xml_dest->SetAttribute("task", "");
				xml_dest->SetAttribute("port", "");
			} else {
				xml_src->SetAttribute("task", "");
				xml_src->SetAttribute("port", "");
				xml_dest->SetAttribute("task", task_name.c_str());
				xml_dest->SetAttribute("port", itr->name().c_str());
			}
		}
	}


	if(savefile)
	{
		std::string out_xml_file = task_name + std::string(".xml");
		xml_doc->SaveFile(out_xml_file.c_str());	
	}
	else
	{
		XMLPrinter printer;
		xml_doc->Print(&printer);
        std::cout << printer.CStr();
        //COCO_LOG(1) << printer.CStr();
	}
	delete xml_doc;
    return true;
}

 void printXMLSkeleton(std::string com_library, std::string com_library_path, bool adddoc, bool savefile)
{
    ComponentRegistry::addLibrary(com_library.c_str(), com_library_path.c_str());

    for (auto com_name : ComponentRegistry::componentsName())
    {
        std::cout << "Component: " << com_name << std::endl;
        subprintXMLSkeleton(com_name, com_library, com_library_path, adddoc, savefile);
    }
}

}
