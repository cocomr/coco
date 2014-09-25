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
#else

	#include <dlfcn.h>
	typedef void* dlhandle;

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


ComponentSpec::ComponentSpec(const char * name, makefx_t fx)
	: name_(name),fx_(fx)
{
	std::cout << "registering component " << name << std::endl;
	ComponentRegistry::addSpec(this);
}

ComponentRegistry & ComponentRegistry::get() {
	ComponentRegistry * a = singleton;
	if(!a)
	{
		singleton = new ComponentRegistry();
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
bool ComponentRegistry::addLibrary(const char * l) {
	return get().addLibrary_(l);
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
	specs[s->name_] = s;
}

bool ComponentRegistry::addLibrary_(const char * path) {
	std::string p = DLLPREFIX + std::string(path) + DLLEXT;


	if(libs.find(p) != libs.end())
		return true; // already loaded

	// TODO: OS specific shared library extensions: so dylib dll
	typedef ComponentRegistry ** (*pfx_t)();
	dlhandle l = dlopen(p.c_str(),RTLD_NOW);
	if(!l)
		return false;		
	pfx_t pfx = (pfx_t)dlsym(l,"getComponentRegistry");
	if(!pfx)
		return false;
	ComponentRegistry ** other = pfx();
	if(!*other)
		*other = this;
	else if(*other != this)
	{
		// import the specs and the destroy the imported registry and replace it inside the shared library
		for(auto&& i : (*other)->specs) {
			specs[i.first] = i.second;
		}		
		delete *other;
		*other = this;
	}
	libs.insert(p);
	return true;
}



/// --------------------------------

PropertyBase::PropertyBase(TaskContext * p, const char * name)
	: name_(name) {
	p->self_props_.push_back(this);
}

AttributeBase::AttributeBase(TaskContext * p, const char * name)
	: name_(name) {
	p->attributes_.push_back(this);
}


// -------------------------------------------------------------------
// Connection
// -------------------------------------------------------------------

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
	stopping_ = false;
	active_ = true;
	if(thread_)
		return;
	thread_ = std::move(std::unique_ptr<std::thread>(new std::thread(&ParallelActivity::entry,this)));
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
			std::unique_lock<std::mutex> mlock(mutex_t_);
			cond_.wait(mlock);
			if(stopping_)
				break;
			else
				runnable_->step();
		}
	}
	std::cout << "FINALIZE\n";
	runnable_->finalize();
}

const PortBase *Service::getPort(std::string name) 
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

TaskContext::TaskContext() {
	engine_ = std::make_shared<ExecutionEngine>(this);
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

void TaskContext::init() {
	if (activity_ == nullptr) {
		std::cout << "Activity not found! Set an activity to start a component!\n";
		return;
	}
	if (state_ == RUNNING) {
		std::cout << "Task already running\n";
		return;
	}
	activity_->init();
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

} // end of namespace

extern "C" 
{
	coco::ComponentRegistry ** getComponentRegistry() { return &singleton; }
}
