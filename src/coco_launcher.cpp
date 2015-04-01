/**
 * Compact Framework Core
 * 2014-2015 Emanuele Ruffaldi and Filippo Brizzi @ Scuola Superiore Sant'Anna, Pisa, Italy
 */
#include "coco_launcher.h"

namespace coco
{

bool CocoLauncher::createApp()
{
	using namespace tinyxml2;
	COCO_LOG(1)  << "Creating app\n";
    XMLError error = doc_.LoadFile(config_file_.c_str());
    if (error != XML_NO_ERROR)
    {
    	COCO_LOG(10) << "Error loading document: " <<  error << std::endl;
    	COCO_LOG(10) << "XML file: " << config_file_ << " not found\n";
    	//COCO_FATAL() << "Error while loading XML file: " << config_file_;
    	return false;
    }
    const char *logconfig = findchildtext(doc_.FirstChildElement("package"),"logconfig",false);
    if (logconfig)
    {
    	if (logconfig[0] != 0)
    	{
    		std::cout << "Calling coco_init_log with file: " << logconfig << std::endl;
    		COCO_INIT_LOG(logconfig);
    	}
    	else
    	{
    		std::cout << "No file for coco_log\n";
    		COCO_INIT_LOG("");
    	}
    }
    else
    	std::cout << "No component logconfig\n";

    XMLElement *components = doc_.FirstChildElement("package")->FirstChildElement("components");
    XMLElement *component = components->FirstChildElement("component");
    COCO_LOG(1) << "Parsing components";
	while (component)
	{
		parseComponent(component);
		component = component->NextSiblingElement("component");
	}
	XMLElement *connections = doc_.FirstChildElement("package")->FirstChildElement("connections");
    if (connections)
    {
        XMLElement *connection  = connections->FirstChildElement("connection");
        COCO_LOG(1) << "Parsing connections";
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
	for (auto it : peers_)
	{
		auto t = tasks_.find(it);
		tasks_.erase(t);
	}
	return true;
}

tinyxml2::XMLElement * CocoLauncher::findchild(tinyxml2::XMLElement *p , const char * child, bool required)
{
	auto r =	p->FirstChildElement(child);
	if(r)
		return r;
	else if(!required)
		return 0;
	else
		throw parseexception();
}

const char * CocoLauncher::findchildtext(tinyxml2::XMLElement * p, const char * child, bool required)
{
	auto r =	p->FirstChildElement(child);
	if(r)
		return r->GetText();
	else if(!required)
		return 0;
	else
		throw parseexception();	
}


bool CocoLauncher::parseComponent(tinyxml2::XMLElement *component)
{
	using namespace tinyxml2;
	const char* task_name    = component->FirstChildElement("task")->GetText();
	XMLElement *name = component->FirstChildElement("name");
	std::string component_name;
	if (name)
	{
		component_name = name->GetText();
		if (component_name.empty())
			component_name = task_name;
	}
	else
		component_name = task_name;
	
	COCO_LOG(1) << "Creating component: " << task_name << 
				   " with name: " << component_name;

	tasks_[component_name] = ComponentRegistry::create(task_name);			

	try
	{
		if (tasks_[component_name] == 0)
		{
			COCO_LOG(1) << "Component " << task_name << 
						   " not found, trying to load from library";
			const char* library_name = component->
					FirstChildElement("library")->GetText();
			const char* library_path = findchildtext(component,"librarypath",false);
			if (!ComponentRegistry::addLibrary(library_name, !library_path ?"":library_path))
			{
				COCO_FATAL() << "Failed to load library: " << library_name;
				return false;
			}
			tasks_[component_name] = ComponentRegistry::create(task_name);
			if (tasks_[component_name] == 0)
			{
				COCO_FATAL() << "Failed to create component: " << task_name;
				return false;
			}
		}
		COCO_LOG(1) << "Parsing attributes";
		TaskContext *t = tasks_[component_name];
		XMLElement *attributes = component->FirstChildElement("attributes");
	    if (attributes)
	    {
	        XMLElement *attribute  = attributes->FirstChildElement("attribute");
	        while (attribute)
	        {
	            const std::string attr_name  = attribute->Attribute("name");
	            const std::string attr_value = attribute->Attribute("value");
	            if (t->getAttribute(attr_name))
	                t->getAttribute(attr_name)->setValue(attr_value);
	            else
	                COCO_ERR() << "Attribute: " << attr_name << " doesn't exist";
	                //std::cerr << "\tAttribute: " << attr_name << " doesn't exist\n";
	            attribute = attribute->NextSiblingElement("attribute");
	        }
	    }
		// Parsing possible peers
		XMLElement *peers = component->FirstChildElement("components");
		if (peers)
		{
	    	XMLElement *peer = peers->FirstChildElement("component");
	    	while (peer)
	    	{
				COCO_LOG(1) << "Parsing peer";
				parseComponent(peer);
				//std::cout << "Find a peer and parsed\n";
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
	    //std::cout << "Calling init\n";
		t->init();
	}
	catch(parseexception e)
	{
		COCO_FATAL() << "Parse error in component";
		return false;
	}
	return true;
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
		if (tasks_[task_out]->getPort(task_out_port) && tasks_[task_in]->getPort(task_in_port))
        {
            tasks_[task_out]->getPort(task_out_port)->connectTo(tasks_[task_in]->getPort(task_in_port), policy);
        }
		else
		{
			/*
			std::cerr << "Component out: " << task_out << " doesn't have port: " << task_out_port << std::endl;
			std::cerr << "Component in: " << task_in << " doesn't have port: " << task_in_port << std::endl;
			std::cerr << "Ports are: \n";
			for (auto p : tasks_[task_in]->getPortNames())
				std::cout << "port: " << p << std::endl;
			*/
			COCO_FATAL() << "Either Component src: " << task_out << " doesn't have port: " << task_out_port
						 << " or Component in: " << task_in << " doesn't have port: " << task_in_port;
		}
	}
	else
		COCO_FATAL() << "Component: " << task_out << " doesn't exist"; 
}

void CocoLauncher::startApp()
{
	//std::cout << "Starting components\n";
	COCO_LOG(1) << "Starting components";
	if (tasks_.size() == 0)
	{
		//std::cerr << "No app created, first runn createApp()\n";
		COCO_FATAL() << "No app created, first runn createApp()";
		return; 
	}
#ifdef __APPLE__
	for (auto &itr : tasks_)
	{
		if (itr.first != "GLManagerTask")
		{
			//std::cout << "Starting component: " << itr.first << std::endl;
			COCO_LOG(1) << "Starting component: " << itr.first;
			itr.second->start();
		}
	}
	//std::cout << "Starting component: GLManagerTask" << std::endl;
	COCO_LOG(1) << "Starting component: GLManagerTask";
	tasks_["GLManagerTask"]->start();
#else
	for (auto &itr : tasks_)
	{
		//std::cout << "Starting component: " << itr.first << std::endl;
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

static void subprintXMLSkeleton(std::string task_name,std::string task_library,std::string task_library_path, bool adddoc, bool savefile) 
{
	using namespace tinyxml2;
	TaskContext *task = ComponentRegistry::create(task_name.c_str());

	if(!task)
		return;

	XMLDocument *xml_doc = new XMLDocument();
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
}

 void printXMLSkeleton(std::string task_library, std::string task_library_path, bool adddoc, bool savefile)
{
	ComponentRegistry::addLibrary(task_library.c_str(), task_library_path.c_str());

	// TODO for task_library empty scan all 
	// TODO for task_name empty or * scan all the components of task_library

    //for ()
    for (auto task_name : ComponentRegistry::componentsName())
        subprintXMLSkeleton(task_name, task_library, task_library_path, adddoc, savefile);
}

}
