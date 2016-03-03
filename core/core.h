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



/*! \brief Policy for an activity instantiation.
 *  Specifies wheter the actvity has to be executed on a new thread
 *  or on the main thread
 */
enum ThreadSpace { OWN_THREAD, CLIENT_THREAD};

/*! \brief Policy for the scheduling of activities.
 *  Contains information regarding the execution activation policy,
 *  and the eventual core affinity.
 */
struct SchedulePolicy 
{
	/*! \brief Activity execution policy.
	 *  Specifies the activity scheduling policy
	 */
	enum Policy
	{ 
		PERIODIC,   //!< The activity executes periodically with a given period
		HARD,	    //!<
		TRIGGERED   //!< The activity execution is triggered by an event port receiving data
	};

	/*! \brief Base constructor with default values.
	 */
	SchedulePolicy(Policy policy = PERIODIC, int period = 1)
		: scheduling_policy(policy), period_ms(period) {}

	
	Policy scheduling_policy; //!< Scheduling policy
	int period_ms; //!< In case of a periodic activity specifies the period in millisecon
	int affinity = -1; //!< Specifies the core id where to pin the activity. If -1 no affinity
	std::list<unsigned int> available_core_id; //!< Contains the list of the available cores where the activity can run

	// missing containment inside other container: require standalone thread
	//std::string trigger; // trigger port
};

/** \brief The container for components
 *  A CoCo application is composed of multiple activities, each one containing components.
 *  Each activity is associated with a thread and scheduled according to \ref SchedulePolicy
 */
class Activity
{
public:
	/*! \brief Specifies the execution policy when instantiating an activity
	 */
	Activity(SchedulePolicy policy);
	/*! \brief Starts the activity.
	 */
	virtual void start() = 0;
	/*! \brief Stops the activity.
	 *  Sets the termination flag and wakes up all the internal components.
	 */
	virtual void stop() = 0;
	/*! \brief Called to resume the execution of a trigger activity.
	 *  Increases the trigger count and wake up the execution.
	 */
	virtual void trigger() = 0;
	/*! \brief Decreases the trigger count.
	 *  When data from an event port is read decreases the trigger counter.
	 */
	virtual void removeTrigger() = 0;
	/*! \brief Main execution function. 
	 *  Contains the main execution loop. Manage the period timer in case of a periodic activity
	 *  and the condition variable for trigger activityies.
	 *  For every execution step, iterates over all the components contained by the activity
	 *  and call ExecutionEngine::step() function.
	 */
	virtual void entry() = 0;
	/*! \brief Join on the thread containing the activity
	 */
	virtual void join() = 0;
	/*!
	 * \return If the activity is periodic.
	 */
	bool isPeriodic() const;
	/*!
	 * \return The period in millisecond of the activity.
	 */
	int period() const { return policy_.period_ms; }
	/*!
	 * \return If the actvity is running.
	 */
	bool isActive() const { return active_; };
	/*!
	 * \return The schedule policy of the activity.
	 */
	SchedulePolicy & policy() { return policy_; }
	/*!
	 * \return The schedule policy of the activity.
	 */
	const SchedulePolicy & policy() const { return policy_; }
	/*! \brief Add a \ref RunnableInterface object to the activity.
	 * \param runnable Shared pointer of a RunnableInterface objec.
	 */
	void addRunnable(const std::shared_ptr<RunnableInterface> &runnable) { runnable_list_.push_back(runnable); }
	/*!
	 * \return The list of all the RunnableInterface objects associated to the activity
	 */
	const std::list<std::shared_ptr<RunnableInterface> > &runnables() const { return runnable_list_; }

protected:
	std::list<std::shared_ptr<RunnableInterface> > runnable_list_;
	SchedulePolicy policy_;
	bool active_;
	std::atomic<bool> stopping_;
};

/// Create an activity that will run in the same thread of the caller
/*! \brief Create an activity running on the main thread of the process.
 *  Maximum one sequential activity per application
 */
class SequentialActivity: public Activity
{
public:
	/*! \brief Specifies the execution policy when instantiating an activity
	 */
	SequentialActivity(SchedulePolicy policy);
	/*! \brief Starts the activity.
	 *  Simply call entry().
	 */
	virtual void start() override;

	virtual void stop() override;
	/*! \brief NOT IMPLEMENTED FOR SEQUENTIAL ACTIVITY
	 */
	virtual void trigger() override;
	/*! \brief NOT IMPLEMENTED FOR SEQUENTIAL ACTIVITY
	 */
	virtual void removeTrigger() override;
	/*! \brief Does nothing, nothing to join
	 */
	virtual void join() override;
protected:

	virtual void entry() override;
}; 

/// Activity that run in its own thread
class ParallelActivity: public Activity
{
public:
	ParallelActivity(SchedulePolicy policy);
	/*! \brief Starts the activity.
	 *  Moves the execution (entry() function) on a new thread
	 *  and sets the affinity according to the \ref SchedulePolicy.
	 */
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

/*! \brief Interface that manages the execution of a component.
 *  It is in charge of the component initialization, loop function
 *  and pending operations.
 */
class RunnableInterface
{
public:
	/*! \brief Calls the component initialization function
	 */
	virtual void init() = 0;
	/*! \brief Execute one step of the loop.
	 */
	virtual void step() = 0;
	/*! \brief It is called When the execution is stopped.
	*/
	virtual void finalize() = 0;
protected:
};

/*! \brief Container to manage the execution of a component.
 *  It is in charge of the component initialization, loop function
 *  and pending operations.
 */
class ExecutionEngine: public RunnableInterface
{
public:
	/*! \brief Base constructor.
	 *  \param task The component assosiaceted to the engine.
	 *  	   One engine controlls only one component and
	 * 		   one component must be associated to only on engine.
	 *  \param profiling Flag to specify to the engine if to inject profiling
	 *         call into the step function.
	 */
	ExecutionEngine(TaskContext *task, bool profiling);
	/*! \brief Calls the TaskContext::onConfig() function of the associated task.
	 */
	virtual void init() override;
	/*! \brief Execution step.
	 *  Iterate over the task pending operations executing them and then
	 *  and then executes the TaskContext::onUpdate() function.
	 */
	virtual void step() override;
	/*! Call the component stop function, TaskContext::stop().
	 */
	virtual void finalize() override;
	/*!
	 * \return The pointer to the associated component object.
	 */
	const TaskContext * task() const { return task_; }
private:
	TaskContext * task_;
	bool stopped_;
	bool profiling_ = false;
};

/*! \brief Specify the policy of the connection between two ports.
 *  For every connection there is a policy specifying the buffer type,
 *  the lock type and the transport type. Connections are always non blocking.
 */
struct ConnectionPolicy 
{
	/*! \brief Buffer policy
	 */
	enum BufferPolicy
	{ 
		DATA,      //!< Buffer of lenght 1. Incoming data always override existing one.
		BUFFER,    //!< Buffer of lenght \ref buffer_size. If the buffer is full new data is discarded.
		CIRCULAR   //!< Circular FIFO buffer of lenght \ref buffer_size. If the buffer is full new data overrides the oldest one.
	};
	/*! \brief Lock policy for concurrent access management.
	 */
	enum LockPolicy
	{ 
		UNSYNC,    //!< No resource access control policy
		LOCKED,    //!< Data access is regulated by mutexes
		LOCK_FREE  //!< NOT IMPLEMENTED YET (TBD)
	};
	/*! \brief Specifies if the connection is between two threads or between processes.
	 */
	enum Transport
	{ 
		LOCAL, //!< Connection between two thread of the same process. Communication using shared memory.
		IPC    //!< NOT IMPLEMENTED YET (TBD)
	};

	BufferPolicy data_policy;
	LockPolicy lock_policy;
	int buffer_size; 		     //!< Size of the buffer
	bool init = false; 
	Transport transport;
	//std::string name_id;

	/*! \brief Default constructor.
	 *  Default values:
	 *  \param data_policy = DATA
	 *	\param lock_policy = LOCKED
	 *  \param buffer_size = 1
	 * 	\param init = 1
	 *  \param transport = LOCAL
	 */
	ConnectionPolicy();
	/*! \brief Construct a ConnectionPolicy object with the options parsed from string.
	 */	
	ConnectionPolicy(const std::string &policy, const std::string &lock,
				     const std::string &transport_type, const std::string &buffer_size);
};

#undef NO_DATA
/*! \brief Status of the data present in a connection buffer,
 */
enum FlowStatus
{
	NO_DATA,   //!< No data present in the buffer.
	OLD_DATA,  //!< There is data in the buffer but it has already been read.
	NEW_DATA   //!< New data in the buffer.
};

/*! \brief Base class for connections.
 *  Contains the basic funcitons to manage a connection.
 */
class ConnectionBase
{
public:
	/*! Costructor of the connection.
	 *  \param in Input port of the connection.
	 *  \param out Output port of the connection.
	 *  \param policy of the connection.
	 */
	ConnectionBase(PortBase *in, PortBase *out, ConnectionPolicy policy);
	virtual ~ConnectionBase() {}

	/*!
	 * \return If new data is present in the InputPort.
	 */
	bool hasNewData() const;
	/*!
	 * \param name name of component.
	 * \return If one of the task involved in the connection has name \param name.
	 */
    bool hasComponent(const std::string &name) const;
    /*!
     * \return Pointer to the input port.
     */
    const PortBase * input() const { return input_; }
    /*!
     * \return Pointer to the output port.
     */
    const PortBase * output() const { return output_; }
protected:
	/*! \brief Call InputPort::triggerComponent() function to trigger the owner component execution.
	 */ 
	void trigger();
	/*! \brief Once data has been read remove the trigger calling InputPort::removeTriggerComponent()
	*/
	void removeTrigger();

	FlowStatus data_status_;
	ConnectionPolicy policy_;
	PortBase *input_ = 0;
	PortBase *output_ = 0;
};


/*! Manages the connections of one PortBase
 *  Ports can have multiple connections associated to them.
 *  ConnectionManager keeps track of all these connections.
 *  In case of multiple incoming port manages the round robin
 *  schedule used to access all the connections
 */
class ConnectionManager 
{
public:
	/*! Base constructor. Set the round robin index to 0
	 * \param port The port owing it.
	 */
	ConnectionManager(PortBase * port);
	/*!
	 * \param connection Shared pointer of the connection to be added at the \ref owner_ port.
	 */
	bool addConnection(std::shared_ptr<ConnectionBase> connection);
	/*!
	 * \return If the associated port has any active connection.
	 */
	bool hasConnections() const;	
	/*!
	 * \param index Index of a connection
	 * \return If index is less than the number of connections return the pointer to that connection.
	 */
    std::shared_ptr<ConnectionBase> connection(unsigned int index);
    /*!
     *  \param name The name of a conneciton.
     *  \return The pointer to the connection indicated by \ref name if it exists.
     */
    std::shared_ptr<ConnectionBase> connection(const std::string &name);
    /*!
     * \return All the connections.
     */
    const std::vector<std::shared_ptr<ConnectionBase>> & connections() const { return connections_; }
	/*!
	 * \return Number of connections.
	 */
	int connectionsSize() const;
	/*! In case of multiple connection attached to an input port this index is used to 
	 *  read data from each connection in round robin way.
	 *  \return The index of the next connection to query for data.
	 */
	int roundRobinIndex() const { return rr_index_; }
	/*!
	 * \param rr_index The index of for the round robin query.
	 */
	void setRoundRobinIndex(int rr_index) { rr_index_ = rr_index; }
protected:
	int rr_index_; //!< round robin index to poll the connection when reading data
	std::shared_ptr<PortBase> owner_; //!< PortBase pointer owning this manager
	std::vector<std::shared_ptr<ConnectionBase>> connections_; //!< List of ConnectionBase associate to \ref owner_
};


/*! \brief Used to set the value of components variables at runtime.
 *  Each Attribute is associated with a component class variable and it is identified by a name.
 *  The same component cannot have two attribute with the same name.
 */
class AttributeBase
{
public:
	/*! \brief Create the attribute and bind it with the component
	 *  \param task The component that contains the attribute.
	 *  \param name The name of the attribute.
	 */
	AttributeBase(TaskContext *task, const std::string &name);
	/*! \brief Set the value of the attribute and therefore of the variable bound to it.
	 *  \value The litteral value of the attribute that will be converted in the underlying variable type.
	 */
	virtual void setValue(const std::string &value) = 0;	// get generic
	/*!
	 * \return The type of the bound variable.
	 */
	virtual const std::type_info & asSig() = 0;
	/*!
	 * \return Pointer to the associated variable.
	 */
	virtual void * value() = 0;
	/*! \brief Serialize the value of the associated variable to a string.
	 */
	virtual std::string toString() = 0;
	/*!
     * \return The name of the attribute.
     */
	const std::string & name() const { return name_; }
	/*!
	 * \param name Set the name of the attribute.
	 */
	void setName(const std::string & name) { name_ = name; }
	/*!
	 * \return The eventual documanetation associated to the attribute.
	 */
	const std::string & doc() const { return doc_; }
	/*!
	 * \param doc Associates to the attribute a documentation.
	 */
	void setDoc(const std::string & doc) { doc_= doc; }
	/*!
	 * \return Cast the underlaying variable to \ref T type and return it.
	 */
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