#include "ezoro.hpp"
#include <iostream>
#include <thread>
#include <chrono>


// dlopen cross platform
#ifdef WIN32
	#include <windows.h>
	typedef HANDLE dlhandle;
	#define DLLEXT ".dll"
	#define DLLPREFIX "lib"
	#define dlopen(x,y) LoadLibrary(x)
	#define dlsym(x,y) GetProcAddress(x,y)
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

/// as pointer to avoid issues of order in ctors
static coco::ComponentRegistry *singleton;

namespace coco
{

AttributeBase::AttributeBase(TaskContext * p, const std::string &name)
	: name_(name) 
{
	p->addAttribute(this);
}

OperationBase::OperationBase(Service * p, const std::string &name) 
	: name_(name)
{
	p->addOperation(this);
}

// -------------------------------------------------------------------
// Connection
// -------------------------------------------------------------------

void ConnectionBase::trigger()
{
	input_->triggerComponent();
}

PortBase::PortBase(TaskContext * p,const std::string &name, bool is_output, bool is_event)
	: task_(p), name_(name), is_output_(is_output), is_event_(is_event)
{
	task_->addPort(this);
}

bool PortBase::addConnection(std::shared_ptr<ConnectionBase> connection) 
{
	manager_.addConnection(connection);
	return true;
}

void PortBase::triggerComponent()
{
	task_->triggerActivity();
}


// -------------------------------------------------------------------
// Execution
// -------------------------------------------------------------------


void ExecutionEngine::init()
{
	task_->onConfig();
}

void ExecutionEngine::step()
{

	//processMessages();
    //processFunctions();
    if (task_) {
    	if (task_->state_ == RUNNING)
    	{
    		Logger log(task_->name());
    		task_->onUpdate();
    	}
    }
}

void ExecutionEngine::finalize()
{
}

Activity * createSequentialActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r = 0)
{
	return new SequentialActivity(sp, r);
}

Activity * createParallelActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r = 0) 
{
	return new ParallelActivity(sp, r);
}

void SequentialActivity::start() 
{
	this->entry();
}

void SequentialActivity::entry()
{
	runnable_->init();
	if(isPeriodic())
	{
		std::chrono::system_clock::time_point currentStartTime{ std::chrono::system_clock::now() };
		std::chrono::system_clock::time_point nextStartTime{ currentStartTime };
		while(!stopping_)
		{
			currentStartTime = std::chrono::system_clock::now();
			runnable_->step();
			nextStartTime =	currentStartTime + std::chrono::milliseconds(policy_.period_ms_);
			std::this_thread::sleep_until(nextStartTime); // NOT interruptible, limit of std::thread
		}
	}
	else
	{
		while(true)
		{
			// wait on condition variable or timer
			if(stopping_)
				break;
			else
				runnable_->step();
		}
	}
	runnable_->finalize();

}

void SequentialActivity::join()
{
	while(true);
}

void SequentialActivity::stop()
{
	if (active_) {
		stopping_ = true;
		if(!isPeriodic())
			trigger();
		else
		{
			// LIMIT: std::thread sleep cannot be interrupted
		}
	}
}

void ParallelActivity::start() 
{
	if(thread_)
		return;
	stopping_ = false;
	active_ = true;
	thread_ = std::move(std::unique_ptr<std::thread>(new std::thread(&ParallelActivity::entry,this)));
}

void ParallelActivity::join()
{
	if(thread_)
	thread_->join();
}

void ParallelActivity::stop() 
{
	if(thread_)
	{
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			stopping_ = true;
		}
		if(!isPeriodic())
			trigger();
		else
		{
			// LIMIT: std::thread sleep cannot be interrupted
		}
		thread_->join();
		std::cout << "Thread join\n";
	}
}

void ParallelActivity::trigger() 
{
	std::unique_lock<std::mutex> mlock(mutex_);
	pending_trigger_++;
	cond_.notify_one();
}

void ParallelActivity::entry()
{
	runnable_->init();
	if(isPeriodic())
	{
		std::chrono::system_clock::time_point currentStartTime{ std::chrono::system_clock::now() };
		std::chrono::system_clock::time_point nextStartTime{ currentStartTime };
		while(!stopping_)
		{
			currentStartTime = std::chrono::system_clock::now();
			runnable_->step();
			nextStartTime =		 currentStartTime + std::chrono::milliseconds(policy_.period_ms_);
			std::this_thread::sleep_until(nextStartTime); // NOT interruptible, limit of std::thread
		}
	}
	else
	{
		while(true)
		{
			// wait on condition variable or timer
			{
				std::unique_lock<std::mutex> mlock(mutex_);
				if(pending_trigger_ > 0) // TODO: if pendingtrigger is ATOMIC then skip the lock
				{
					pending_trigger_ = 0;
				}
				else
					cond_.wait(mlock);
			}
			if(stopping_)
				break;
			else
				runnable_->step();
		}
	}
	std::cout << "FINALIZE\n";
	runnable_->finalize();
}


bool Service::addAttribute(AttributeBase *a)
{
	if (attributes_[a->name()])
	{
		std::cerr << "An attribute with name: " << a->name() << " already exist\n";
		return false;
	}
	attributes_[a->name()] = a;
	return true;
}

AttributeBase *Service::getAttribute(std::string name) 
{
	auto it = attributes_.find(name);
	if(it == attributes_.end())
			return nullptr;
		else
			return it->second;
}

bool Service::addPort(PortBase *p) {
	if (ports_[p->name()]) {
		std::cerr << "A port with name: " << p->name() << " already exist\n";	
		return false;
	}
	else
	{
		ports_[p->name()] = p;
		return true;
	}
}

PortBase *Service::getPort(std::string name) 
{
	auto it = ports_.find(name);
	if(it == ports_.end())
			return nullptr;
		else
			return it->second;
}

bool Service::addOperation(OperationBase *o) {
	if (operations_[o->name()]) {
		std::cerr << "An operation with name: " << o->name() << " already exist\n";
		return false;
	}
	operations_[o->name()] = o;
	return true;
}

bool Service::addPeer(TaskContext *p) {
	peers_.push_back(p);
	return true;
}

Service * Service::provides(const std::string &x) {
	auto it = subservices_.find(x);
	if(it == subservices_.end())
	{
		Service * p = new Service(x);
		subservices_[x] = std::unique_ptr<Service>(p);
		return p;
	}
	else
		return it->second.get();
	return 0;
}

TaskContext::TaskContext() 
	: engine_(std::make_shared<ExecutionEngine>(this))
{
	//engine_ = std::make_shared<ExecutionEngine>(this);
	activity_ = nullptr;
	state_ = STOPPED;
}

void TaskContext::start()
{
	if (activity_ == nullptr)
	{
		std::cout << "Activity not found! Set an activity to start a component!\n";
		return;
	}
	if (state_ == RUNNING)
	{
		std::cout << "Task already running\n";
		return;
	}
	state_ = RUNNING;
	activity_->start();
}

void TaskContext::stop()
{
	if (activity_ == nullptr)
	{
		std::cout << "Activity not found!\n";
		return;
	} else if (!activity_->isActive())
	{
		std::cout << "Activity not running\n";
		return;
	}
	state_ = STOPPED;
	activity_->stop();
}

void TaskContext::triggerActivity() 
{
	activity_->trigger();
}

ComponentSpec::ComponentSpec(const std::string &name, makefx_t fx)
	: name_(name),fx_(fx)
{
	std::cout << "[coco] " << this << " spec selfregistering " << name << std::endl;
	ComponentRegistry::addSpec(this);
}

ComponentRegistry & ComponentRegistry::get()
{
	ComponentRegistry * a = singleton;
	if(!a)
	{
		singleton = new ComponentRegistry();
		std::cout << "[coco] registry creation " << singleton << std::endl;
		return *singleton;
	}
	else
		return *a;
}


// static
TaskContext * ComponentRegistry::create(const std::string &name)
{
	return get().create_(name);
}

// static
void ComponentRegistry::addSpec(ComponentSpec * s)
{
	get().addSpec_(s);
}

// static
bool ComponentRegistry::addLibrary(const std::string &l, const std::string &path)
{
	return get().addLibrary_(l, path);
}

// static
void ComponentRegistry::alias(const std::string &newname,const std::string &oldname)
{
	return get().alias(newname,oldname);
}

void ComponentRegistry::alias_(const std::string &newname,const std::string &oldname)
{
	auto it = specs.find(oldname);
	if(it != specs.end())
		specs[newname] = it->second;
}

TaskContext * ComponentRegistry::create_(const std::string &name)
{
	auto it = specs.find(name);
	if(it == specs.end())
		return 0;
	return it->second->fx_();
}

void ComponentRegistry::addSpec_(ComponentSpec * s)
{
	std::cout << "[coco] " << this << " adding spec " << s->name_ << " " << s << std::endl;	
	specs[s->name_] = s;
}

bool ComponentRegistry::addLibrary_(const std::string &lib, const std::string &path)
{
	std::string p = std::string(path);
	if(p.size() != 0 && p[p.size()-1] != DIRSEP)
		p += (DIRSEP);
	p += DLLPREFIX + std::string(lib) + DLLEXT;

	if(libs.find(p) != libs.end())
		return true; // already loaded

	// TODO: OS specific shared library extensions: so dylib dll
	typedef ComponentRegistry ** (*pfx_t)();
	dlhandle l = dlopen(p.c_str(),RTLD_NOW);
	if(!l) {
		std::cout << "Error: " << dlerror() << std::endl;
		return false;		
	}
	pfx_t pfx = (pfx_t)dlsym(l,"getComponentRegistry");
	if(!pfx)
		return false;
	ComponentRegistry ** other = pfx();
	if(!*other)
	{
		std::cout << "[coco] " << this << " propagating to " << other;
		*other = this;
	}
	else if(*other != this)
	{
		std::cout << "[coco] " << this << " embedding other " << *other << " stored in " << other << std::endl;

		// import the specs and the destroy the imported registry and replace it inside the shared library
		for(auto&& i : (*other)->specs) {
			specs[i.first] = i.second;
		}		
		delete *other;
		*other = this;
	}
	else
		std::cout << "[coco] " << this << " skipping self stored in " << other;
	libs.insert(p);
	return true;
}

/*---------------------------- */


void CocoLauncher::createApp()
{
	using namespace tinyxml2;
    XMLError error = doc_.LoadFile(config_file_.c_str());
    if (error != XML_NO_ERROR)
    {
    	std::cerr << "Error loading document: " <<  error << std::endl;
    	std::cerr << "XML file: " << config_file_ << " not found\n";
    	return;
    }

    XMLElement *components = doc_.FirstChildElement("package")->FirstChildElement("components");
    XMLElement *component = components->FirstChildElement("component");
    std::cout << "Parsing components\n";
	while (component)
	{
		parseComponent(component);
		component = component->NextSiblingElement("component");
	}
	XMLElement *connections = doc_.FirstChildElement("package")->FirstChildElement("connections");
	XMLElement *connection  = connections->FirstChildElement("connection");
	std::cout << "Parsing connections\n";
	while (connection)
	{
		parseConnection(connection);
		connection = connection->NextSiblingElement("connection");
	}

	for (auto it : peers_)
	{
		auto t = tasks_.find(it);
		tasks_.erase(t);
	}
}

void CocoLauncher::parseComponent(tinyxml2::XMLElement *component)
{
	using namespace tinyxml2;
	const char* task_name    = component->FirstChildElement("task")->GetText();
	std::cout << "Creating component: " << task_name << std::endl;
	
	tasks_[task_name] = ComponentRegistry::create(task_name);
	if (tasks_[task_name] == 0)
	{
		std::cout << "Component " << task_name << " not found, trying to load from library\n";
		const char* library_name = component->FirstChildElement("library")->GetText();
		const char* library_path = component->FirstChildElement("librarypath")->GetText();
		if (!ComponentRegistry::addLibrary(library_name, library_path))
		{
			std::cerr << "Failed to load library: " << library_name << std::endl;
			return;
		}
		tasks_[task_name] = ComponentRegistry::create(task_name);
		if (tasks_[task_name] == 0)
		{
			std::cerr << "Failed to create component: " << task_name << std::endl;
			return;
		}
	}
	std::cout << "Parsing attributes\n";
	TaskContext *t = tasks_[task_name];
	XMLElement *attributes = component->FirstChildElement("attributes");
	XMLElement *attribute  = attributes->FirstChildElement("attribute");
	while (attribute)
	{
		const std::string attr_name  = attribute->Attribute("name");
		const std::string attr_value = attribute->Attribute("value");
		if (t->getAttribute(attr_name))
			t->getAttribute(attr_name)->setValue(attr_value); 
		else
			std::cerr << "\tAttribute: " << attr_name << " doesn't exist\n";
		attribute = attribute->NextSiblingElement("attribute");
	}
	// Parsing possible peers
	XMLElement *peers = component->FirstChildElement("components");
	if (peers)
	{
    	XMLElement *peer = peers->FirstChildElement("component");
    	while (peer)
    	{
			parseComponent(peer);
			std::cout << "Find a peer and parsed\n";
			std::string peer_name = peer->FirstChildElement("task")->GetText();
			TaskContext *peer_task = tasks_[peer_name];
			if (peer_task)
				t->addPeer(peer_task);
			peers_.push_back(peer_name);
			peer = peer->NextSiblingElement("component");
		}
    }
    std::cout << "Calling init\n";
	t->init();
	std::cout << "Init called\n";
}

void CocoLauncher::parseConnection(tinyxml2::XMLElement *connection)
{
	using namespace tinyxml2;	
	ConnectionPolicy policy(connection->Attribute("data"),
							connection->Attribute("policy"),
							connection->Attribute("transport"),
							connection->Attribute("buffersize"));
	const std::string &task_out 	 = connection->FirstChildElement("src")->Attribute("task");
	const std::string &task_out_port = connection->FirstChildElement("src")->Attribute("port");
	const std::string &task_in	     = connection->FirstChildElement("dest")->Attribute("task");
	const std::string &task_in_port  = connection->FirstChildElement("dest")->Attribute("port");
	std::cout << task_out << " " << task_out_port << " " << task_in << " " << task_in_port << std::endl;
	if (tasks_[task_out])
	{
		if (tasks_[task_out]->getPort(task_out_port) && tasks_[task_in]->getPort(task_in_port))
			tasks_[task_out]->getPort(task_out_port)->connectTo(tasks_[task_in]->getPort(task_in_port), policy);
		else
			std::cerr << "Component: " << task_out << " doesn't have port: " << task_out_port << std::endl;
	}
	else
		std::cerr << "Component: " << task_out << " doesn't exist\n";
}

void CocoLauncher::startApp()
{
	std::cout << "Starting components\n";
	if (tasks_.size() == 0)
	{
		std::cerr << "No app created, first runn createApp()\n";
		return; 
	}
#ifdef __APPLE__
	for (auto &itr : tasks_)
	{
		if (itr.first != "GLManagerTask")
		{
			std::cout << "Starting component: " << itr.first << std::endl;
			itr.second->start();
		}
	}
	std::cout << "Starting component: GLManagerTask" << std::endl;
	tasks_["GLManagerTask"]->start();
#else
	for (auto &itr : tasks_)
	{
		std::cout << "Starting component: " << itr.first << std::endl;
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
	}
	delete xml_doc;
}

 void printXMLSkeleton(std::string task_name, std::string task_library, std::string task_library_path, bool adddoc, bool savefile)
{
	ComponentRegistry::addLibrary(task_library.c_str(), task_library_path.c_str());

	// TODO for task_library empty scan all 
	// TODO for task_name empty or * scan all the components of task_library

	subprintXMLSkeleton(task_name,task_library,task_library_path,adddoc,savefile);
}

} // end of namespace

extern "C" 
{
	coco::ComponentRegistry ** getComponentRegistry() { return &singleton; }
}

