/**
 * Compact Framework Core
 * 2014-2015 Emanuele Ruffaldi and Filippo Brizzi @ Scuola Superiore Sant'Anna, Pisa, Italy
 */
#include <iostream>
#include <thread>
#include <chrono>

#include "core.h"

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
// -------------------------------------------------------------------
// Activity
// -------------------------------------------------------------------

Activity::Activity(SchedulePolicy policy, std::shared_ptr<RunnableInterface> r)
    : runnable_(r), policy_(policy), active_(false), stopping_(false)
{

}

bool Activity::isPeriodic()
{ 
	return policy_.timing_policy != SchedulePolicy::TRIGGERED;
}

SequentialActivity::SequentialActivity(SchedulePolicy policy, 
									   std::shared_ptr<RunnableInterface> r)
	: Activity(policy, r)
{}

void SequentialActivity::start() 
{
	this->entry();
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

void SequentialActivity::trigger()
{

}

void SequentialActivity::removeTrigger()
{

}

void SequentialActivity::join()
{
	while(true);
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
			nextStartTime =	currentStartTime + std::chrono::milliseconds(policy_.period_ms);
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

ParallelActivity::ParallelActivity(SchedulePolicy policy,
								   std::shared_ptr<RunnableInterface> r)
	: Activity(policy, r)
{

}

void ParallelActivity::start() 
{
	if(thread_)
		return;
	stopping_ = false;
	active_ = true;
	thread_ = std::move(std::unique_ptr<std::thread>(new std::thread(&ParallelActivity::entry,this)));
}

void ParallelActivity::stop() 
{
	COCO_LOG(1) << "STOPPING ACTIVITY";
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
	// TODO cannot use pending_trigger_++
	// to do so we should decrease the trigger when reading the port
	++pending_trigger_;
	cond_.notify_one();
}

void ParallelActivity::removeTrigger()
{
	std::unique_lock<std::mutex> mlock(mutex_);
	if (pending_trigger_ > 0)
	{
		--pending_trigger_;
	}
}

void ParallelActivity::join()
{
	if(thread_)
	thread_->join();
}

void ParallelActivity::entry()
{
	runnable_->init();
	if(isPeriodic())
	{
		std::chrono::system_clock::time_point currentStartTime{ std::chrono::system_clock::now() };
		std::chrono::system_clock::time_point next_start_time{ currentStartTime };
		while(!stopping_)
		{
			//currentStartTime = std::chrono::system_clock::now();
			next_start_time = std::chrono::system_clock::now() + std::chrono::milliseconds(policy_.period_ms);
			runnable_->step();
			//next_start_time = currentStartTime + std::chrono::milliseconds(policy_.period_ms_);
			std::this_thread::sleep_until(next_start_time); // NOT interruptible, limit of std::thread
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
				{
					cond_.wait(mlock);
					//pending_trigger_ = 0;	
				}
			}
			if(stopping_)
				break;
			else
				runnable_->step();
		}
	}
	runnable_->finalize();
}

Activity * createSequentialActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r = 0)
{
	return new SequentialActivity(sp, r);
}

Activity * createParallelActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r = 0) 
{
	return new ParallelActivity(sp, r);
}

// -------------------------------------------------------------------
// Execution
// -------------------------------------------------------------------
ExecutionEngine::ExecutionEngine(TaskContext *t) 
	: task_(t)
{

}

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
    		// processing enqueued operation;
    		while (task_->hasPending())
			{
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

// -------------------------------------------------------------------
// Connection
// -------------------------------------------------------------------

ConnectionPolicy::ConnectionPolicy() 
	: data_policy(DATA), lock_policy(LOCKED), 
	  buffer_size(1), init(false)
{

}
ConnectionPolicy::ConnectionPolicy(Policy policiy, int buffer_size)
	: data_policy(policiy), lock_policy(LOCKED), 
	  buffer_size(buffer_size), init(false)
{

}

ConnectionPolicy::ConnectionPolicy(const std::string &policy,
								   const std::string &lock,
								   const std::string &transport_type,
								   const std::string &size)
{
	// look here http://tinodidriksen.com/2010/02/16/cpp-convert-string-to-int-speed/
	buffer_size = atoi(size.c_str()); // boost::lexical_cast<int>(buffer_size);
	if (policy.compare("DATA") == 0)
		data_policy = DATA;
	else if (policy.compare("BUFFER") == 0)
		data_policy = BUFFER;
	else if (policy.compare("CIRCULAR") == 0)
		data_policy = CIRCULAR;
	if (lock.compare("UNSYNC") == 0)
		lock_policy = UNSYNC;
	else if (lock.compare("LOCKED") == 0)
		lock_policy = LOCKED;
	else if (lock.compare("LOCK_FREE") == 0)
		lock_policy = LOCK_FREE;
	if (transport_type.compare("LOCAL") == 0)
		transport = LOCAL;
	else if (transport_type.compare("IPC") == 0)
		transport = IPC;
}

ConnectionBase::ConnectionBase(PortBase *in, PortBase *out, ConnectionPolicy policy)
    : data_status_(NO_DATA), policy_(policy), input_(in), output_(out)
{

}

bool ConnectionBase::hasNewData() const
{
	return data_status_ == NEW_DATA;
}

bool ConnectionBase::hasComponent(const std::string &name) const
{
    if (input_->taskName() == name)
        return true;
    if (output_->taskName() == name)
        return true;
    return false;
}

void ConnectionBase::trigger()
{
	input_->triggerComponent();
}

void ConnectionBase::removeTrigger()
{
	input_->removeTriggerComponent();
}

ConnectionManager::ConnectionManager(PortBase * port)
	: rr_index_(0), owner_(port)
{

}

bool ConnectionManager::addConnection(std::shared_ptr<ConnectionBase> connection) 
{
	connections_.push_back(connection);
	return true;
}

bool ConnectionManager::hasConnections() const
{
	return connections_.size()  != 0;
}

std::shared_ptr<ConnectionBase> ConnectionManager::getConnection(unsigned int index)
{
	if (index < connections_.size()) 
		return connections_[index];
	else
		return nullptr;
}

std::shared_ptr<ConnectionBase> ConnectionManager::getConnection(const std::string &name)
{
    for (auto conn : connections_)
    {
        if (conn->hasComponent(name))
            return conn;
    }
    return nullptr;
}

int ConnectionManager::connectionsSize() const
{ 
	return connections_.size();
}

// -------------------------------------------------------------------
// Attribute
// -------------------------------------------------------------------

AttributeBase::AttributeBase(TaskContext * p, const std::string &name)
	: name_(name) 
{
	p->addAttribute(this);
}

// -------------------------------------------------------------------
// Operation
// -------------------------------------------------------------------

OperationBase::OperationBase(Service * p, const std::string &name) 
	: name_(name)
{
	p->addOperation(this);
}

OperationInvocation::OperationInvocation(const std::function<void(void)> & f)
	: fx(f)
{

}

// -------------------------------------------------------------------
// Port
// -------------------------------------------------------------------

PortBase::PortBase(TaskContext * p, const std::string &name,
				   bool is_output, bool is_event)
	: task_(p), name_(name), is_output_(is_output), is_event_(is_event)
{
	task_->addPort(this);
}

bool PortBase::isConnected() const
{ 
	return manager_.hasConnections();
}	

int PortBase::connectionsCount() const
{
	return manager_.connectionsSize();
}

std::string PortBase::taskName() const
{
    return task_->instantiationName();
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

void PortBase::removeTriggerComponent()
{
	task_->removeTriggerActivity();
}
// -------------------------------------------------------------------
// Service
// -------------------------------------------------------------------

Service::Service(const std::string &n)
	: name_(n)
{

}

bool Service::addAttribute(AttributeBase *a)
{
	if (attributes_[a->name()])
	{
		COCO_ERR() << "An attribute with name: " << a->name() << " already exist\n";
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

coco::impl::map_keys<std::string,AttributeBase*> Service::getAttributeNames()
{ 
	return coco::impl::make_map_keys(attributes_);
}

coco::impl::map_values<std::string,AttributeBase*> Service::getAttributes()
{ 
	return coco::impl::make_map_values(attributes_);
}

bool Service::addPort(PortBase *p)
{
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

coco::impl::map_keys<std::string,PortBase*> Service::getPortNames()
{ 
	return coco::impl::make_map_keys(ports_);
}

coco::impl::map_values<std::string,PortBase*> Service::getPorts()
{
	return coco::impl::make_map_values(ports_);
}

bool Service::addOperation(OperationBase *operation)
{
	if (operations_[operation->name()])
	{
		COCO_ERR() << "An operation with name: " << operation->name() << " already exist";
		return false;
	}
	operations_[operation->name()] = operation;
	return true;
}

coco::impl::map_keys<std::string, OperationBase*> Service::getOperationNames()
{
	return coco::impl::make_map_keys(operations_);
}

coco::impl::map_values<std::string, OperationBase*> Service::getOperations()
{
	return coco::impl::make_map_values(operations_);
}

bool Service::hasPending() const
{ 
	return !asked_ops_.empty();
}

void Service::stepPending()
{
	auto op = asked_ops_.front();
	asked_ops_.pop_front();
	op.fx();
}

bool Service::addPeer(TaskContext *p)
{
	peers_.push_back(p);
	return true;
}

coco::impl::map_keys<std::string,std::unique_ptr<Service> > Service::getServiceNames()
{ 
	return coco::impl::make_map_keys(subservices_);
}
coco::impl::map_values<std::string,std::unique_ptr<Service> > Service::getServices()
{ 
	return coco::impl::make_map_values(subservices_);
}

Service * Service::provides(const std::string &x)
{
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
{
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

void TaskContext::removeTriggerActivity() 
{
	activity_->removeTrigger();
}

}

