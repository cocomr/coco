/**
 * Compact Component Framework
 *
 * Principles:
 * - TaskContext
 * - Port (in and out)
 * - Property (for params)
 * - Connection (connecting In and Out port)
 *
 * Ownership:
 * - ports are managed by 
 *
 * Progress:
 * - loading of components
 *
 * 2014-2015 Emanuele Ruffaldi and Filippo Brizzi @ Scuola Superiore Sant'Anna, Pisa, Italy 
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
#include <boost/circular_buffer.hpp>
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
#include "coco_impl.hpp"
#include "util/coco_profiling.h"
#include "util/coco_logging.h"

namespace coco
{
class Activity;
class ParallelActivity;
class SequentialActivity;
class RunnableInterface;
class ExecutionEngine;
class AttributeBase;
class OperationBase;
// template <class T>
// class Operation;
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
	enum Policy { PERIODIC, HARD, TRIGGERED };

	// missing containment inside other container: require standalone thread
	Policy timing_policy_ = PERIODIC;
	int period_ms_;
	std::string trigger_; // trigger port
	SchedulePolicy(Policy policy = PERIODIC, int period = 1)
		: timing_policy_(policy), period_ms_(period) {}
};

/// Base class for something that loops or is activated
class Activity
{
public:
	/// Specify the execution policy and the RunnableInterface to be executed
	Activity(SchedulePolicy policy, std::shared_ptr<RunnableInterface> r);
	/// Start the activity
	virtual void start() = 0;
	/// Stop the activity
	virtual void stop() = 0;
	/// In case of a TRIGGER activity starts one step of the execution
	virtual void trigger() = 0;
	/// Main execution function
	virtual void entry() = 0;
	virtual void join() = 0;
	/// Return if the activity is periodic or not
	bool isPeriodic();
	bool isActive() const { return active_; };

	SchedulePolicy::Policy getPolicyType() const { return policy_.timing_policy_; }
protected:
	std::shared_ptr<RunnableInterface> runnable_;
	SchedulePolicy policy_;
	bool active_;
	std::atomic<bool> stopping_;
};

/// Create an activity that will run in the same thread of the caller
class SequentialActivity: public Activity
{
public:
	SequentialActivity(SchedulePolicy policy,
					  std::shared_ptr<RunnableInterface> r = nullptr); 

	virtual void start() override;
	virtual void stop() override;
	virtual void trigger() override;
	virtual void join() override;
protected:
	void entry() override;
}; 

/// Activity that run in its own thread
class ParallelActivity: public Activity
{
public:
	/// Simply call Activity constructor
	ParallelActivity(SchedulePolicy policy,
					 std::shared_ptr<RunnableInterface> r = nullptr);

	virtual void start() override;
	virtual void stop() override;
	virtual void trigger() override;
	virtual void join() override;
protected:
	void entry() override;

	int pending_trigger_ = 0;
	std::unique_ptr<std::thread> thread_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

// extern Activity * createSequentialActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r);
// extern Activity * createParallelActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r);
Activity * createSequentialActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r);
Activity * createParallelActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r);

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
};

/// Concrete class to execture the components  
class ExecutionEngine: public RunnableInterface
{
public:
	/// Constructor to set the current TaskContext to be executed
	ExecutionEngine(TaskContext *t);
	virtual void init() override;
	virtual void step() override;
	virtual void finalize() override;	
private:
	TaskContext *task_;

	bool stopped_;
};

/**
 * Connection Policy
 */
struct ConnectionPolicy 
{
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
	 *  @param data_policy_ = DATA
	 *	@param lock_policy_ = LOCKED
	 *  @param buffer_size_ = 1
	 * 	@param init_ = 1
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
    bool hasComponent(const std::string &name) const;
protected:
	/// Trigger the port to communicate new data is present
	void trigger();

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
    std::shared_ptr<ConnectionBase> getConnection(unsigned int index);
    /// Return the ConnectionBase connection inidicated by name if it exist
    std::shared_ptr<ConnectionBase> getConnection(const std::string &name);
	/// Return the number of connections
	int connectionsSize() const;
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

	virtual const std::type_info & assig() = 0;

	virtual void * value() = 0;

	virtual std::string toString() = 0;

	const std::string & name() const { return name_; }

	void setName(const std::string & n) { name_ = n; }
	/// associated documentation
	const std::string & doc() const { return doc_; }
	/// stores doc
	void setDoc(const std::string & d) { doc_= d; }
	// TODO try to fix this
	template <class T>
	T & as() 
	{
		if(typeid(T) != assig())
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
	virtual void* asfx() = 0;
	/// returns the type signature
	virtual const std::type_info & assig() = 0;
	/// returns the function as Signature if it matches, otherwise raises exception
	template <class Sig>
	std::function<Sig> & as() 
	{
		if(typeid(Sig) != assig())
		{
			COCO_FATAL() << typeid(Sig).name() << " vs expected " << assig().name();
			throw std::exception();
		}
		else
		{			
			return *(std::function<Sig> *)(this->asfx());
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

/**
 * Operator Class specialized for T as function holder (anything) 
 */
template <class T>
class Operation: public OperationBase
{
public:
	Operation(Service* p, const std::string &name, const T & fx)
		: OperationBase(p,name), fx_(fx) {}
	
	typedef typename coco::impl::get_functioner<T>::fx Sig;
 	/// return the signature of the function
	virtual const std::type_info &assig() override
	{
		return typeid(Sig);
	}

	virtual void *asfx() override
	{ 
		return (void*)&fx_;
	} 
#if 0
	/// invokation given params and return value
	virtual boost::any  call(std::vector<boost::any> & params)
	{
		if(params.size() != arity<T>::value)
		{
			std::cout << "argument count mismatch\n";
			throw std::exception();
		}
		return call_n_args<T>::call(fx_,params, make_int_sequence< arity<T>::value >{});
	}
#endif
private:
	T fx_;
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
	virtual const std::type_info & getTypeInfo() const = 0;
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
    std::shared_ptr<TaskContext> task() { return task_; }
    // TODO check why this function was protected
    /// Add a connection to the ConnectionManager
	bool addConnection(std::shared_ptr<ConnectionBase> connection);
protected:
	ConnectionManager manager_ = { this };
	std::shared_ptr<TaskContext> task_; /// Task using this port
	std::string name_;
	std::string doc_;
	bool is_event_;
	bool is_output_;
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
	AttributeBase *getAttribute(std::string name);
	/// Return a reference to the variable managed by this attribute
	template <class T>
	T & getAttributeRef(std::string name)
	{
		auto it = attributes_.find(name);
		if(it != attributes_.end())
			return it->second->as<T>();
		else
			throw std::exception();
	}
	/// Return a custo map to iterate over the keys
	coco::impl::map_keys<std::string,AttributeBase*> getAttributeNames();
	/// Return a custo map to iterate over the values
	coco::impl::map_values<std::string,AttributeBase*> getAttributes();
	/// Add a port to its list
	bool addPort(PortBase *p);
	/// Return a port based on its name
	PortBase *getPort(std::string name);
	/// Return a custo map to iterate over the keys
	coco::impl::map_keys<std::string,PortBase*> getPortNames();
	/// Return a custo map to iterate over the values
	coco::impl::map_values<std::string,PortBase*> getPorts();
	/// Add an operation from an existing ptr
	bool addOperation(OperationBase *operation);
	/// Create and add a new operation
	// TODO remove this function
	template <class Function, class Obj>
	bool addOperation(const std::string &name, Function  a, Obj b)
	{
		if (operations_[name])
		{
			COCO_ERR() << "An operation with name: " << name << " already exist";
			return false;
		}
		typedef typename coco::impl::get_functioner<Function>::target target_t;
		auto x = coco::impl::bind_this(a, b);
		operations_[name] = new Operation<target_t>(this, name, x);
		return true;
	}

	/// Create and add a new operation
	template <class Function>
	bool addOperation(const std::string &name, Function f)
	{
		if (operations_[name])
		{
			COCO_ERR() << "An operation with name: " << name << " already exist";
			return false;
		}
		typedef typename coco::impl::get_functioner<Function>::target target_t;
		operations_[name] = new Operation<target_t>(this, name, f);
		return true;
	}

	template <class Sig>
	std::function<Sig> getOperation(const std::string & name)
	{
		auto it = operations_.find(name);
		if(it == operations_.end())
			return std::function<Sig>();
		else
			return it->second->as<Sig>();
	}
	/// Return a custo map to iterate over the keys
	coco::impl::map_keys<std::string, OperationBase*> getOperationNames();
	/// Return a custo map to iterate over the values
	coco::impl::map_values<std::string, OperationBase*> getOperations();
	/// Return true if the task has more operation enqueued
	bool hasPending() const;
	/// Execute the first pending operation in the queue
	void stepPending();
	/// Enqueue a new operation to be executed asynchronously
	template <class Sig, class ...Args>
	bool enqueueOperation(const std::string & name, Args... args)
	{
		//static_assert< returnof(Sig) == void 
		std::function<Sig> fx = getOperation<Sig>(name);
		if(!fx)
			return false;
		asked_ops_.push_back(OperationInvocation(
								[=] () { fx(args...); }
							 ));
		return true;
	}
	/// Add a peer
	bool addPeer(TaskContext *p);
	std::list<TaskContext*> getPeers() { return peers_; }

	coco::impl::map_keys<std::string,std::unique_ptr<Service> > getServiceNames();
	coco::impl::map_values<std::string,std::unique_ptr<Service> > getServices();
	/// returns self as provider
	Service * provides() { return this; }
	/// check for sub services
	Service * provides(const std::string &x); 

	void setName(std::string name) { name_ = name; }
	/// Return the name of the Task that should be equal to the name of the class
	const std::string & name() const { return name_; }
	/// Identifier name for the task
	// TODO change in idName
	void setInstantiationName(const std::string &name) { instantiation_name_ = name; }

	const std::string &instantiationName() const { return instantiation_name_;}
	
	void setDoc(std::string doc) { doc_ = doc; }
	const std::string & doc() const { return doc_; }
private:
	std::string name_;
	std::string instantiation_name_ = "";
	std::string doc_;

	std::list<TaskContext*> peers_;
	std::map<std::string, PortBase* > ports_; 
	std::map<std::string, AttributeBase*> attributes_;
	std::map<std::string, OperationBase*> operations_;
	std::map<std::string, std::unique_ptr<Service> > subservices_;
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
	virtual void start();
	/// Stop the execution of the component
	virtual void stop();
	/// In case of a TRIGGER task execute one step
	virtual void triggerActivity();

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