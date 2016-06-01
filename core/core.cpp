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

#include "core.h"
#include "core_impl.hpp"
#include "register.h"
#include "util/timing.h"
#include "util/logging.h"

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
ExecutionEngine::ExecutionEngine(std::shared_ptr<TaskContext> task)
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

ConnectionBase::ConnectionBase(std::shared_ptr<PortBase> in,
                               std::shared_ptr<PortBase> out,
                               ConnectionPolicy policy)
    : input_(in), output_(out),
      data_status_(NO_DATA), policy_(policy)
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

ConnectionManager::ConnectionManager()
    : rr_index_(0)
{
}

bool ConnectionManager::addConnection(std::shared_ptr<ConnectionBase> connection) 
{
	connections_.push_back(connection);
	return true;
}

bool ConnectionManager::hasConnections() const
{
    return connections_.size() != 0;
}

int ConnectionManager::queueLenght(int connection) const
{
    if (connection > (int)connections_.size())
        return 0;
    if (connection >= 0)
        return connections_[connection]->queueLength();

    unsigned int lenght = 0;
    for (auto & conn : connections_)
    {
        lenght += conn->queueLength();
    }
    return lenght;
}

//std::shared_ptr<ConnectionBase> ConnectionManager::connection(unsigned int index)
//{
//	if (index < connections_.size())
//		return connections_[index];
//	else
//		return nullptr;
//}

//const std::shared_ptr<ConnectionBase> ConnectionManager::connection(unsigned int index) const
//{
//    if (index < connections_.size())
//        return connections_[index];
//    else
//        return nullptr;
//}

//std::shared_ptr<ConnectionBase> ConnectionManager::connection(const std::string &name)
//{
//    for (auto conn : connections_)
//    {
//        if (conn->hasComponent(name))
//            return conn;
//    }
//    return nullptr;
//}

int ConnectionManager::connectionsCount() const
{ 
	return connections_.size();
}

// -------------------------------------------------------------------
// Attribute
// -------------------------------------------------------------------

AttributeBase::AttributeBase(TaskContext *task,
                             const std::string &name)
	: name_(name) 
{
    std::shared_ptr<AttributeBase> shared_this(this);
    task->addAttribute(shared_this);
}

// -------------------------------------------------------------------
// Operation
// -------------------------------------------------------------------

OperationBase::OperationBase(TaskContext *task,
                             const std::string &name)
    : name_(name)
{
    std::shared_ptr<OperationBase> shared_this(this);
    task->addOperation(shared_this);
}

OperationInvocation::OperationInvocation(const std::function<void(void)> & f)
	: fx(f)
{

}

// -------------------------------------------------------------------
// Port
// -------------------------------------------------------------------

PortBase::PortBase(TaskContext *task, const std::string &name,
				   bool is_output, bool is_event)
    : task_(task), name_(name), is_output_(is_output), is_event_(is_event)
{
    std::shared_ptr<PortBase> shared_this(this);
    task->addPort(shared_this);
}

bool PortBase::isConnected() const
{ 
    return manager_->hasConnections();
}	

unsigned int PortBase::connectionsCount() const
{
    return manager_->connectionsCount();
}

unsigned int PortBase::queueLength(int connection) const
{
    return manager_->queueLenght();
}

void PortBase::triggerComponent()
{
    task_->triggerActivity(this->name_);
}

void PortBase::removeTriggerComponent()
{
	task_->removeTriggerActivity();
}

bool PortBase::addConnection(std::shared_ptr<ConnectionBase> &connection)
{
    if (!is_output_ && is_event_)
    {
        task_->addEventPort(name_);
    }

    manager_->addConnection(connection);
	return true;
}

// -------------------------------------------------------------------
// Service
// -------------------------------------------------------------------

Service::Service(const std::string &name)
	: name_(name)
{

}

bool Service::addAttribute(std::shared_ptr<AttributeBase> &attribute)
{
	if (attributes_[attribute->name()])
	{
		COCO_ERR() << "An attribute with name: " << attribute->name() << " already exist\n";
		return false;
	}
    attributes_[attribute->name()] = attribute;
	return true;
}

std::shared_ptr<AttributeBase> Service::attribute(const std::string &name)
{
    auto it = attributes_.find(name);
	if(it == attributes_.end())
    {
        return nullptr;
    }
    else
    {
        return it->second;
    }
}

bool Service::addPort(std::shared_ptr<PortBase> &port)
{
    if (ports_[port->name()])
    {
        COCO_ERR() << instantiationName() <<  ": A port with name: " << port->name() << " already exist";
		return false;
	}
	else
	{
		ports_[port->name()] = port;
		return true;
	}
}

std::shared_ptr<PortBase> Service::port(const std::string &name)
{
	auto it = ports_.find(name);
	if(it == ports_.end())
			return nullptr;
		else
			return it->second;
}

bool Service::addOperation(std::shared_ptr<OperationBase> &operation)
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

void Service::addPeer(std::shared_ptr<TaskContext> & peer)
{
	peers_.push_back(peer);
}

//const std::unique_ptr<Service> &Service::provides(const std::string &name)
//{
//	auto it = subservices_.find(name);
//	if(it == subservices_.end())
//	{
//        std::unique_ptr<Service> service(new Service(name));
//        subservices_[name] = std::move(service);
//        return subservices_[name];
//	}
//	else
//        return it->second;
//	return nullptr;
//}

TaskContext::TaskContext()
{
    state_ = IDLE;
    std::unique_ptr<AttributeBase> attribute(new Attribute<bool>(this, "wait_all_trigger", wait_all_trigger_));
    att_wait_all_trigger_.swap(attribute);
}

void TaskContext::stop()
{

}

bool TaskContext::isOnSameThread(const std::shared_ptr<TaskContext> &other) const
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

