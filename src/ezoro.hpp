/**
 * Micro Component Framework -- Orocos RTT 2.0 inspired
 * C++11
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
 
 */
#pragma once
#include <vector>
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
#include <mutex>
#include <memory>
#include <iostream>
#include <deque>
#include <string>
#include <atomic>
#include <condition_variable>
//#include <boost/any.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/lockfree/queue.hpp>
 #include <boost/lexical_cast.hpp>

#include "tinyxml2.h"

namespace coco
{
	namespace impl
	{
		template<std::size_t...> struct int_sequence {};
		template<std::size_t N, std::size_t... Is> struct make_int_sequence
		    : make_int_sequence<N-1, N-1, Is...> {};

		template<std::size_t... Is> struct make_int_sequence<0, Is...>
		    : int_sequence<Is...> {};		
	}
}
 
namespace std
{
	template<int> // begin with 0 here!
	struct placeholder_template
	{};

    template<int N>
    struct is_placeholder< placeholder_template<N> >        : integral_constant<int, N+1> // the one is important
    {};
}


namespace coco {

class Port;
class TaskContext;
class Service;
class Runnable;
class Activity;
class ExecutionEngine;

/// status of a connection port
enum FlowStatus { NO_DATA, OLD_DATA, NEW_DATA };

/// state of a TaskContext
enum TaskState { INIT, PRE_OPERATIONAL, STOPPED, RUNNING};

/// policy for the execution of operations
enum ThreadSpace { OWN_THREAD, CLIENT_THREAD};

#if 0
/// param time
/// @TODO virtual
class PropertyBase {
public:
	PropertyBase(TaskContext * p, const char * name);

	virtual void setValue(const char *c_value) = 0;

	const std::string & name() const { return name_; }

private:
	// get generic
	// get by type

	std::string name_;
};

/// type spec
template <class T>
class Property: public PropertyBase {
public:
	Property(TaskContext * p, const char * name): PropertyBase(p,name) {}

	Property & operator = (const T & x) { value_ = x; return *this;}

	virtual void setValue(const char *c_value) override {
        value_ = boost::lexical_cast<T>(c_value);    
	}

	T value_;
};
#endif

/// run-time value
/// @TODO virtual
class AttributeBase {
public:
	AttributeBase(TaskContext * p, std::string name);

	virtual void setValue(const char *c_value) = 0;	// get generic

	virtual const std::type_info & assig() = 0;

	virtual void * getPValue() = 0;

	const std::string & name() const { return name_; }

	template <class T>
	T & as() 
	{
		if(typeid(T) != assig())
			throw std::exception();
		else
			return *(T*)getPValue();
	}

private:
	// get by type
	std::string name_;
};

/// template spec of attribute
template <class T>
class Attribute: public AttributeBase {
public:
	typedef T value_t;

	Attribute(TaskContext * p, std::string name) : AttributeBase(p,name) {}
	Attribute & operator = (const T & x) { value_ = x; return *this;}

	virtual const std::type_info & assig() override { return typeid(value_t); }

	virtual void setValue(const char *c_value) override {
        value_ = boost::lexical_cast<value_t>(c_value);    
	}

	virtual void * getPValue() { return & value_; }

	const T & value() const { return value_; }

private:
	T value_;
};

template <class T>
class AttributeRef: public AttributeBase {
public:
	typedef T value_t;

	AttributeRef(TaskContext * p, std::string name, T & rvalue) : AttributeBase(p, name), value_(rvalue)  {  }
	AttributeRef & operator = (const T & x) { value_ = x; return *this;}

	virtual const std::type_info & assig() override { return typeid(value_t); }

	virtual void setValue(const char *c_value) override 
	{
        value_ = boost::lexical_cast<value_t>(c_value);    
	}

	virtual void * getPValue() override { return & value_; }

private:
	T & value_;
};


#if 0
// attribute
// in RTT 2.0 it is base for: Attribute, Constant, Alias
class AttributeBase
{
public:
	// name
	// ready
	// virtual clone
	virtual AttributeBase * clone() = 0;
};

template <class T>
class Attribute : public AttributeBase
{
public:
	T const & get() const;
	void set(T const & s);

	std::string name;

	virtual AttributeBase * clone() {}
};
#endif

namespace impl
{
	template< class T>
	struct getfunctioner {};
	 
	template< class R, class U, class...Args>
	struct getfunctioner<R (U::*)(Args...) > {
		typedef std::function<R(Args...)> target;
		using fx = R(Args...);
	};
	 
	template< class R,  class...Args>
	struct getfunctioner< std::function<R(Args...) > > {
		typedef std::function<R(Args...)> target;
		using fx = R(Args...);
	};
	 
	template< class R, class...Args>
	struct getfunctioner<R(Args...)> {
		typedef std::function<R(Args...)> target;
		using fx = R(Args...);
	};

	// utility
	template<class R, class U, class... Args, std::size_t... Is>
	auto bindthissub(R (U::*p)(Args...), U * pp, coco::impl::int_sequence<Is...>) -> decltype(std::bind(p, pp, std::placeholder_template<Is>{}...))
	{
		return std::bind(p, pp, std::placeholder_template<Is>{}...);
	}
	 
	// binds a member function only for the this pointer using std::bind
	template<class R, class U, class... Args>
	auto bindthis(R (U::*p)(Args...), U * pp) -> decltype(bindthissub(p,pp,coco::impl::make_int_sequence< sizeof...(Args) >{}))
	{
		return bindthissub(p,pp,coco::impl::make_int_sequence< sizeof...(Args) >{});
	}
 }

/**
 * Basic Class for Operations
 */
class OperationBase {
public:
	OperationBase(Service * p, const char * name) : name_(name) {}

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
			std::cout << typeid(Sig).name() << " vs expected " << assig().name() << std::endl;
			throw std::exception();
		}
		else
			return *(std::function<Sig> *)asfx();	
	}

	std::string name_; /// name of the operation
};

/**
 * Operator Class specialized for T as Signature 
 */
template <class T>
class Operation: public OperationBase {
public:
	Operation(Service * p, const T & fx, const char * name): OperationBase(p,name), fx_(fx) {}

	typedef T value_t;
	typedef typename coco::impl::getfunctioner<T>::fx Sig;
 
	virtual const std::type_info &assig()	{ 		return typeid(Sig);  	}

	// all std::function are the same ... 
	virtual void *asfx()  	{		return (void*)&fx_; 	}
 
 #if 0
 	/// complete this for dynamic invocation (not exact signature invocation)

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

	T fx_;
};




/**
 * Connection Policy
 */
struct ConnectionPolicy 
{
	enum Policy { DATA, BUFFER, CIRCULAR};
	enum LockPolicy { UNSYNC, LOCKED, LOCK_FREE};
	enum Transport { LOCAL, IPC};

	Policy data_policy_; /**< type of data container */
	LockPolicy lock_policy_; /** type of lock policy */
	int buffer_size_; /** size of the data container */
	bool init_ = false; 
	std::string name_id_; // of connection
	Transport transport_ = LOCAL;
	/** Default constructor, default value:
	 *  @param data_policy_ = DATA
	 *	@param lock_policy_ = LOCKED
	 *  @param buffer_size_ = 1
	 * 	@param init_ = 1
	 */
	ConnectionPolicy() 
		: data_policy_(DATA), lock_policy_(LOCKED), buffer_size_(1), init_(false) {}
	ConnectionPolicy(Policy policiy, int buffer_size, bool blocking = false)
		: data_policy_(policiy), lock_policy_(LOCKED), buffer_size_(buffer_size), init_(false) {}

	ConnectionPolicy(const char *policy, const char *lock_policy, const char *transport, const char *buffer_size) {
		buffer_size_ = boost::lexical_cast<int>(buffer_size);

		if (strcmp(policy, "DATA") == 0)
			data_policy_ = DATA;
		else if (strcmp(policy, "BUFFER") == 0)
			data_policy_ = BUFFER;
		else if (strcmp(policy, "CIRCULAR") == 0)
			data_policy_ = CIRCULAR;

		if (strcmp(lock_policy, "UNSYNC") == 0)
			lock_policy_ = UNSYNC;
		else if (strcmp(lock_policy, "LOCKED") == 0)
			lock_policy_ = LOCKED;
		else if (strcmp(lock_policy, "LOCK_FREE") == 0)
			lock_policy_ = LOCK_FREE;

		if (strcmp(transport, "LOCAL") == 0)
			transport_ = LOCAL;
		else if (strcmp(transport, "IPC") == 0)
			transport_ = IPC;
	}
};

class PortBase;


/**
 * Base class for connections 
 */
class ConnectionBase
{
public:
	/** \brief Constructor */
	ConnectionBase(PortBase *in, PortBase *out, ConnectionPolicy policy)
		: input_(in), output_(out), policy_(policy), data_status_(NO_DATA) {}


	virtual ~ConnectionBase() {}

	/** @return NEW_DATA if new data is present in the Input port */
	bool hasNewData() const { return data_status_ == NEW_DATA;  }

protected:
	/** Trigger the port to communicate new data is present */
	void trigger();

	FlowStatus data_status_; /*!< \brief status of the data in the container */
	ConnectionPolicy policy_; /*!< \brief policy for data management */
	PortBase * input_ = 0; /*!< \brief input port untyped */
	PortBase * output_ = 0; /*!< \brief output port untyped */
};


template <class T>
class OutputPort;

template <class T>
class InputPort;

/**
 * \brief Template class to manage the type of the connection
 */
template <class T>
class ConnectionT : public ConnectionBase
{
public:
	/** \brief Simply call ConnectionBase constructor */
	ConnectionT(InputPort<T> *in, OutputPort<T> *out, ConnectionPolicy policy)
		: ConnectionBase((PortBase*)in, (PortBase*)out, policy) {}

	using ConnectionBase::ConnectionBase;
	/** \brief If there is new data in the container retrive it */
	virtual FlowStatus getData(T & data) = 0;
	/** \brief Add data to the container.
	 *  If the input port is of type event trigger it to wake up the execution
	 */
	virtual bool addData(T & data) = 0;
	//virtual FlowStatus getNewestData(T & data) = 0;

};

/**
 * \brief Specialized class for the type T to manage ConnectionPolicy::DATA ConnectionPolicy::LOCKED
 */
template <class T>
class ConnectionDataL : public ConnectionT<T>
{
public:
	/** \brief Simply call ConnectionT<T> constructor */
	ConnectionDataL(InputPort<T> *in, OutputPort<T> *out, ConnectionPolicy policy)
		: ConnectionT<T>(in,out, policy) {}

	~ConnectionDataL()
	{
		if(this->data_status_ == NEW_DATA) 
		{			
			value_t_.~T();  
		}
	}

	/*virtual FlowStatus getNewestData(T & data) 
	{
		return getData(data);
	}*/	

	virtual FlowStatus getData(T & data) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_t_);
		if(dtorpolicy_)
		{
			if(this->data_status_ == NEW_DATA) 
			{			
				data = value_t_; // copy => std::move
				value_t_.~T();   // destructor 
				this->data_status_ = NO_DATA;
				return NEW_DATA;
			} 
			else
				return NO_DATA;			
		}
		else
		{
			if(this->data_status_ == NEW_DATA) 
			{			
				data = value_t_;
				this->data_status_ = OLD_DATA;
				return NEW_DATA;
			} 
			else 
				return this->data_status_;
		}
	}


	virtual bool addData(T & input) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_t_);
		if(dtorpolicy_)
		{
			if (this->data_status_ == NEW_DATA)
			{
				value_t_ = input;
			}
			else
			{
				new (&value_t_) T(input); 
				this->data_status_ = NEW_DATA; // mark
			}
		}
		else
		{
			if (this->data_status_ == NO_DATA)
			{
				new (&value_t_) T(input); 
				this->data_status_ = NEW_DATA; 
			}
			else
			{
				value_t_ = input;
				this->data_status_ = NEW_DATA; 
			}
		}
		if(this->input_->isEvent())
			this->trigger();
		return true;
	}
private:
	bool dtorpolicy_ = false;
	union { T value_t_; };
	std::mutex mutex_t_;
	//std::condition_variable cond_;
};

#if 0
template <class T>
class ConnectionDataLF : public ConnectionT<T>
{
public:
	ConnectionDataLF(InputPort<T> *in, OutputPort<T> *out, ConnectionPolicy policy)
		: ConnectionT<T>(in,out, policy) 
	{
	}
	
	virtual FlowStatus getData(T & data) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_t_);
		if(this->data_status_ == NEW_DATA) 
		{
			data = value_t_;
			value_t_.~T();
			this->data_status_ = NO_DATA;
			return NEW_DATA;
		} 
		else 
			return NO_DATA;
	}

	// TODO: to know if the write always succedes!! or in the case of buffer if it is full
	// if it erase the last value or first in the line.
	virtual bool addData(T &input) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_t_);
		new (&value_t_)T(input);
		this->data_status_ = NEW_DATA;
		if(this->input_->isEvent())
		{
			// trigger
		}
		return true;
	}
private:
	union { T value_t_; };
};
#endif

/**
 * \brief Specialized class for the type T to manage ConnectionPolicy::DATA ConnectionPolicy::UNSYNC
 */
template <class T>
class ConnectionDataU : public ConnectionT<T>
{
public:
	/** \brief Simply call ConnectionT<T> constructor */
	ConnectionDataU(InputPort<T> *in, OutputPort<T> *out, ConnectionPolicy policy)
		: ConnectionT<T>(in,out, policy) {}
	~ConnectionDataU() {
		value_t_.~T();
	}
	/*virtual FlowStatus getNewestData(T & data) 
	{
		return getData(data);
	}*/	

	virtual FlowStatus getData(T & data) override
	{
		if(this->data_status_ == NEW_DATA) 
		{
			data = value_t_;
			value_t_.~T();
			this->data_status_ = NO_DATA;
			return NEW_DATA;
		}  
		return NO_DATA;
	}

	virtual bool addData(T &input) override
	{
		if (this->data_status_ == NEW_DATA)
			value_t_.~T();
		new (&value_t_) T(input);
		this->data_status_ = NEW_DATA;

		if(this->input_->isEvent())
			this->trigger();
	
		return true;
	}
private:
	union { T value_t_; };
};


/**
 * \brief Specialized class for the type T to manage ConnectionPolicy::BUFFER/CIRCULAR_BUFFER ConnectionPolicy::UNSYNC
 */
template <class T>
class ConnectionBufferU : public ConnectionT<T>
{
public:
	/** \brief Simply call ConnectionT<T> constructor */
	ConnectionBufferU(InputPort<T> *in, OutputPort<T> *out, ConnectionPolicy policy)
		: ConnectionT<T>(in, out, policy) 
	{
		buffer_.resize(policy.buffer_size_);
	}
	
	virtual FlowStatus getNewestData(T & data) 
	{
		bool something = false;
		while(!buffer_.empty())
		{
			something = true;
			data = buffer_.front();
			buffer_.pop_front();
		}
		return something ? NEW_DATA : NO_DATA;
	}	

	virtual FlowStatus getData(T & data) override
	{
		if(!buffer_.empty())
		{
			data = buffer_.front();
			buffer_.pop_front();
			return NEW_DATA;
		}
		else 
			return NO_DATA;
	}

	virtual bool addData(T &input) override
	{
		if (buffer_.full())
		{
			if(this->policy_.data_policy_ == ConnectionPolicy::CIRCULAR)
				buffer_.pop_front();
			else
				return false;
		}
		buffer_.push_back(input);
		this->data_status_ = NEW_DATA;
		if(this->input_->isEvent())		
			this->trigger();

		return true;
	}

private:
	boost::circular_buffer<T> buffer_;
};

/**
 * \brief Specialized class for the type T to manage ConnectionPolicy::BUFFER/CIRCULAR_BUFFER ConnectionPolicy::LOCKED
 */
template <class T>
class ConnectionBufferL : public ConnectionT<T>
{
public:
	/** \brief Simply call ConnectionT<T> constructor */
	ConnectionBufferL(InputPort<T> *in, OutputPort<T> *out, ConnectionPolicy policy)
		: ConnectionT<T>(in, out, policy) 
	{
		buffer_.resize(policy.buffer_size_);
	}
	
	virtual FlowStatus getNewestData(T & data) 
	{
		std::unique_lock<std::mutex> mlock(this->mutex_t_);
		bool something = false;
		while(!buffer_.empty())
		{
			data = buffer_.front();
			buffer_.pop_front();
			something = true;
		}
		return something ? NEW_DATA : NO_DATA;
	}	

	virtual FlowStatus getData(T & data) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_t_);
		if(!buffer_.empty())
		{
			data = buffer_.front();
			buffer_.pop_front();
			return NEW_DATA;
		} 
		return NO_DATA;
	}

	virtual bool addData(T &input) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_t_);
		if (buffer_.full())
		{
			if(this->policy_.data_policy_ == ConnectionPolicy::CIRCULAR)
				buffer_.pop_front();
			else
				return false;
		} 
		buffer_.push_back(input);
		if(this->input_->isEvent())
			this->trigger();
		
		return true;
	}

private:
	boost::circular_buffer<T> buffer_;
	std::mutex mutex_t_;
};

/**
 * Specialized class for the type T
 */
 /*
template <class T>
class ConnectionBufferLF : public ConnectionT<T>
{
public:
	ConnectionBufferLF(InputPort<T> *in, OutputPort<T> *out, ConnectionPolicy policy)
		: ConnectionT<T>(in, out, policy) 
	{
		queue_.reserve(policy.buffer_size_);
	}
	
	virtual FlowStatus getNewestData(T & data) 
	{
		bool once = false;
		while(queue_.pop(data))
		{
			once = true;
		}
		return once ? NEW_DATA : NO_DATA;
	}	

	virtual FlowStatus getData(T & data) override
	{
		return queue_.pop(data) ? NEW_DATA : NO_DATA; 
	}

	virtual bool addData(T &input) override
	{
		if(!queue_.push(input))
		{
			if(this->policy_.data_policy_ == ConnectionPolicy::CIRCULAR)
			{
				T dummy;
				queue_.pop(dummy);
				queue_.push(input);				
			}
			return false;
		}
		if(this->input_->isEvent())
			this->trigger();
		
		return true;
	}

private:
	// TODO with fixed_sized<true> it's not possible to specify the lenght of the queue at runtime!
	//boost::lockfree::queue<T,boost::lockfree::fixed_sized<true> > queue_;
	boost::lockfree::queue<T> queue_;
};
*/

/** 
 * \brief Manage the connections of one PortBase
 */
class ConnectionManager 
{
public:
	/** \brief set the RounRobin index to 0 and set its PortBase ptr owner */
	ConnectionManager(PortBase * o)
		: rr_index_(0), owner_(o) {}

	/** \brief add a connection to \p connections_ */
	bool addConnection(std::shared_ptr<ConnectionBase> connection) 
	{
		connections_.push_back(connection);
		return true;
	}
	/** \brief return true if \p connections_ has at list one elemnt */
	bool hasConnections() const
	{
		return connections_.size()  != 0;
	}
	/** \brief return the ConnectionBase connection inidicated by index if it exist */ 
	std::shared_ptr<ConnectionBase> getConnection(int index)
	{
		if (index < connections_.size()) 
			return connections_[index];
		else
			return nullptr;
	}

	/** \brief return the ConnectionT<T> connection inidicated by index if it exist */
	template <class T>
	std::shared_ptr<ConnectionT<T> > getConnectionT(int index)
	{
		return std::static_pointer_cast<ConnectionT<T>>(getConnection(index));
	}
	/** \brief return the number of connections */
	int connectionsSize() const { return connections_.size(); }

protected:
	template <class T>
	friend class InputPort;

	template <class T>
	friend class OutputPort;

	int rr_index_; /*!< \brief round robin index to poll the connection when reading data */
	PortBase * owner_; /*!< \brief PortBase pointer owning this manager*/
	std::vector<std::shared_ptr<ConnectionBase>> connections_; /*!< \brief List of ConnectionBase associate to \p owner_ */
};

/**
 * \brief Base class to manage ports
 */
class PortBase 
{
public:
	/** \brief initialize input and output ports */
	PortBase(TaskContext * p,const char * name, bool is_output, bool is_event);
	/** \brief return the template type name of the port data */
	virtual const std::type_info & getTypeInfo() const = 0;
	/** \brief connect a port to another with a specified ConnectionPolicy */
	virtual bool connectTo(PortBase *, ConnectionPolicy policy) = 0;
	/** \brief return true if this port is connected to another one */
	bool isConnected() const { return manager_.hasConnections(); }	
	/** \brief return true if this port is of type event */
	bool isEvent() const { return is_event_; }	
	/** Trigger the task to notify new dara is present in the port */
	void triggerComponent();

	std::shared_ptr<TaskContext> task_; /**< \brief Task using this port */
	std::string name_;
protected:
	/** \brief add a connection to the ConnectionManager */
	bool addConnection(std::shared_ptr<ConnectionBase> connection);
	/** \brief returns the number of connections associate to this port */
	int connectionsCount() const;

	template <class T>
	friend class InputPort;

	template <class T>
	friend class OutputPort;

	ConnectionManager manager_ = { this };
	//std::shared_ptr<TaskContext> task_; /**< \brief Task using this port */
	
	bool is_event_;
	bool is_output_;
};

template <class T>
ConnectionT<T> * makeConnection(InputPort<T> * a, OutputPort<T> * b, ConnectionPolicy p);


/**
 * \brief Class representing an input port containing data of type T
 */
template <class T>
class InputPort: public PortBase 
{
public:
	typedef T value_t;

	/** \brief simply call PortBase constructor */
	InputPort(TaskContext * p, const char * name, bool is_event = false) 
		: PortBase(p, name, false, is_event) {}

	const std::type_info &getTypeInfo() const override { return typeid(value_t); }

	bool connectTo(PortBase *other, ConnectionPolicy policy) 
	{
		// check if the they have the same template
		if (getTypeInfo() == other->getTypeInfo())
		{
			// check if it is Output Port
			if (OutputPort<T> *o = dynamic_cast<OutputPort<T> *>(other)) {
				return connectToTyped(o, policy);
			} else {
				std::cout << "Destination port is not an OutputPort!" << std::endl;
				return false;
			}
		} else {
			std::cout << "Type mismatch between the two ports!" << std::endl;
			return false;
		}
		return true;
	}

	/** \brief using a round robin schedule polls all its connections to see if someone has new data to be read */
	FlowStatus read(T & output) 
	{
		int tmp_index = manager_.rr_index_;
		int size = connectionsCount();
		std::shared_ptr<ConnectionT<T> > connection;
		for (int i = 0; i < size; ++i) {
			connection = getConnection(tmp_index % size);	
			if (connection->getData(output) == NEW_DATA) {
				manager_.rr_index_ = (tmp_index + 1) % size;
				return NEW_DATA;
			}
			tmp_index = (tmp_index + 1) % size;
		}
		// Never return OLD_DATA; TBF
		return NO_DATA;
	}	

private:
	/** \brief get the connection at position \p index */
	std::shared_ptr<ConnectionT<T> > getConnection(int index) {
		return manager_.getConnectionT<T>(index);
	}
	/** \brief connect the current port with \p other */
	bool connectToTyped(OutputPort<T> *other, ConnectionPolicy policy)  
	{
		if(task_ == other->task_)
			return false;
		
		std::shared_ptr<ConnectionBase> connection(makeConnection(this, other, policy));
		addConnection(connection);
		other->addConnection(connection);
		return true;		
	}
};

/**
 * \brief Class representing an output port containing data of type T
 */
template <class T>
class OutputPort: public PortBase
{
public:
	/** \brief simply call PortBase constructor */
	OutputPort(TaskContext * p, const char * name) : PortBase(p, name, true, false) {}

	const std::type_info& getTypeInfo() const override { return typeid(T); }

	bool connectTo(PortBase *other, ConnectionPolicy policy)
	{
		// check if the they have the same template
		if (getTypeInfo() == other->getTypeInfo())
		{
			// check if it is Output Port
			if (InputPort<T> *o = dynamic_cast<InputPort<T> *>(other)) {
				return connectToTyped(o,policy);
			} else {
				std::cout << "Destination port is not an OutputPort!\n";
				return false;
			}
		} else {
			std::cout << "Type mismatch between the two ports!\n";
			return false;
		}
		return true;
	}

	/** \brief writes in each of its connections \p input */ 
	void write(T & input) 
	{
		for (int i = 0; i < connectionsCount(); ++i) 
		{
			getConnection(i)->addData(input);
		}
	}
	
private:
	/** \brief get the connection at position \p index */
	std::shared_ptr<ConnectionT<T> > getConnection(int index) {
		return manager_.getConnectionT<T>(index);
	}
	/** \brief connect the current port with \p other */
	bool connectToTyped(InputPort<T> *other, ConnectionPolicy policy)  
	{
		if(task_ == other->task_)
			return false;
		std::shared_ptr<ConnectionBase> connection(makeConnection(other, this, policy));
		addConnection(connection);
		other->addConnection(connection);
		return true;		
	}
};


/**
 * Factory fo the connection policy
 */
template <class T>
ConnectionT<T> * makeConnection(InputPort<T> * a, OutputPort<T> * b, ConnectionPolicy p) {
	switch(p.lock_policy_) {
		case ConnectionPolicy::LOCKED:
			switch (p.data_policy_) {
				case ConnectionPolicy::DATA:		return new ConnectionDataL<T>(a,b,p);
				case ConnectionPolicy::BUFFER:		return new ConnectionBufferL<T>(a,b,p);
				case ConnectionPolicy::CIRCULAR:		return new ConnectionBufferL<T>(a,b,p);
			}
			break;
		case ConnectionPolicy::UNSYNC:
			switch (p.data_policy_) {
				case ConnectionPolicy::DATA:		return new ConnectionDataU<T>(a,b,p);
				case ConnectionPolicy::BUFFER:		return new ConnectionBufferU<T>(a,b,p);
				case ConnectionPolicy::CIRCULAR:		return new ConnectionBufferU<T>(a,b,p);
			}
			break;
		case ConnectionPolicy::LOCK_FREE:
			switch (p.data_policy_) {
				case ConnectionPolicy::DATA:
					p.buffer_size_ = 1;
					p.data_policy_ = ConnectionPolicy::CIRCULAR;
					return new ConnectionBufferL<T>(a,b,p);
					//return new ConnectionBufferLF<T>(a,b,p);
				case ConnectionPolicy::BUFFER:		return new ConnectionBufferL<T>(a,b,p);
				case ConnectionPolicy::CIRCULAR:		return new ConnectionBufferL<T>(a,b,p);	
				//case ConnectionPolicy::BUFFER:		return new ConnectionBufferLF<T>(a,b,p);
				//case ConnectionPolicy::CIRCULAR:		return new ConnectionBufferLF<T>(a,b,p);
			}
			break;
	}
	throw std::exception();
}

template <class T>
ConnectionT<T> * makeConnection(OutputPort<T> * a, InputPort<T> * b, ConnectionPolicy p) {
	return makeConnection(b,a,p);
}

/*
template <class T>
ConnectionT<T> * makeConnection(OutputPort<T> & a, InputPort<T> & b, ConnectionPolicy p) {
	return makeConnection(&b,&a,p);
}

template <class T>
ConnectionT<T> * makeConnection(InputPort<T> & a, OutputPort<T> & b, ConnectionPolicy p) {
	return makeConnection(&a,&b,p);
}
*/



// -------------------------------------------------------------------
// Execution
// -------------------------------------------------------------------

// http://www.orocos.org/stable/documentation/rtt/v2.x/api/html/classRTT_1_1base_1_1RunnableInterface.html
/**
 * \brief Interface class to execute the components
 */
class RunnableInterface
{
public:
	/** \brief Initialize the components members */
	virtual void init() = 0;
	/** \brief If the task is running execute uno step of the execution function */
	virtual void step() = 0;
	//virtual void loop() = 0;
	//virtual bool breakLoop() = 0;
	/** \brief when the task is stopped clear all the members */
	virtual void finalize() = 0;

	Activity * activity_ = 0;
};

/**
 * Concrete class to execture the components 
 */
class ExecutionEngine: public RunnableInterface {
public:
	/** \brief constructor to set the current TaskContext to be executed */
	ExecutionEngine(TaskContext *t) : task_(t) {}
	virtual void init() override;
	virtual void step() override;
	virtual void finalize() override;

	//virtual void loop() override;
	//virtual bool breakLoop() override;

protected:
	TaskContext * task_ = 0;
	bool stopped_;
};

/** \brief policy for executing the component */
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

/**
 * Base class for something that loops or is activated
 *
 * TODO: ownership
 * TODO: specialized for: Sequential and Parallel
 */
class Activity
{
public:
	/** \brief specify the execution policy and the RunnableInterface to be executed */
	Activity(SchedulePolicy policy, std::shared_ptr<RunnableInterface> r) 
		: policy_(policy), runnable_(r), active_(false), stopping_(false) {}
	/** \brief Start the activity */
	virtual void start() = 0;
	/** \brief Stop the activity */
	virtual void stop() = 0;
	//virtual void run(std::shared_ptr<RunnableInterface>);
	/** \brief */
	virtual bool isPeriodic() { return policy_.timing_policy_ != SchedulePolicy::TRIGGERED; }
	//virtual void execute() = 0;
	/** \brief in case of a TRIGGER activity starts one step of the execution */
	virtual void trigger() = 0;
	/** \brief main execution function */
	virtual void entry() = 0;
	/** \brief initialize the acrivity and the Engine */

	virtual void join() = 0;
	/** \brief initialize the acrivity and the Engine */

	bool isActive() { return active_; };
	SchedulePolicy::Policy getPolicyType() { return policy_.timing_policy_; }

protected:
	std::shared_ptr<RunnableInterface> runnable_;
	SchedulePolicy policy_;
	bool active_;
	std::atomic<bool> stopping_;
};

extern Activity * createSequentialActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r);
extern Activity * createParallelActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r);



/**
 * Manages all properties of a Task Context. Services is present because Task Context can have sub ones
 */
class Service
{
public:
	friend class PropertyBase;
	friend class AttributeBase;
	friend class OperationBase;
	friend class PortBase;
	
	/** \brief does nothing */
	Service(const char * n = "") : name_(n) {}

	#if 0
	/** \brief Create a new attribute and initialize it with a. 
	 *  If an attribute with the same name already exist return false */
	template<class T>
	bool addAttribute(std::string name, T &a) {
		if (attributes_[name])
			return false;
		Attribute<T> *attribute = new Attribute<T>(name, a);
		attributes_[name] = attribute;
		return true;
	}
	#endif

	/** \brief add an Atribute */
	bool addAttribute(AttributeBase *a);
	AttributeBase *getAttribute(std::string name) { return attributes_[name]; }

	//bool addProperty(PropertyBase *a);
	//PropertyBase *getProperty(std::string name) { return properties_[name]; }
	//template <class T>
	//T & getPropertyRef(std::string name); 

	template <class T>
	T & getAttributeRef(std::string name)
	{
		auto it = attributes_.find(name);
		if(it != attributes_.end())
			return it->second->as<T>();
		else
			throw std::exception();
	}

	/** \brief add a port to its list */
	bool addPort(PortBase *p);
	/** \brief return a port based on its name */
	PortBase *getPort(std::string name);

	/** \brief return the list of operations */
	std::list<std::shared_ptr<OperationBase> >& operations() { return operations_; }

	template <class T, class Y>
	void addOperator(const char * name, Y b, T  a)
	{
		typedef typename coco::impl::getfunctioner<T>::target target_t;
		target_t x = coco::impl::bindthis(a, b);
		operations_.push_back(std::shared_ptr<OperationBase>(new Operation<target_t>(this,x,name)));
		std::cout << "ops are " << operations_.size() << std::endl;
	}

	/// returns self as provider
	Service * provides() { return this; }

	/// check for sub services
	Service * provides(const char *x); 

	

private:
	std::map<std::string, PortBase* > ports_; 
	std::string name_;

	//std::list<AttributeBase*> attributes_; // all properties
	//std::map<std::string, PropertyBase*> properties_;
	std::map<std::string, AttributeBase*> attributes_;
	std::list<std::shared_ptr<OperationBase> > operations_;
	std::map<std::string, std::unique_ptr<Service> > subservices_;
};


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
	/** \brief set the activity that will manage the execution of this task */
	void setActivity(Activity *activity) { activity_ = activity; }
	/** \brief init the task attributes and properties */	
	/** \brief start the execution */

	virtual void init() {}
	virtual void start();
	/** \brief stop the execution of the component */
	virtual void stop();
	/** \brief in case of a TRIGGER task execute one step */
	virtual void triggerActivity();

	virtual const std::type_info & type() const = 0;

	void setName(std::string name) { name_ = name;}
	std::string getName() { return name_; }
	
	TaskState state_;

	std::shared_ptr<ExecutionEngine>  engine() const { return engine_; }
protected:
	/** \brief creates an ExecutionEngine object */
	TaskContext();

	friend class System;
	friend class ExecutionEngine;

	//virtual bool breakLoop(); Used to interrupt a task...difficult to implement
	
	/** \brief called every time before executing the component function */
	void prepareUpdate(){};
	/** \brief function to be overload by the user. It is called in the init phase */
	virtual void onConfig() = 0;
	/** \brief function to be overload by the user. It is the execution funciton */
	virtual void onUpdate() = 0;

	std::shared_ptr<ExecutionEngine> engine_; // ExecutionEngine is owned by activity

private:
	Activity * activity_; // TaskContext is owned by activity
	std::string name_;
};

template<class T>
class TaskContextT: public TaskContext
{
public:
	TaskContextT() {
		setName(type().name());
	}
	virtual const std::type_info & type() const override { return typeid(T); }
};

/**
 * Specification of the component, as created by the macro
 */
class ComponentSpec
{
public:
	typedef std::function<TaskContext * ()> makefx_t;
	ComponentSpec(const char * name, makefx_t fx);

	std::string name_;
	makefx_t fx_;
};

/**
 * Component Registry that is singleton per each exec or library. Then when the component library is loaded 
 * the singleton is replacedÃ¹ 
 *
 * @TODO add the addition of a full path to automatically add libraries
 */
class ComponentRegistry
{
public:
	/// creates a component by name
	static TaskContext * create(const char * name);

	/// adds a specification
	static void addSpec(ComponentSpec * s);

	/// adds a library
	static bool addLibrary(const char * l, const char * path );

	/// defines an alias. note that oldname should be present
	static void alias(const char * newname, const char * oldname);

private:
	static ComponentRegistry & get();

	TaskContext * create_(const char * name);

	void addSpec_(ComponentSpec *s);

	bool addLibrary_(const char * lib, const char * path );
	
	void alias_(const char * newname, const char * oldname);
	
	std::map<std::string,ComponentSpec*> specs;
	std::set<std::string> libs;
	std::map<std::string, std::shared_ptr<TaskContext> > contexts; // TODO but using mutex for protection

	//std::list<dlhandle> handles; // never relased
};


class Logger {
public:
	Logger(std::string name);
	~Logger();
private:
	std::string name_;
	clock_t start_time_;
};

class LoggerManager {
public:
	~LoggerManager();
	static LoggerManager* getInstance();
	void addTime(std::string id, double elapsed_time);
	void addServiceTime(std::string id, clock_t elapsed_time);
	void printStatistics();
	void printStatistic(std::string id);
	void resetStatistics();
private:
	LoggerManager(const char *log_file);
	std::ofstream log_file_;
	std::map<std::string, std::vector<double>> time_list_;
	std::map<std::string, std::vector<clock_t>> service_time_list_;
};

class CocoLauncher {
public:
    CocoLauncher(const char *config_file)
    	: config_file_(config_file) {}
    void createApp();
    void startApp();
private:
	void parseComponent(tinyxml2::XMLElement *comoponent);
	void parseConnection(tinyxml2::XMLElement *connection);

	const char *config_file_;
	tinyxml2::XMLDocument doc_;
	std::map<std::string, TaskContext *> tasks_;
};

}

/// registration macro, the only thing needed
#define EZORO_REGISTER(T) \
	coco::ComponentSpec T##_spec = { #T, [] () -> coco::TaskContext* { return new T(); } };
