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


/** 
 * No thread but thread safe
 */
 
class SequentialActivity: public Activity {
public:
	SequentialActivity(SchedulePolicy policy, std::shared_ptr<RunnableInterface> r = nullptr) 
		: Activity(policy, r) {}

	virtual void start() override;
	virtual void stop() override;
	//virtual bool isActive() override {};
	//virtual void execute() override {};
	virtual void trigger() override {};
	virtual void join() override;
protected:
	void entry() override;
}; 

/** 
 * Uses thread
 */
class ParallelActivity: public Activity
{
public:
	/** \brief simply call Activity constructor */
	ParallelActivity(SchedulePolicy policy, std::shared_ptr<RunnableInterface> r = nullptr) 
		: Activity(policy, r) {}

	virtual void start() override;
	virtual void stop() override;
	//virtual bool isActive() override {};
	//virtual void execute() override {};
	virtual void trigger() override;
	virtual void join() override;
protected:
	void entry() override;

	int pendingtrigger_ = 0;
	std::unique_ptr<std::thread> thread_;
	std::mutex mutex_t_;
	std::condition_variable cond_;
};


ComponentSpec::ComponentSpec(const char * name, makefx_t fx)
	: name_(name),fx_(fx)
{
	std::cout << "[coco] " << this << " spec selfregistering " << name << std::endl;
	ComponentRegistry::addSpec(this);
}

ComponentRegistry & ComponentRegistry::get() {
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
TaskContext * ComponentRegistry::create(const char * name) {
	return get().create_(name);
}

// static
void ComponentRegistry::addSpec(ComponentSpec * s) {
	get().addSpec_(s);
}

// static
bool ComponentRegistry::addLibrary(const char * l, const char * path) {
	return get().addLibrary_(l, path);
}

// static
void ComponentRegistry::alias(const char * newname,const char * oldname) {
	return get().alias(newname,oldname);
}

void ComponentRegistry::alias_(const char * newname,const char * oldname) {
	auto it = specs.find(oldname);
	if(it != specs.end())
		specs[newname] = it->second;
}

TaskContext * ComponentRegistry::create_(const char * name) {
	auto it = specs.find(name);
	if(it == specs.end())
		return 0;
	return it->second->fx_();
}

void ComponentRegistry::addSpec_(ComponentSpec * s) {
	std::cout << "[coco] " << this << " adding spec " << s->name_ << " " << s << std::endl;	
	specs[s->name_] = s;
}

bool ComponentRegistry::addLibrary_(const char * lib, const char * path) {
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



/// --------------------------------

#if 0
PropertyBase::PropertyBase(TaskContext * p, const char * name)
	: name_(name) {
		p->addProperty(this);
}
#endif

AttributeBase::AttributeBase(TaskContext * p, std::string name)
	: name_(name) 
	{
		p->addAttribute(this);
	}


// -------------------------------------------------------------------
// Connection
// -------------------------------------------------------------------
PortBase::PortBase(TaskContext * p,const char * name, bool is_output, bool is_event)
	: task_(p), name_(name), is_output_(is_output), is_event_(is_event) {
	task_->addPort(this);
}

bool PortBase::addConnection(std::shared_ptr<ConnectionBase> connection) 
{
		manager_.addConnection(connection);
		return true;
}

int PortBase::connectionsCount() const
{
	return manager_.connectionsSize();
}

void ConnectionBase::trigger() 
{
	input_->triggerComponent();
}

void PortBase::triggerComponent() 
{
	task_->triggerActivity();
}


// -------------------------------------------------------------------
// Execution
// -------------------------------------------------------------------


/*
TaskContext * System::create(const char * name)
{
	TaskContext * c = ComponentRegistry::create(name);
	if(!c)
		return c;
	c->onconfig();
	return c;
}
*/

void ExecutionEngine::init() {
	task_->onConfig();
}

void ExecutionEngine::step() {
	// TODO: clarify case of the operations async
	// 
	// look at the queue and check for operation requests (TBD LATER)
	// if it is a periodic task simply call update over the task
	// if it is an aperiodc check if the task has been triggered by some port or message:
		// ok ma come fa a passare il contenuto del messaggio a onUpdate?
	
	// In Orocos this funciton simple run the onUpdate...no check, I think that should be up to Activity

	//processMessages();
    //processFunctions();
    if (task_) {
    	if (task_->state_ == RUNNING) {
    		//task_->prepareUpdate();
    		LoggerManager::getInstance()->addServiceTime(task_->getName(), clock());
    		Logger log(task_->getName());
    		task_->onUpdate();
    	}
    }
}

//void ExecutionEngine::loop() {
//	step();
//}
/*
bool ExecutionEngine::breakLoop() {
	if (task_)
        return task_->breakLoop();
    return true;
}
*/
void ExecutionEngine::finalize() {
}


Activity * createSequentialActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r = 0) {
	return new SequentialActivity(sp, r);
}

Activity * createParallelActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r = 0) 
{
	return new ParallelActivity(sp, r);
}

/*void Activity::run(std::shared_ptr<RunnableInterface> a) 
{
	runnable_ = a;	
}*/

void SequentialActivity::start() 
{
	this->entry();
}

void SequentialActivity::entry() {
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
	while(true)
		;
}

void SequentialActivity::stop() {
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
	//runnable_->init();
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
			std::unique_lock<std::mutex> mlock(mutex_t_);
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
	std::unique_lock<std::mutex> mlock(mutex_t_);
	pendingtrigger_++;
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
				std::unique_lock<std::mutex> mlock(mutex_t_);
				if(pendingtrigger_ > 0) // TODO: if pendingtrigger is ATOMIC then skip the lock
				{
					pendingtrigger_ = 0;
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

bool Service::addAttribute(AttributeBase *a) {
	if (attributes_[a->name()]) {
		std::cerr << "An attribute with name: " << a->name() << " already exist\n";
		return false;
	}
	attributes_[a->name()] = a;
	attributes_list_.push_back(a->name());
	return true;
}

#if 0
bool Service::addProperty(PropertyBase *a) {
	if (properties_[a->name()]) {
		std::cerr << "A property with name: " << a->name() << " already exist\n";
		return false;
	}
	properties_[a->name()] = a;
	return true;
}
#endif

bool Service::addPort(PortBase *p) {
	if (ports_[p->name_]) {
		std::cerr << "A port with name: " << p->name_ << " already exist\n";	
		return false;
	}
	ports_[p->name_] = p;
	ports_list_.push_back(p->name_);
	return true;
}

PortBase *Service::getPort(std::string name) 
{
	auto it = ports_.find(name);
	if(it == ports_.end())
			return nullptr;
		else
			return it->second;
}

Service * Service::provides(const char *x) {
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

TaskContext::TaskContext() : engine_(std::make_shared<ExecutionEngine>(this))
{
	//engine_ = std::make_shared<ExecutionEngine>(this);
	activity_ = nullptr;
	state_ = STOPPED;
}

void TaskContext::start() {
	if (activity_ == nullptr) {
		std::cout << "Activity not found! Set an activity to start a component!\n";
		return;
	}
	if (state_ == RUNNING) {
		std::cout << "Task already running\n";
		return;
	}
	state_ = RUNNING;
	activity_->start();
}

void TaskContext::stop() {
	if (activity_ == nullptr) {
		std::cout << "Activity not found!\n";
		return;
	} else if (!activity_->isActive()) {
		std::cout << "Activity not running\n";
		return;
	}
	state_ = STOPPED;
	activity_->stop();
}
/*
bool TaskContext::breakLoop() 
{
	return false;
}
*/
void TaskContext::triggerActivity() 
{
	activity_->trigger();
}

/** -------------------------------------------------
 *	LOGGER
 * ------------------------------------------------- */

LoggerManager::LoggerManager(const char *log_file) {
	if (log_file) {
		log_file_.open(log_file);
	}
}

LoggerManager::~LoggerManager() {
	log_file_.close();
}

LoggerManager* LoggerManager::getInstance() {
	const char *log_file = 0;
	static LoggerManager log(log_file);
	return &log;
}

void LoggerManager::addTime(std::string id, double elapsed_time) {
	time_list_[id].push_back(elapsed_time);
}

void LoggerManager::addServiceTime(std::string id, clock_t service_time) {
	service_time_list_[id].push_back(service_time);
}

void LoggerManager::printStatistic(std::string id) {
	std::cout << id << std::endl;
	if (time_list_[id].size() > 0) { 
		double tot_time = 0;
		for(auto &itr : time_list_[id])
			tot_time += itr;

		double avg_time = tot_time / time_list_[id].size();
		std::cout << "\tExecution time: " << avg_time << " s on " << time_list_[id].size() << " iterations\n";

		if (log_file_.is_open())
			log_file_ << id << " " << avg_time << " s on " << time_list_[id].size() << " iterations\n";
	}
// Service Time
	if (service_time_list_[id].size() > 0) {
		double tot_time = 0;
		clock_t tmp = service_time_list_[id][0];
		for (int i = 1; i < service_time_list_[id].size(); ++i){
			tot_time += (double)(service_time_list_[id][i] - tmp) / CLOCKS_PER_SEC;
			tmp = service_time_list_[id][i];
		}
		
		double avg_time = tot_time / (service_time_list_[id].size() - 1);
		std::cout << "\tService Time: " << " " << avg_time << " s on " << service_time_list_[id].size() << " iterations\n";

		if (log_file_.is_open())
			log_file_ << "Service Time: " << " " << avg_time << " s on " << service_time_list_[id].size() << " iterations\n";
	}
}

void LoggerManager::printStatistics() {
	std::cout << "Statistics of " << time_list_.size() << " components\n";
	std::map<std::string, std::vector<double>>::iterator map_itr;
	for (map_itr = time_list_.begin(); map_itr != time_list_.end(); ++ map_itr) {
		printStatistic(map_itr->first);
	}
}

void LoggerManager::resetStatistics() {
	time_list_.clear();
	service_time_list_.clear();
	std::cout << "Statistics cleared\n";
}

/*---------------------------- */

Logger::Logger(std::string name) : name_(name){
	start_time_ = clock();

}

Logger::~Logger() {
	double elapsed_time = ((double)(clock() - start_time_)) / CLOCKS_PER_SEC;
	LoggerManager::getInstance()->addTime(name_, elapsed_time);
}

void CocoLauncher::createApp() {
	using namespace tinyxml2;
    XMLError error = doc_.LoadFile(config_file_);
    if (error != XML_NO_ERROR) {
    	std::cerr << "Error loading document: " <<  error << std::endl;
    	std::cerr << "XML file: " << config_file_ << " not found\n";
    	return;
    }

    XMLElement *components = doc_.FirstChildElement("package")->FirstChildElement("components");
    XMLElement *component = components->FirstChildElement("component");
    std::cout << "Parsing components\n";
	while (component) {
		parseComponent(component);
		component = component->NextSiblingElement("component");
	}
	XMLElement *connections = doc_.FirstChildElement("package")->FirstChildElement("connections");
	XMLElement *connection  = connections->FirstChildElement("connection");
	std::cout << "Parsing connections\n";
	while (connection) {
		parseConnection(connection);
		connection = connection->NextSiblingElement("connection");
	} 
}

void CocoLauncher::parseComponent(tinyxml2::XMLElement *component) {
	using namespace tinyxml2;
	const char* task_name    = component->FirstChildElement("task")->GetText();
	std::cout << "Creating component: " << task_name << std::endl;
	
	tasks_[task_name] = ComponentRegistry::create(task_name);
	if (tasks_[task_name] == 0) {
		std::cout << "Component " << task_name << " not found, trying to load from library\n";
		const char* library_name = component->FirstChildElement("library")->GetText();
		const char* library_path = component->FirstChildElement("librarypath")->GetText();
		if (!ComponentRegistry::addLibrary(library_name, library_path)) {
			std::cerr << "Failed to load library: " << library_name << std::endl;
			return;
		}
		tasks_[task_name] = ComponentRegistry::create(task_name);
		if (tasks_[task_name] == 0) {
			std::cerr << "Failed to create component: " << task_name << std::endl;
			return;
		}
	}
	TaskContext *t = tasks_[task_name];
	XMLElement *attributes = component->FirstChildElement("attributes");
	XMLElement *attribute  = attributes->FirstChildElement("attribute");
	while (attribute) {
		const char *attr_name  = attribute->Attribute("name");
		const char *attr_value = attribute->Attribute("value");

		if (t->getAttribute(attr_name)){
			t->getAttribute(attr_name)->setValue(attr_value); 
		}
		else
			std::cerr << "\tAttribute: " << attr_name << " doesn't exist\n";
		
		attribute = attribute->NextSiblingElement("attribute");
	}
	t->init();
}

void CocoLauncher::parseConnection(tinyxml2::XMLElement *connection) {
	using namespace tinyxml2;	
	ConnectionPolicy policy(connection->Attribute("data"),
							connection->Attribute("policy"),
							connection->Attribute("transport"),
							connection->Attribute("buffersize"));
	const char *task_out 	  = connection->FirstChildElement("src")->Attribute("task");
	const char *task_out_port = connection->FirstChildElement("src")->Attribute("port");
	const char *task_in	      = connection->FirstChildElement("dest")->Attribute("task");
	const char *task_in_port  = connection->FirstChildElement("dest")->Attribute("port");
	if (tasks_[task_out])
		if (tasks_[task_out]->getPort(task_out_port))
			tasks_[task_out]->getPort(task_out_port)->connectTo(tasks_[task_in]->getPort(task_in_port), policy);
		else
			std::cerr << "Component: " << task_out << " doesn't have port: " << task_out_port << std::endl;
	else
		std::cerr << "Component: " << task_out << " doesn't exist\n";
}

void CocoLauncher::startApp() {
	std::cout << "Starting components\n";
	if (tasks_.size() == 0) {
		std::cerr << "No app created, first runn createApp()\n";
		return; 
	}
#ifdef __APPLE__	
	for (auto &itr : tasks_) {
		std::cout << "Starting component: " << itr.first << std::endl;
		if (itr.first != "GLManagerTask")
			itr.second->start();
	}
	tasks_["GLManagerTask"]->start();
#else
	for (auto &itr : tasks_) {
		std::cout << "Starting component: " << itr.first << std::endl;
			itr.second->start();	
	}
#endif
}

void printXMLScheleton(std::string task_name, std::string task_library, std::string task_library_path) {
	using namespace tinyxml2;
	std::cout << "XML 0\n";
	ComponentRegistry::addLibrary(task_library.c_str(), task_library_path.c_str());
	TaskContext *task = ComponentRegistry::create(task_name.c_str());
	std::cout << "XML 1\n";
	XMLDocument *xml_doc = new XMLDocument();

	XMLElement *xml_package = xml_doc->NewElement("package");
	xml_doc->InsertEndChild(xml_package);
	XMLElement *xml_components = xml_doc->NewElement("components");
	xml_package->InsertEndChild(xml_components);
	XMLElement *xml_component = xml_doc->NewElement("component");
	xml_components->InsertEndChild(xml_component);
	
	XMLElement *xml_info = xml_doc->NewElement("info");
	//std::cout << task->info() << std::endl;
	//XMLText *info_text = xml_doc->NewText(task->info().c_str());
	//xml_info->InsertEndChild(info_text);
	xml_component->InsertEndChild(xml_info);

	XMLElement *xml_task = xml_doc->NewElement("task");
	XMLText *task_text = xml_doc->NewText(task_name.c_str());
	xml_task->InsertEndChild(task_text);
	xml_component->InsertEndChild(xml_task);
	
	XMLElement *xml_lib = xml_doc->NewElement("library");
	XMLText *lib_text = xml_doc->NewText(task_library.c_str());
	xml_lib->InsertEndChild(lib_text);
	xml_component->InsertEndChild(xml_lib);
	
	XMLElement *xml_libpath = xml_doc->NewElement("librarypath");
	XMLText *libpath_text = xml_doc->NewText(task_library_path.c_str());
	xml_libpath->InsertEndChild(libpath_text);
	xml_component->InsertEndChild(xml_libpath);
		
	// Inserting empty attributes
	std::cout << "Inserting attributes\n";
	XMLElement *xml_attributes = xml_doc->NewElement("attributes");
	xml_component->InsertEndChild(xml_attributes);
	for (auto &itr : task->getAttributesList()) {
		XMLElement *xml_attribute = xml_doc->NewElement("attribute");
		xml_attribute->SetAttribute("name", itr.c_str());
		xml_attribute->SetAttribute("value", "");
		xml_attributes->InsertEndChild(xml_attribute);
	}

	// Adding connections
	std::cout << "Inserting connections\n";
	XMLElement *xml_connections = xml_doc->NewElement("connections");
	xml_package->InsertEndChild(xml_connections);
	for(auto &itr : task->getPortsList()) {
		XMLElement *xml_connection = xml_doc->NewElement("connection");
		xml_connection->SetAttribute("data", "");
		xml_connection->SetAttribute("policy", "");
		xml_connection->SetAttribute("transport", "");
		xml_connection->SetAttribute("buffersize", "");
		XMLElement *xml_src = xml_doc->NewElement("src");
		XMLElement *xml_dest = xml_doc->NewElement("dest");
		if (task->getPort(itr)->isOutput()) {
			xml_src->SetAttribute("task", task_name.c_str());
			xml_src->SetAttribute("port", itr.c_str());
			xml_dest->SetAttribute("task", "");
			xml_dest->SetAttribute("port", "");
		} else {
			xml_src->SetAttribute("task", "");
			xml_src->SetAttribute("port", "");
			xml_dest->SetAttribute("task", task_name.c_str());
			xml_dest->SetAttribute("port", itr.c_str());
		}
		xml_connection->InsertEndChild(xml_src);
		xml_connection->InsertEndChild(xml_dest);
		xml_connections->InsertEndChild(xml_connection);
	}
	std::string out_xml_file = task_name + std::string(".xml");
	xml_doc->SaveFile(out_xml_file.c_str());
}

} // end of namespace

extern "C" 
{
	coco::ComponentRegistry ** getComponentRegistry() { return &singleton; }
}



/*
tinyxml2::XMLDocument *xml_doc = new tinyxml2::XMLDocument();
  tinyxml2::XMLElement *file_element = xml_doc->NewElement("File");
  xml_doc->InsertEndChild(file_element);

  tinyxml2::XMLElement *name_element = xml_doc->NewElement("Name");
  tinyxml2::XMLText* name_text = xml_doc->NewText(file_name);
  name_element->InsertEndChild(name_text);
  file_element->InsertEndChild(name_element);


  for(std::vector<Root *>::iterator root_itr = root_vect->begin(); root_itr != root_vect->end(); ++ root_itr)     
    (*root_itr)->CreateXMLFunction(xml_doc);
  

  std::string out_xml_file (file_name);
  size_t ext = out_xml_file.find_last_of(".");
  if (ext == std::string::npos)
    ext = out_xml_file.length();
  out_xml_file = out_xml_file.substr(0, ext);
  std::cout << out_xml_file << std::endl;
  
  out_xml_file.insert(ext, "_pragmas.xml");
  std::cout << out_xml_file << std::endl;

  xml_doc->SaveFile(out_xml_file.c_str());

  */