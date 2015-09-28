/**
 * Compact Framework Core
 * 2014-2015 Emanuele Ruffaldi and Filippo Brizzi @ Scuola Superiore Sant'Anna, Pisa, Italy
 */
#include <iostream>
#include <thread>
#include <chrono>

#include "coco_core.hpp"

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
//static coco::ComponentRegistry *singleton;

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

bool ConnectionBase::hasComponent(const std::string &name) const
{
    if (input_->taskName() == name)
        return true;
    if (output_->taskName() == name)
        return true;
    return false;
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

std::string PortBase::taskName() const
{
    return task_->instantiationName();
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
    if (task_)
    {
    	if (task_->state_ == RUNNING)
    	{
    		//processFunctions();
    		while (task_->hasPending())
			{
    			std::cout << "Execution function\n";
    			task_->stepPending();
			}
#ifdef PROFILING    		
    		util::Profiler log(task_->name());
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
	if (active_)
	{
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
	std::cout << "STOPPING ACTIVITY\n";
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
		COCO_LOG(10) << "Thread join";
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
	//addOperation("stop", &TaskContext::stop, this);
}

void TaskContext::start()
{
	if (activity_ == nullptr)
	{
		COCO_FATAL() << "Activity not found! Set an activity to start a component!";
		return;
	}
	if (state_ == RUNNING)
	{
		COCO_ERR() << "Task already running";
		return;
	}
	state_ = RUNNING;
	activity_->start();
}

void TaskContext::stop()
{
	std::cout << "TASK STOP\n";
	if (activity_ == nullptr)
	{
		COCO_ERR() << "Activity not found! Nothing to be stopped";
		return;
	}
	else if (!activity_->isActive())
	{
		COCO_ERR() << "Activity not running! Nothing to be stopped";
		return;
	}
	state_ = STOPPED;
	activity_->stop();
}

void TaskContext::triggerActivity() 
{
	activity_->trigger();
}

}

