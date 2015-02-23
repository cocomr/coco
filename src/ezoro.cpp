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
	: task_(p), name_(p->name() + "_" + name), is_output_(is_output), is_event_(is_event)
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
#ifdef PROFILING    		
    		Profiler log(task_->name());
#endif
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

} // end of namespace

extern "C" 
{
	coco::ComponentRegistry ** getComponentRegistry() { return &singleton; }
}

