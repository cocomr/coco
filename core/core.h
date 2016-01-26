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

#pragma once
#include <vector>
#include <unordered_map>
#include <list>
#include <memory>
#include <cstddef>
#include <string>
#include <map>
#include <set>
#include <mutex>
#include <thread>
#include <functional>
#include <typeinfo>
#include <iostream>
#include <fstream>
#include <queue>
#include <memory>
#include <iostream>
#include <deque>
#include <string>
#include <atomic>
#include <condition_variable>
#include <type_traits>
#include <typeinfo>
#include <boost/lexical_cast.hpp>
//#include <boost/lockfree/queue.hpp>

// workaround for std::mutex in MINGW
#ifdef WIN32
#include "mingw-std-threads/mingw.thread.h"
#include <mutex>
#include "mingw-std-threads/mingw.mutex.h"
#include "mingw-std-threads/mingw.condition_variable.h"
#else
#include <mutex>
#endif
#include <atomic>
#include "impl.hpp"
#include "util/logging.h"

namespace coco
{
class Activity;
class ParallelActivity;
class SequentialActivity;
class RunnableInterface;
class ExecutionEngine;
class AttributeBase;
class OperationBase;
class PortBase;
class Service;
class TaskContext;
class PeerTask;
struct ConnectionPolicy;
class ConnectionBase;
class ConnectionManager;


/// policy for the execution of operations
enum ThreadSpace { OWN_THREAD, CLIENT_THREAD};

/// Policy for executing the component
struct SchedulePolicy 
{
	/// A policy can be:
	/// Periodic: execute periodically with a given period.
	/// Triggered: its execution is triggered when the port designated as event receives data.	
	enum Policy { PERIODIC, HARD, TRIGGERED };

	// missing containment inside other container: require standalone thread
	Policy timing_policy = PERIODIC;
	int period_ms;
	std::string trigger; // trigger port
	int affinity = -1;
	std::list<unsigned int> available_core_id;

	SchedulePolicy(Policy policy = PERIODIC, int period = 1)
		: timing_policy(policy), period_ms(period) {}
};

/// Base class for something that loops or is activated
class Activity
{
public:
	/// Specify the execution policy and the RunnableInterface to be executed
	Activity(SchedulePolicy policy);// std::vector<std::shared_ptr<RunnableInterface> > r_list = 
									//	std::vector<std::shared_ptr<RunnableInterface> >());
	/// Start the activity
	virtual void start() = 0;
	/// Stop the activity
	virtual void stop() = 0;
	/// In case of a TRIGGER activity it increases the trigger counter and wake up the activiy
	virtual void trigger() = 0;
	/// When data from an event port is read decreases the trigger counter
	virtual void removeTrigger() = 0;
	/// Main execution function that contains the real execution 
	virtual void entry() = 0;
	/// Wait for the thread to complete execution
	virtual void join() = 0;
	/// Return if the activity is periodic or not
	bool isPeriodic() const;
	/// Return the period in millisecond of the activity
	int period() const { return policy_.period_ms; }
	/// Return if the activity is running
	bool isActive() const { return active_; };
	/// Return the schedule policy type: PERIODIC, TRIGGERED
	//SchedulePolicy::Policy policyType() const { return policy_.timing_policy; }
	/// Return the SchedulePolicy associated with this activity
	SchedulePolicy & policy() { return policy_; }
	const SchedulePolicy & policy() const { return policy_; }
	/// Add a RunnableInterface to the list of the one to be executed
	void addRunnable(const std::shared_ptr<RunnableInterface> &runnable) { runnable_list_.push_back(runnable); }
	/// Return the list of runnables associated with this activity
	const std::list<std::shared_ptr<RunnableInterface> > &runnables () const { return runnable_list_; }
protected:
	std::list<std::shared_ptr<RunnableInterface> > runnable_list_;
	SchedulePolicy policy_;
	bool active_;
	std::atomic<bool> stopping_;
};

/// Create an activity that will run in the same thread of the caller
class SequentialActivity: public Activity
{
public:
	SequentialActivity(SchedulePolicy policy);
	virtual void start() override;
	virtual void stop() override;
	virtual void trigger() override;
	virtual void removeTrigger() override;
	virtual void join() override;
protected:
	virtual void entry() override;
}; 

/// Activity that run in its own thread
class ParallelActivity: public Activity
{
public:
	ParallelActivity(SchedulePolicy policy);

	virtual void start() override;
	virtual void stop() override;
	virtual void trigger() override;
	virtual void removeTrigger() override;
	virtual void join() override;
protected:
	virtual void entry() override;

	std::atomic<int> pending_trigger_ = {0};
	std::unique_ptr<std::thread> thread_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

/// Interface class to execute the components 
class RunnableInterface
{
public:
	/// Initialize the components members
	virtual void init() = 0;
	/// If the task is running execute uno step of the execution function
	virtual void step() = 0;
	/// When the task is stopped clear all the members
	virtual void finalize() = 0;
protected:
};

/// Concrete class to execture the components  
class ExecutionEngine: public RunnableInterface
{
public:
	/// Constructor to set the current TaskContext to be executed
	ExecutionEngine(TaskContext *t, bool profiling);
	virtual void init() override;
	virtual void step() override;
	virtual void finalize() override;
	/// Return the task associated with this Engine
	const TaskContext * task() const { return task_; }
private:
	TaskContext *task_;
	bool stopped_;
	bool profiling_ = false;
};

/**
 * Connection Policy
 */
struct ConnectionPolicy 
{
	/// Specify the type of data container: single slot, buffer or circular buffer
	enum Policy { DATA, BUFFER, CIRCULAR };
	enum LockPolicy { UNSYNC, LOCKED, LOCK_FREE };
	enum Transport { LOCAL, IPC };

	Policy data_policy; /// type of data container
	LockPolicy lock_policy; /// type of lock policy
	int buffer_size; /// size of the data container
	bool init = false; 
	std::string name_id;
	Transport transport = LOCAL;

	ConnectionPolicy();
    ConnectionPolicy(Policy policiy, int buffer_size);
	/** Default constructor, default value:
	 *  @param data_policy = DATA
	 *	@param lock_policy = LOCKED
	 *  @param buffer_size = 1
	 * 	@param init = 1
	 */
	ConnectionPolicy(const std::string &policy, const std::string &lock,
				     const std::string &transport_type, const std::string &buffer_size);
};

#undef NO_DATA
/// status of a connection port
enum FlowStatus { NO_DATA, OLD_DATA, NEW_DATA };

/**
 * Base class for connections 
 */
class ConnectionBase
{
public:
	/// Constructor
	ConnectionBase(PortBase *in, PortBase *out, ConnectionPolicy policy);
	virtual ~ConnectionBase() {}

	/// @return NEW_DATA if new data is present in the Input port
	bool hasNewData() const;
	/// return true if one of the task involved in the connection has name \param name
    bool hasComponent(const std::string &name) const;
    /// return the input port pointer
    const PortBase * input() const { return input_; }
    /// return the output port pointer
    const PortBase * output() const { return output_; }
protected:
	/// Trigger the port to communicate new data is present
	void trigger();
	/// Remove the trigger once the data is read
	void removeTrigger();

	FlowStatus data_status_; /// status of the data in the container
	ConnectionPolicy policy_; /// policy for data management
	PortBase *input_ = 0; /// input port untyped
	PortBase *output_ = 0; /// output port untyped
};


/// Manage the connections of one PortBase
class ConnectionManager 
{
public:
	/// Set the RounRobin index to 0 and set its PortBase ptr owner
	ConnectionManager(PortBase * port);
	/// Add a connection to \p connections_
	bool addConnection(std::shared_ptr<ConnectionBase> connection);
	/// Return true if \p connections_ has at list one elemnt
	bool hasConnections() const;	
	/// Return the ConnectionBase connection inidicated by index if it exist
    std::shared_ptr<ConnectionBase> connection(unsigned int index);
    /// Return the ConnectionBase connection inidicated by name if it exist
    std::shared_ptr<ConnectionBase> connection(const std::string &name);
    /// Return the list of connections
    const std::vector<std::shared_ptr<ConnectionBase>> & connections() const { return connections_; }
	/// Return the number of connections
	int connectionsSize() const;
	/// In case of multiple connection attached to an input port this index is used to 
	/// read data from each connection in round robin way
	int roundRobinIndex() const { return rr_index_; }
	void setRoundRobinIndex(int rr_index) { rr_index_ = rr_index; }
protected:
	int rr_index_; /// round robin index to poll the connection when reading data
	std::shared_ptr<PortBase> owner_; /// PortBase pointer owning this manager
	std::vector<std::shared_ptr<ConnectionBase>> connections_; /// List of ConnectionBase associate to \p owner_
};


/// run-time value
class AttributeBase
{
public:
	AttributeBase(TaskContext *p, const std::string &name);

	virtual void setValue(const std::string &c_value) = 0;	// get generic

	virtual const std::type_info & asSig() = 0;

	virtual void * value() = 0;

	virtual std::string toString() = 0;

	const std::string & name() const { return name_; }

	void setName(const std::string & n) { name_ = n; }
	/// associated documentation
	const std::string & doc() const { return doc_; }
	/// stores doc
	void setDoc(const std::string & d) { doc_= d; }
	template <class T>
	T & as() 
	{
		if(typeid(T) != asSig())
			throw std::exception();
		else
			return *(T*)value();
	}
private:
	std::string name_;
	std::string doc_;
};

/**
 * Basic Class for Operations
 */
class OperationBase
{
public:
	OperationBase(Service * p, const std::string &name);
	/// commented for future impl
	//virtual boost::any  call(std::vector<boost::any> & params) = 0;
	/// returns the contained function pointer
	virtual void* asFx() = 0;
	/// returns the type signature
	virtual const std::type_info & asSig() = 0;
	/// returns the function as Signature if it matches, otherwise raises exception
	template <class Sig>
	std::function<Sig> & as() 
	{
		if(typeid(Sig) != asSig())
		{
			COCO_FATAL() << typeid(Sig).name() << " vs expected " << asSig().name();
			throw std::exception();
		}
		else
		{			
			return *(std::function<Sig> *)(this->asFx());
		}
	}
	/// associated documentation
	const std::string & doc() const { return doc_; }
	/// stores doc
	void setDoc(const std::string & doc) { doc_= doc; }
	/// return the name of the operation
	const std::string & name() const { return name_; }
	/// set the name of the operation
	void setName(const std::string & name) { name_ = name; }
private:
	std::string name_; /// name of the operation
	std::string doc_;
};

struct OperationInvocation
{
	OperationInvocation(const std::function<void(void)> & p);
	std::function<void(void)> fx;
};

/// Base class to manage ports
class PortBase 
{
public:
	/// Initialize input and output ports
	PortBase(TaskContext * p,const std::string &name, bool is_output, bool is_event);
	/// Return the template type name of the port data 
	virtual const std::type_info & typeInfo() const = 0;
	/// Connect a port to another with a specified ConnectionPolicy
	virtual bool connectTo(PortBase *, ConnectionPolicy policy) = 0;
	/// Return true if this port is connected to another one
	bool isConnected() const;	
	/// Return true if this port is of type event
	bool isEvent() const { return is_event_; }
	/// Return true if this port is an output port
	bool isOutput() const { return is_output_; };
	/// Trigger the task to notify new dara is present in the port
	void triggerComponent();
	/// Remove the trigger once the data has been read
	void removeTriggerComponent();
	/// associated documentation
	const std::string & doc() const { return doc_; }
	/// stores doc
	void setDoc(const std::string & d) { doc_= d; }
	/// Get the name of the port
	const std::string & name() const { return name_; }
	/// Set the name of the port
	void setName(const std::string & name) { name_ = name; }
	/// Returns the number of connections associate to this port
	int connectionsCount() const;
    /// Return the name of the task owing this connection
    std::string taskName() const;
    /// Return the task owing the port
    const TaskContext * task() const { return task_.get(); }
    /// Add a connection to the ConnectionManager
	bool addConnection(std::shared_ptr<ConnectionBase> connection);
	/// Return the ConnectionManager of this port
	const ConnectionManager &connectionManager() { return manager_; }
protected:
	ConnectionManager manager_ = { this };
	std::shared_ptr<TaskContext> task_; /// Task using this port
	std::string name_;
	std::string doc_;
	bool is_output_;
	bool is_event_;
};

// -------------------------------------------------------------------
// Execution
// -------------------------------------------------------------------

// http://www.orocos.org/stable/documentation/rtt/v2.x/api/html/classRTT_1_1base_1_1RunnableInterface.html
//class ExecutionEngine;

/// Manages all properties of a Task Context. Services is present because Task Context can have sub ones
class Service
{
public:
	friend class AttributeBase;
	friend class OperationBase;
	friend class PortBase;

	/// Initialize Service name
	Service(const std::string &n = "");
	/// Add an Atribute
	bool addAttribute(AttributeBase *a);
	/// Return an AttributeBase ptr
	AttributeBase *attribute(std::string name);
	/// Return a reference to the variable managed by this attribute
	template <class T>
	T & attributeRef(std::string name)
	{
		auto it = attributes_.find(name);
		if(it != attributes_.end())
			return it->second->as<T>();
		else
			throw std::exception();
	}
	/// Return a custo map to iterate over the keys
	const std::unordered_map<std::string, AttributeBase*> attributes() const { return attributes_; }
	/// Add a port to its list
	bool addPort(PortBase *p);
	/// Return a port based on its name
	PortBase *port(std::string name);
	/// Return a custo map to iterate over the keys
	const std::unordered_map<std::string, PortBase*> & ports() const { return ports_; };
	/// Add an operation from an existing ptr
	bool addOperation(OperationBase *operation);

	template <class Sig>
	std::function<Sig> operation(const std::string & name)
	{
		auto it = operations_.find(name);
		if(it == operations_.end())
			return std::function<Sig>();
		else
			return it->second->as<Sig>();
	}
	/// Return a custo map to iterate over the values
	const std::unordered_map<std::string, OperationBase*> & operations() const { return operations_; }
	/// Return true if the task has more operation enqueued
	bool hasPending() const;
	/// Execute the first pending operation in the queue
	void stepPending();
	/// Enqueue a new operation to be executed asynchronously
	template <class Sig, class ...Args>
	bool enqueueOperation(const std::string & name, Args... args)
	{
		//static_assert< returnof(Sig) == void 
		std::function<Sig> fx = operation<Sig>(name);
		if(!fx)
			return false;
		asked_ops_.push_back(OperationInvocation(
								//[=] () { fx(args...); }
								std::bind(fx, args...)
							 ));
		return true;
	}
	/// Add a peer
	bool addPeer(TaskContext *p);
	const std::list<TaskContext*> & peers() const { return peers_; }

	const std::unordered_map<std::string,std::unique_ptr<Service> > & services() const { return subservices_; }
	/// returns self as provider
	Service * provides() { return this; }
	/// check for sub services
	Service * provides(const std::string &x); 

	void setName(std::string name) { name_ = name; }
	/// Return the name of the Task that should be equal to the name of the class
	const std::string & name() const { return name_; }
	/// Identifier name for the task
	void setInstantiationName(const std::string &name) { instantiation_name_ = name; }

	const std::string &instantiationName() const { return instantiation_name_;}
	
	void setDoc(std::string doc) { doc_ = doc; }
	const std::string & doc() const { return doc_; }
private:
	std::string name_;
	std::string instantiation_name_ = "";
	std::string doc_;

	std::list<TaskContext*> peers_;
	std::unordered_map<std::string, PortBase* > ports_; 
	std::unordered_map<std::string, AttributeBase*> attributes_;
	std::unordered_map<std::string, OperationBase*> operations_;
	std::unordered_map<std::string, std::unique_ptr<Service> > subservices_;
	std::list<OperationInvocation> asked_ops_;
};

/// state of a TaskContext
enum TaskState { INIT, PRE_OPERATIONAL, STOPPED, RUNNING};

/**
 * The Task Context is the single task of the Component being instantiated
 *
 * A Task Context provides:
 * - operators
 * - input ports
 * - output ports
 * - parameters (config time)
 * - properties (run time)
 */
class TaskContext : public Service
{
public:
	friend class CocoLauncher;

	/// Set the activity that will manage the execution of this task
	void setActivity(Activity *activity) { activity_ = activity; }
	
	/// Start the execution
	//virtual void start();
	/// Stop the execution of the component and of the activity running the component
	virtual void stop();
	/// In case of a TRIGGER task execute one step
	void triggerActivity();
	/// Once the data is read reduce the trigger count from the activity
	void removeTriggerActivity();
	/// Return the current state of the task
	TaskState state() const { return state_; }
	//virtual const std::type_info & type() const = 0;
	//const std::type_info & type() const { return type_handler_->type_info; }
	const std::type_info & type() const { return *type_info_; }
	//void setType(const std::type_info &type) { type_handler_ = new TypeHanlder(type); }
	template<class T>
	void setType()
	{
		type_info_ = &(typeid(T));
	}

	std::shared_ptr<ExecutionEngine>  engine() const { return engine_; }
	void setEngine(std::shared_ptr<ExecutionEngine> engine) { engine_ = engine; }
protected:
	/// Creates an ExecutionEngine object
	TaskContext();

	friend class System;
	friend class ExecutionEngine;
	
	virtual std::string info() = 0;
	/// Init the task attributes and properties, called by the instantiator before spawing the thread	
	virtual void init() = 0;
	/// Function to be overload by the user. It is called once as the thread starts
	virtual void onConfig() = 0;
	/// Function to be overload by the user. It is the execution funciton
	virtual void onUpdate() = 0;

	std::shared_ptr<ExecutionEngine> engine_; // ExecutionEngine is owned by activity
private:
	Activity * activity_; // TaskContext is owned by activity
	TaskState state_;
	const std::type_info *type_info_;
};

/// Class to create peer to be associated to taskcomponent
class PeerTask : public TaskContext
{
public:
	void init() {}
	void onUpdate() {}
};

}