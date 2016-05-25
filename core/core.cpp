/*
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

#include <iostream>
#include <thread>
#include <chrono>

#include "core.h"
#include "core_impl.hpp"
#include "register.h"
#include "util/timing.h"

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
		#include <pthread.h>
		#define DLLEXT ".so"
		#define DLLPREFIX "lib"
	#endif

#endif

namespace coco
{
// -------------------------------------------------------------------
// Activity
// -------------------------------------------------------------------

Activity::Activity(SchedulePolicy policy)
    : policy_(policy), active_(false), stopping_(false)
{}

bool Activity::isPeriodic() const
{ 
	return policy_.scheduling_policy != SchedulePolicy::TRIGGERED;
}

SequentialActivity::SequentialActivity(SchedulePolicy policy)
	: Activity(policy)
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
	return;
}

std::thread::id SequentialActivity::threadId() const
{
    return std::thread::id();
}


void SequentialActivity::entry()
{
	for (auto &runnable : runnable_list_)
		runnable->init();
	// PERIODIC
	if(isPeriodic())
	{
		std::chrono::system_clock::time_point next_start_time;
		while(!stopping_)
		{
			next_start_time = std::chrono::system_clock::now() + std::chrono::milliseconds(policy_.period_ms);
			for (auto &runnable : runnable_list_)
				runnable->step();
			std::this_thread::sleep_until(next_start_time); // NOT interruptible, limit of std::thread
		}
	}
	// TRIGGERED
	else
	{
		while(true)
		{
			// wait on condition variable or timer
			if(stopping_)
			{
				break;
			}
			else
			{
				for (auto &runnable : runnable_list_)
					runnable->step();
			}
		}
	}
	active_ = false;

	for (auto &runnable : runnable_list_)
		runnable->finalize();

}

ParallelActivity::ParallelActivity(SchedulePolicy policy)
	: Activity(policy)
{}

void ParallelActivity::start() 
{
	if(thread_)
		return;
	stopping_ = false;
	active_ = true;
	thread_ = std::move(std::unique_ptr<std::thread>(new std::thread(&ParallelActivity::entry,this)));

#ifdef __linux__
	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);
	if (policy_.affinity >= 0)
	{	
		CPU_SET(policy_.affinity, &cpu_set);
	}
	else
	{
		for (auto i : policy_.available_core_id)
			CPU_SET(i, &cpu_set);
	}
	int res = pthread_setaffinity_np(thread_->native_handle(), sizeof(cpu_set_t), &cpu_set);
	if (res != 0)
	{
		COCO_FATAL() << "Failed to set affinity on core: " << policy_.affinity
					 << " with error: " << res << std::endl;
	}
#elif __APPLE__

#elif WIN

#endif
	
}

void ParallelActivity::stop() 
{
	COCO_DEBUG("Activity") << "STOPPING ACTIVITY";
	if(thread_)
	{		
		stopping_ = true;
		cond_.notify_all();
	}
}

void ParallelActivity::trigger()
{
	++pending_trigger_;
	cond_.notify_all();
}

void ParallelActivity::removeTrigger()
{
	if (pending_trigger_ > 0)
	{
		--pending_trigger_;
	}
}

void ParallelActivity::join()
{
	if(thread_)
	{
		if (thread_->joinable())
		{
			thread_->join();
		}
	}
}

std::thread::id ParallelActivity::threadId() const
{
    return thread_->get_id();
}

void ParallelActivity::entry()
{
	for (auto &runnable : runnable_list_)
		runnable->init();

	// PERIODIC
	if(isPeriodic())
	{
		std::chrono::system_clock::time_point next_start_time;
		while(!stopping_)
		{
			auto now = std::chrono::system_clock::now();
			for (auto &runnable : runnable_list_)
				runnable->step();
			
			auto new_now = std::chrono::system_clock::now();
			//auto sleep_time = std::chrono::milliseconds(policy_.period_ms) - (new_now - now);
			auto sleep_time = std::chrono::microseconds(policy_.period_ms * 1000) - (new_now - now);
			if (sleep_time > std::chrono::microseconds(0))
			{
				std::unique_lock<std::mutex> mlock(mutex_);
	        	cond_.wait_for(mlock, sleep_time);
        	}
		}
	}
	// TRIGGERED
	else
	{
		while(true)
		{
			// wait on condition variable or timer
			{
				if(pending_trigger_ == 0)
				{
					std::unique_lock<std::mutex> mlock(mutex_);
					cond_.wait(mlock);
				}
			}
			if(stopping_)
			{
				break;
			}

            for (auto &runnable : runnable_list_)
                runnable->step();
		}
	}
	active_ = false;
	for (auto &runnable : runnable_list_)
		runnable->finalize();
}

// -------------------------------------------------------------------
// Execution
// -------------------------------------------------------------------
ExecutionEngine::ExecutionEngine(TaskContext *task)
    : task_(task)//, profiling_(profiling)
{

}

void ExecutionEngine::init()
{
	task_->setState(INIT);
	task_->onConfig();
	COCO_DEBUG("Execution") << "[" << task_->instantiationName() << "] onConfig completed.";
	coco::ComponentRegistry::increaseConfigCompleted();
	task_->setState(IDLE);
}

void ExecutionEngine::step()
{

	//processMessages();
    //processFunctions();
    if (task_)
    {
		while (task_->hasPending())
		{
			task_->setState(PRE_OPERATIONAL);
			task_->stepPending();
		}
		task_->setState(RUNNING);
        //if (profiling_)
        if (ComponentRegistry::profilingEnabled())
		{
			COCO_START_TIMER(task_->instantiationName())
			task_->onUpdate();
			COCO_STOP_TIMER(task_->instantiationName())
		}
		else
		{
			task_->onUpdate();
		}
		task_->setState(IDLE);
    }
}

void ExecutionEngine::finalize()
{
	if (task_->state() != STOPPED) 
		task_->stop();
}

// -------------------------------------------------------------------
// Connection
// -------------------------------------------------------------------

ConnectionPolicy::ConnectionPolicy() 
	: data_policy(DATA), lock_policy(LOCKED), 
	  buffer_size(1), init(false), transport(LOCAL)
{

}

ConnectionPolicy::ConnectionPolicy(const std::string & policy,
								   const std::string & lock,
								   const std::string & transport_type,
								   const std::string & size)
{
	// look here http://tinodidriksen.com/2010/02/16/cpp-convert-string-to-int-speed/
	buffer_size = atoi(size.c_str()); // boost::lexical_cast<int>(buffer_size);
	if (policy.compare("DATA") == 0)
		data_policy = DATA;
	else if (policy.compare("BUFFER") == 0)
		data_policy = BUFFER;
	else if (policy.compare("CIRCULAR") == 0)
		data_policy = CIRCULAR;
	else
		COCO_FATAL() << "Failed to parse connection policy data type: " << policy;
	if (lock.compare("UNSYNC") == 0)
		lock_policy = UNSYNC;
	else if (lock.compare("LOCKED") == 0)
		lock_policy = LOCKED;
	else if (lock.compare("LOCK_FREE") == 0)
		lock_policy = LOCK_FREE;
	else
		COCO_FATAL() << "Failed to parse connection policy lock type: " << lock;
	if (transport_type.compare("LOCAL") == 0)
		transport = LOCAL;
	else if (transport_type.compare("IPC") == 0)
		transport = IPC;
	else
		COCO_FATAL() << "Failed to parse connection policy transport type: " << transport_type;
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
    if (input_->task()->instantiationName() == name)
        return true;
    if (output_->task()->instantiationName() == name)
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

std::shared_ptr<ConnectionBase> ConnectionManager::connection(unsigned int index)
{
	if (index < connections_.size()) 
		return connections_[index];
	else
		return nullptr;
}

const std::shared_ptr<ConnectionBase> ConnectionManager::connection(unsigned int index) const
{
    if (index < connections_.size())
        return connections_[index];
    else
        return nullptr;
}

std::shared_ptr<ConnectionBase> ConnectionManager::connection(const std::string &name)
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

AttributeBase::AttributeBase(TaskContext * task, const std::string &name)
	: name_(name) 
{
	task->addAttribute(this);
}

// -------------------------------------------------------------------
// Operation
// -------------------------------------------------------------------

OperationBase::OperationBase(Service * service, const std::string &name) 
	: name_(name)
{
	service->addOperation(this);
}

OperationInvocation::OperationInvocation(const std::function<void(void)> & f)
	: fx(f)
{

}

// -------------------------------------------------------------------
// Port
// -------------------------------------------------------------------

PortBase::PortBase(TaskContext * task, const std::string &name,
				   bool is_output, bool is_event)
	: task_(task), name_(name), is_output_(is_output), is_event_(is_event)
{
	task_->addPort(this);
}

bool PortBase::isConnected() const
{ 
	return manager_.hasConnections();
}	

unsigned int PortBase::connectionsCount() const
{
    return manager_.connectionsSize();
}

unsigned int PortBase::queueLength(int connection) const
{
    if (connection >= 0)
        return manager_.connection(connection)->queueLength();

    auto connections = manager_.connections();
    unsigned int lenght = 0;
    for (auto & conn : connections)
    {
        lenght += conn->queueLength();
    }
    return lenght;
}

void PortBase::triggerComponent()
{
    task_->triggerActivity(this->name_);
}

void PortBase::removeTriggerComponent()
{
	task_->removeTriggerActivity();
}

bool PortBase::addConnection(std::shared_ptr<ConnectionBase> connection) 
{
    if (!is_output_ && is_event_)
    {
        task_->addEventPort(name_);
    }

    manager_.addConnection(connection);
	return true;
}

// -------------------------------------------------------------------
// Service
// -------------------------------------------------------------------

Service::Service(const std::string &name)
	: name_(name)
{

}

bool Service::addAttribute(AttributeBase *attribute)
{
	if (attributes_[attribute->name()])
	{
		COCO_ERR() << "An attribute with name: " << attribute->name() << " already exist\n";
		return false;
	}
	attributes_[attribute->name()] = attribute;
	return true;
}

AttributeBase *Service::attribute(std::string name) 
{
	auto it = attributes_.find(name);
	if(it == attributes_.end())
			return nullptr;
		else
			return it->second;
}

bool Service::addPort(PortBase *port)
{
	if (ports_[port->name()]) {
		std::cerr << "A port with name: " << port->name() << " already exist\n";	
		return false;
	}
	else
	{
		ports_[port->name()] = port;
		return true;
	}
}

PortBase *Service::port(std::string name) 
{
	auto it = ports_.find(name);
	if(it == ports_.end())
			return nullptr;
		else
			return it->second;
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

void Service::addPeer(TaskContext *peer)
{
	peers_.push_back(peer);
}

Service * Service::provides(const std::string &name)
{
	auto it = subservices_.find(name);
	if(it == subservices_.end())
	{
		Service * p = new Service(name);
		subservices_[name] = std::unique_ptr<Service>(p);
		return p;
	}
	else
		return it->second.get();
	return nullptr;
}

TaskContext::TaskContext()
{
	activity_ = nullptr;
    state_ = IDLE;
    att_wait_all_trigger_ = new Attribute<bool>(this, "wait_all_trigger", wait_all_trigger_);
}

void TaskContext::stop()
{

}

bool TaskContext::isOnSameThread(const TaskContext * other) const
{
    return this->activity_->threadId() == other->activity_->threadId();
}

void TaskContext::triggerActivity(const std::string &port_name)
{
    if (!wait_all_trigger_)
    {
        activity_->trigger();
        return;
    }
    /* Check wheter all the ports have been triggered.
     * This is done checking the size of the unordered_set.
     * If size equal total number of ports, trigger.
     * To avoid emptying the set, the check is reversed and items are removed
     * till size is zero and activity triggered.
     */
    if (forward_check_)
    {
        event_ports_.insert(port_name);
        if (event_ports_.size() == event_port_num_)
        {
            activity_->trigger();
            forward_check_ = false;
        }
    }
    else
    {
        event_ports_.erase(port_name);
        if (event_ports_.size() == 0)
        {
            activity_->trigger();
            forward_check_ = true;
        }
    }
}

void TaskContext::removeTriggerActivity() 
{
	activity_->removeTrigger();
}

}

