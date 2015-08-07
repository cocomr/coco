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
#include <mutex>
#include <memory>
#include <iostream>
#include <deque>
#include <string>
#include <atomic>
#include <condition_variable>
#include <type_traits>
#include <typeinfo>
#include <boost/circular_buffer.hpp>
//#include <boost/lockfree/queue.hpp>
#include <boost/lexical_cast.hpp>

#include "tinyxml2.h"
#include "coco_profiling.h"
#include "coco_logging.h"

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

	namespace impl
	{
		/// Used to iterate over the keys of a map
		template <class Key, class Value>
		struct mapkeys_t
		{
			typedef std::map<Key,Value> map_t;

			struct iterator 
			{
				typedef Key value_t;
				typename map_t::iterator under;

				iterator(typename map_t::iterator  x) 
					: under(x) {}

				bool operator != (const iterator& o) const
				{ 
					return under != o.under;
				}

				value_t  operator*()   { return  under->first; }
				value_t * operator->() { return &under->first; }

				iterator & operator++()
				{ 
					++under; 
					return * this;
				}
				
				iterator operator++(int)
				{
					iterator x(*this);
					++under;
					return x;
				}
			};

			mapkeys_t(map_t & x) : x_(x) {}
			iterator begin() { return iterator(x_.begin()); }
			iterator end()   { return iterator(x_.end());   }

			map_t & x_;
		};

		template <class Key, class Value>
		mapkeys_t<Key,Value> mapkeys(std::map<Key,Value>& x)
		{
			return mapkeys_t<Key,Value>(x);
		}

		/// Used to iterate over the values of a map
		template <class Key, class Value>
		struct mapvalues_t
		{
			typedef std::map<Key,Value> map_t;

			struct iterator 
			{
				typedef Value value_t;
				typename map_t::iterator under;

				iterator(typename map_t::iterator  x) 
					: under(x) { }

				bool operator != (const iterator& o) const
				{ 
					return under != o.under;
				}

				value_t  operator*()   { return  under->second; }
				value_t * operator->() { return &under->second; }

				iterator & operator++()
				{
					++under;
					return * this;
				}
				iterator operator++(int)
				{
					iterator x(*this);
					++under;
					return x;
				}
			};
			mapvalues_t(map_t & x) : x_(x) {}

			iterator begin() { return iterator(x_.begin()); }
			iterator end()   { return iterator(x_.end());   }
			unsigned int size() const { return x_.size(); }
			
			map_t & x_;
		};
		template <class Key, class Value>
		mapvalues_t<Key,Value> mapvalues(std::map<Key,Value>& x)
		{
			return mapvalues_t<Key,Value>(x);
		}
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

namespace coco
{

class Port;
class TaskContext;
class PeerTask;
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

	void name(const std::string & n) { name_ = n; }

	/// associated documentation
	const std::string & doc() const { return doc_; }

	/// stores doc
	void doc(const std::string & d) { doc_= d; }

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

/// template spec of attribute
template <class T>
class Attribute: public AttributeBase
{
public:
	Attribute(TaskContext * p, std::string name, T & rvalue) 
		: AttributeBase(p, name), value_(rvalue) {}
	
	Attribute & operator = (const T & x)
	{ 
		value_ = x;
		return *this;
	}

	virtual const std::type_info & assig() override
	{ 
		return typeid(T);
	}

	virtual void setValue(const std::string &c_value) override 
	{
        value_ = boost::lexical_cast<T>(c_value);    
	}

	virtual void * value() override{
		return & value_;
	}

	virtual std::string toString() override
	{
		std::stringstream ss;
		ss << value_;
		return ss.str();
	}
private:
	T & value_;
};

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

	// utility
	template<class R, class U, class... Args, std::size_t... Is>
	auto bindthissub(R (U::*p)(Args...) const, U * pp, coco::impl::int_sequence<Is...>) -> decltype(std::bind(p, pp, std::placeholder_template<Is>{}...))
	{
		return std::bind(p, pp, std::placeholder_template<Is>{}...);
	}
	 
	// binds a member function only for the this pointer using std::bind
	template<class R, class U, class... Args>
	auto bindthis(R (U::*p)(Args...) const, U * pp) -> decltype(bindthissub(p,pp,coco::impl::make_int_sequence< sizeof...(Args) >{}))
	{
		return bindthissub(p,pp,coco::impl::make_int_sequence< sizeof...(Args) >{});
	}	
}

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
			//std::cout << typeid(Sig).name() << " vs expected " << assig().name() << std::endl;
			COCO_FATAL() << typeid(Sig).name() << " vs expected " << assig().name();
			throw std::exception();
		}
		else
		{			
			return *(std::function<Sig> *)(this->asfx());
		}
	}
	/// associated documentation
	const std::string & doc() const
	{ 
		return doc_;
	}
	/// stores doc
	void doc(const std::string & d)
	{ 
		doc_= d;
	}
	/// return the name of the operation
	const std::string & name() const
	{
		return name_;
	}
	/// set the name of the operation
	void name(const std::string & d)
	{ 
		name_ = d;
	}
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
	
	typedef typename coco::impl::getfunctioner<T>::fx Sig;
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

/**
 * Connection Policy
 */
struct ConnectionPolicy 
{
	enum Policy { DATA, BUFFER, CIRCULAR };
	enum LockPolicy { UNSYNC, LOCKED, LOCK_FREE};
	enum Transport { LOCAL, IPC};

	Policy data_policy_; /// type of data container
	LockPolicy lock_policy_; /// type of lock policy
	int buffer_size_; /// size of the data container
	bool init_ = false; 
	std::string name_id_;
	Transport transport_ = LOCAL;

	ConnectionPolicy() 
		: data_policy_(DATA), lock_policy_(LOCKED), buffer_size_(1), init_(false) {}
	ConnectionPolicy(Policy policiy, int buffer_size, bool blocking = false)
		: data_policy_(policiy), lock_policy_(LOCKED), buffer_size_(buffer_size), init_(false) {}
	/** Default constructor, default value:
	 *  @param data_policy_ = DATA
	 *	@param lock_policy_ = LOCKED
	 *  @param buffer_size_ = 1
	 * 	@param init_ = 1
	 */
	ConnectionPolicy(const std::string &policy, const std::string &lock_policy, const std::string &transport, const std::string &buffer_size) {
		// look here http://tinodidriksen.com/2010/02/16/cpp-convert-string-to-int-speed/
		buffer_size_ = atoi(buffer_size.c_str()); // boost::lexical_cast<int>(buffer_size);
		if (policy.compare("DATA") == 0)
			data_policy_ = DATA;
		else if (policy.compare("BUFFER") == 0)
			data_policy_ = BUFFER;
		else if (policy.compare("CIRCULAR") == 0)
			data_policy_ = CIRCULAR;
		if (lock_policy.compare("UNSYNC") == 0)
			lock_policy_ = UNSYNC;
		else if (lock_policy.compare("LOCKED") == 0)
			lock_policy_ = LOCKED;
		else if (lock_policy.compare("LOCK_FREE") == 0)
			lock_policy_ = LOCK_FREE;
		if (transport.compare("LOCAL") == 0)
			transport_ = LOCAL;
		else if (transport.compare("IPC") == 0)
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
	/// Constructor
	ConnectionBase(PortBase *in, PortBase *out, ConnectionPolicy policy)
		: input_(in), output_(out), policy_(policy), data_status_(NO_DATA) {}

	virtual ~ConnectionBase() {}

	/// @return NEW_DATA if new data is present in the Input port
	bool hasNewData() const
	{
		return data_status_ == NEW_DATA;
	}
    bool hasComponent(const std::string &name) const;

protected:
	/// Trigger the port to communicate new data is present
	void trigger();

	FlowStatus data_status_; /// status of the data in the container
	ConnectionPolicy policy_; /// policy for data management
	PortBase *input_ = 0; /// input port untyped
	PortBase *output_ = 0; /// output port untyped
};

template <class T>
class OutputPort;

template <class T>
class InputPort;


/// Template class to manage the type of the connection 
template <class T>
class ConnectionT : public ConnectionBase
{
public:
	/// Simply call ConnectionBase constructor
	ConnectionT(InputPort<T> *in, OutputPort<T> *out, ConnectionPolicy policy)
		: ConnectionBase((PortBase*)in, (PortBase*)out, policy) {}

	using ConnectionBase::ConnectionBase;
	/// If there is new data in the container retrive it
	virtual FlowStatus getData(T & data) = 0;
	/// Add data to the container. \n If the input port is of type event trigger it to wake up the execution
	virtual bool addData(T & data) = 0;
};


/// Specialized class for the type T to manage ConnectionPolicy::DATA ConnectionPolicy::LOCKED 
template <class T>
class ConnectionDataL : public ConnectionT<T>
{
public:
	/// Simply call ConnectionT<T> constructor
	ConnectionDataL(InputPort<T> *in, OutputPort<T> *out, ConnectionPolicy policy)
		: ConnectionT<T>(in,out, policy) {}
	/// Destroy remainig data
	~ConnectionDataL()
	{
		if(this->data_status_ == NEW_DATA) 
		{			
			value_.~T();  
		}
	}

	virtual FlowStatus getData(T & data) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_);
		if(destructor_policy_)
		{
			if(this->data_status_ == NEW_DATA) 
			{			
				data = value_; // copy => std::move
				value_.~T();   // destructor 
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
				data = value_;
				this->data_status_ = OLD_DATA;
				return NEW_DATA;
			} 
			else 
				return this->data_status_;
		}
	}

	virtual bool addData(T & input) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_);
		if(destructor_policy_)
		{
			if (this->data_status_ == NEW_DATA)
			{
				value_ = input;
			}
			else
			{
				new (&value_) T(input); 
				this->data_status_ = NEW_DATA; // mark
			}
		}
		else
		{
			if (this->data_status_ == NO_DATA)
			{
				new (&value_) T(input); 
				this->data_status_ = NEW_DATA; 
			}
			else
			{
				value_ = input;
				this->data_status_ = NEW_DATA; 
			}
		}
		if(this->input_->isEvent())
			this->trigger();
		return true;
	}
private:
	/// Specify wheter to keep old data, or to deallocate it
	bool destructor_policy_ = false;
	union
	{ 
		T value_;
	};
	std::mutex mutex_;
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


///Specialized class for the type T to manage ConnectionPolicy::DATA ConnectionPolicy::UNSYNC 
template <class T>
class ConnectionDataU : public ConnectionT<T>
{
public:
	/// Simply call ConnectionT<T> constructor
	ConnectionDataU(InputPort<T> *in, OutputPort<T> *out, ConnectionPolicy policy)
		: ConnectionT<T>(in,out, policy) {}
	/// Destroy remainig data
	~ConnectionDataU()
	{
		value_.~T();
	}

	virtual FlowStatus getData(T & data) override
	{
		if(this->data_status_ == NEW_DATA) 
		{
			data = value_;
			value_.~T();
			this->data_status_ = NO_DATA;
			return NEW_DATA;
		}  
		return NO_DATA;
	}

	virtual bool addData(T &input) override
	{
		if (this->data_status_ == NEW_DATA)
			value_.~T();
		new (&value_) T(input);
		this->data_status_ = NEW_DATA;

		if(this->input_->isEvent())
			this->trigger();
	
		return true;
	}
private:
	union
	{ 
		T value_;
	};
};


/// Specialized class for the type T to manage ConnectionPolicy::BUFFER/CIRCULAR_BUFFER ConnectionPolicy::UNSYNC 
template <class T>
class ConnectionBufferU : public ConnectionT<T>
{
public:
	/// Simply call ConnectionT<T> constructor
	ConnectionBufferU(InputPort<T> *in, OutputPort<T> *out, ConnectionPolicy policy)
		: ConnectionT<T>(in, out, policy) 
	{
		buffer_.resize(policy.buffer_size_);
	}
	/// Remove all data in the buffer and return the last value
	virtual FlowStatus getNewestData(T & data) 
	{
		bool status = false;
		while(!buffer_.empty())
		{
			status = true;
			data = buffer_.front();
			buffer_.pop_front();
		}
		return status ? NEW_DATA : NO_DATA;
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


/// Specialized class for the type T to manage ConnectionPolicy::BUFFER/CIRCULAR_BUFFER ConnectionPolicy::LOCKED
template <class T>
class ConnectionBufferL : public ConnectionT<T>
{
public:
	/// Simply call ConnectionT<T> constructor
	ConnectionBufferL(InputPort<T> *in, OutputPort<T> *out, ConnectionPolicy policy)
		: ConnectionT<T>(in, out, policy)
	{
		buffer_.resize(policy.buffer_size_);
	}
	/// Remove all data in the buffer and return the last value
	virtual FlowStatus getNewestData(T & data) 
	{
		std::unique_lock<std::mutex> mlock(this->mutex_);
		bool status = false;
		while(!buffer_.empty())
		{
			data = buffer_.front();
			buffer_.pop_front();
			status = true;
		}
		return status ? NEW_DATA : NO_DATA;
	}	

	virtual FlowStatus getData(T & data) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_);
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
		std::unique_lock<std::mutex> mlock(this->mutex_);
		if (buffer_.full())
		{
			if(this->policy_.data_policy_ == ConnectionPolicy::CIRCULAR)
			{
				buffer_.pop_front();
			}
			else
			{
				//COCO_ERR() << "BUFFER FULL!";
				return false;
			}
		} 
		buffer_.push_back(input);
		if(this->input_->isEvent())
			this->trigger();
		
		return true;
	}
private:
	boost::circular_buffer<T> buffer_;
	std::mutex mutex_;
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

/// Manage the connections of one PortBase
class ConnectionManager 
{
public:
	/// Set the RounRobin index to 0 and set its PortBase ptr owner
	ConnectionManager(PortBase * o)
		: rr_index_(0), owner_(o) {}

	/// Add a connection to \p connections_
	bool addConnection(std::shared_ptr<ConnectionBase> connection) 
	{
		connections_.push_back(connection);
		return true;
	}

	/// Return true if \p connections_ has at list one elemnt
	bool hasConnections() const
	{
		return connections_.size()  != 0;
	}
	/// Return the ConnectionBase connection inidicated by index if it exist
	std::shared_ptr<ConnectionBase> getConnection(int index)
	{
		if (index < connections_.size()) 
			return connections_[index];
		else
			return nullptr;
	}
    /// Return the ConnectionBase connection inidicated by name if it exist
    std::shared_ptr<ConnectionBase> getConnection(const std::string &name)
    {
        for (auto conn : connections_)
        {
            if (conn->hasComponent(name))
                return conn;
        }
        return nullptr;
    }

    /// Return the ConnectionT<T> connection inidicated by index if it exist
	template <class T>
    std::shared_ptr<ConnectionT<T> > getConnectionT(int index)
	{
		return std::static_pointer_cast<ConnectionT<T>>(getConnection(index));
	}
    /// Return the ConnectionT<T> connection inidicated by name if it exist
    template <class T>
    std::shared_ptr<ConnectionT<T> > getConnectionT(const std::string &name)
    {
        return std::static_pointer_cast<ConnectionT<T>>(getConnection(name));
    }
	/// Return the number of connections
	int connectionsSize() const
	{ 
		return connections_.size();
	}
protected:
	template <class T>
	friend class InputPort;
	template <class T>
	friend class OutputPort;

	int rr_index_; /// round robin index to poll the connection when reading data
	std::shared_ptr<PortBase> owner_; /// PortBase pointer owning this manager
	std::vector<std::shared_ptr<ConnectionBase>> connections_; /// List of ConnectionBase associate to \p owner_
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
	bool isConnected() const
	{ 
		return manager_.hasConnections();
	}	
	/// Return true if this port is of type event
	bool isEvent() const
	{ 
		return is_event_;
	}
	/// Return true if this port is an output port
	bool isOutput() const
	{
		return is_output_;
	};
	/// Trigger the task to notify new dara is present in the port
	void triggerComponent();
	//{
	//	task_->triggerActivity();
	//}
	/// associated documentation
	const std::string & doc() const
	{
		return doc_;
	}
	/// stores doc
	void doc(const std::string & d)
	{
		doc_= d;
	}
	/// Get the name of the port
	const std::string & name() const{
		return name_;
	}
	/// Set the name of the port
	void name(const std::string & d)
	{
		name_ = d;
	}
	/// Returns the number of connections associate to this port
	int connectionsCount() const
	{
		return manager_.connectionsSize();
	}
    /// Return the name of the task owing this connection
    std::string taskName() const;

protected:
	/// Add a connection to the ConnectionManager
	bool addConnection(std::shared_ptr<ConnectionBase> connection);
	
	template <class T>
	friend class InputPort;

	template <class T>
	friend class OutputPort;

	ConnectionManager manager_ = { this };
	std::shared_ptr<TaskContext> task_; /// Task using this port
	std::string name_;
	std::string doc_;
	bool is_event_;
	bool is_output_;
};

/// Class representing an input port containing data of type T
template <class T>
class InputPort: public PortBase 
{
public:
	/// Simply call PortBase constructor
	InputPort(TaskContext * p, const std::string &name, bool is_event = false) 
		: PortBase(p, name, false, is_event) {}
	/// Get the type of the Port variable
	const std::type_info &getTypeInfo() const override { return typeid(T); }
	/// Connect this port to an OutputPort
	bool connectTo(PortBase *other, ConnectionPolicy policy) 
	{
		// check if they have the same template
		if (getTypeInfo() == other->getTypeInfo())
		{
			// check if it is Output Port
			if (OutputPort<T> *o = dynamic_cast<OutputPort<T> *>(other))
			{
				return connectToTyped(o, policy);
			}
			else
			{
				//std::cout << "Destination port is not an OutputPort!" << std::endl;
				COCO_FATAL() << "Destination port: " << other->name_ << " is not an OutputPort!";
				return false;
			}
		} else {
			//std::cout << "Type mismatch between the two ports!" << std::endl;
			COCO_FATAL() << "Type mismatch between ports: " << this->name_ << " and " << other->name_;
			return false;
		}
		return true;
	}
	/// Using a round robin schedule polls all its connections to see if someone has new data to be read
	FlowStatus read(T & output) 
	{
		int tmp_index = manager_.rr_index_;
		int size = connectionsCount();
		std::shared_ptr<ConnectionT<T> > connection;
		for (int i = 0; i < size; ++i)
		{
			connection = getConnection(tmp_index % size);	
			if (connection->getData(output) == NEW_DATA)
			{
				manager_.rr_index_ = (tmp_index + 1) % size;
				return NEW_DATA;
			}
			tmp_index = (tmp_index + 1) % size;
		}
		return NO_DATA;
	}	

	/// Using a round robin schedule polls all its connections to see if someone has new data to be read
	FlowStatus readAll(std::vector<T> & output) 
	{
		T toutput; 
		output.clear();
		int size = connectionsCount();
		for (int i = 0; i < size; ++i)
		{
			if (getConnection(i)->getData(toutput) == NEW_DATA)
				output.push_back(toutput);
		}
		return output.empty() ? NO_DATA : NEW_DATA;
	}	

private:
	/// Get the connection at position \p index
	std::shared_ptr<ConnectionT<T> > getConnection(int index)
	{
		return manager_.getConnectionT<T>(index);
	}
	/// Connect the current port with \p other, 
	bool connectToTyped(OutputPort<T> *other, ConnectionPolicy policy)  
	{
		// Check that the two ports doesn't belong to the same task
		if(task_ == other->task_)
			return false;
		
		std::shared_ptr<ConnectionBase> connection(makeConnection(this, other, policy));
		addConnection(connection);
		other->addConnection(connection);
		return true;		
	}
};

/// Class representing an output port containing data of type T
template <class T>
class OutputPort: public PortBase
{
public:
	/// Simply call PortBase constructor
	OutputPort(TaskContext * p, const std::string &name) 
		: PortBase(p, name, true, false) {}
	/// Get the type of the Port variable
	const std::type_info& getTypeInfo() const override { return typeid(T); }
	/// Connect with an InputPort
	bool connectTo(PortBase *other, ConnectionPolicy policy)
	{
		// check if the they have the same template
		if (getTypeInfo() == other->getTypeInfo())
		{
			// check if it is Output Port
			if (InputPort<T> *o = dynamic_cast<InputPort<T> *>(other))
			{
				return connectToTyped(o,policy);
			}
			else
			{
				//std::cout << "Destination port is not an InputPort!\n";
				COCO_FATAL() << "Destination port: " << other->name_ << " is not an InputPort!";
				return false;
			}
		} else {
			//std::cout << "Type mismatch between the two ports!\n";
			COCO_FATAL() << "Type mismatch between ports: " << this->name_ << " and " << other->name_;
			return false;
		}
		return true;
	}

	/// Writes in each of its connections \p input
	void write(T & input) 
	{
		for (int i = 0; i < connectionsCount(); ++i) 
		{
			getConnection(i)->addData(input);
		}
	}

    void write(T &input, const std::string &name)
    {
        auto connection = getConnection(name);
        if (connection)
            connection->addData(input);
        else
            COCO_ERR() << "Connection " << name << " doesn't exist";
    }

private:
	/// Get the connection at position \p index
	std::shared_ptr<ConnectionT<T> > getConnection(int index)
	{
		return manager_.getConnectionT<T>(index);
	}

    /// Get the connection with component \p name
    std::shared_ptr<ConnectionT<T> > getConnection(const std::string &name)
    {
        return manager_.getConnectionT<T>(name);
    }
	/// Connect the current port with \p other
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

template <class T>
struct MakeConnection_t
{
	static ConnectionT<T> * fx(InputPort<T> * a, OutputPort<T> * b, ConnectionPolicy p)
	{
		switch(p.lock_policy_)
		{
			case ConnectionPolicy::LOCKED:
				switch (p.data_policy_)
				{
					case ConnectionPolicy::DATA:		return new ConnectionDataL<T>(a,b,p);
					case ConnectionPolicy::BUFFER:		return new ConnectionBufferL<T>(a,b,p);
					case ConnectionPolicy::CIRCULAR:	return new ConnectionBufferL<T>(a,b,p);
				}
				break;
			case ConnectionPolicy::UNSYNC:
				switch (p.data_policy_)
				{
					case ConnectionPolicy::DATA:		return new ConnectionDataU<T>(a,b,p);
					case ConnectionPolicy::BUFFER:		return new ConnectionBufferU<T>(a,b,p);
					case ConnectionPolicy::CIRCULAR:	return new ConnectionBufferU<T>(a,b,p);
				}
				break;
			case ConnectionPolicy::LOCK_FREE:
				switch (p.data_policy_)
				{
					case ConnectionPolicy::DATA:
						p.buffer_size_ = 1;
						p.data_policy_ = ConnectionPolicy::CIRCULAR;
						return new ConnectionBufferL<T>(a,b,p);
						//return new ConnectionBufferLF<T>(a,b,p);
					case ConnectionPolicy::BUFFER:		return new ConnectionBufferL<T>(a,b,p);
					case ConnectionPolicy::CIRCULAR:	return new ConnectionBufferL<T>(a,b,p);	
					//case ConnectionPolicy::BUFFER:	  return new ConnectionBufferLF<T>(a,b,p);
					//case ConnectionPolicy::CIRCULAR:	  return new ConnectionBufferLF<T>(a,b,p);
				}
				break;
		}
		throw std::exception();
	}
};

/// Factory fo the connection policy
template <class T>
ConnectionT<T> * makeConnection(InputPort<T> * a, OutputPort<T> * b, ConnectionPolicy p)
{
	return MakeConnection_t<T>::fx(a,b,p);
}

template <class T>
ConnectionT<T> * makeConnection(OutputPort<T> * a, InputPort<T> * b, ConnectionPolicy p)
{
	return MakeConnection_t<T>::fx(b,a,p);
}


/// [*] -> free -> writing -> ready -> reading -> free
///
/// and if discardold is true: ready -> writing
///
/// TODO: current version requires default constructor
template <class T>
class PooledChannel
{
public:
	std::vector<T> data_;
	std::list<T*> free_list_;
	std::list<T*> ready_list_;
	std::mutex mutex_;
	std::condition_variable read_ready_var_, write_ready_var_;
	bool discard_old_;

	PooledChannel(int n, bool adiscardold): data_(n),discard_old_(adiscardold)
	{
		for(int i = 0; i < data_.size(); ++i)
			free_list_.push_back(&data_[i]);
	}

	T* writerGet()
	{
		T * r = 0;
		{
			std::unique_lock<std::mutex> lk(mutex_);

			if(free_list_.empty())
			{
				// TODO check what happens when someone read, why cannot discard if there is only one element in read_list
				if(!discard_old_ || ready_list_.size() < 2)
				{
					if(!discard_old_)
						COCO_ERR() << "Queues are too small, no free, and only one (just prepared) ready\n";
						//std::cout << "Queues are too small, no free, and only one (just prepared) ready\n";
				    write_ready_var_.wait(lk, [this]{ return !this->free_list_.empty(); });
					r = free_list_.front();
					free_list_.pop_front();
				}
				else
				{
					// policy deleteold: kill the oldest
					r = ready_list_.front();
					ready_list_.pop_front();
				}
			}
			else
			{
				r = free_list_.front();
				free_list_.pop_front();
			}
		}
		return r;
	}

	void writeNotDone(T * x)
	{
		{
			std::unique_lock<std::mutex> lk(mutex_);
			if(x != 0)
				free_list_.push_back(x);
		}		
	}

	void writerDone(T * x)
	{
		{
			std::unique_lock<std::mutex> lk(mutex_);
			if(x != 0)
				ready_list_.push_back(x);
		}
		read_ready_var_.notify_one();
	}

	void readerDone(T * in)
	{
		{
			std::unique_lock<std::mutex> lk(mutex_);
			free_list_.push_back(in);
		}
		write_ready_var_.notify_one();
	}

	void readerGet(T * & out)
	{
		std::unique_lock<std::mutex> lk(mutex_);
	    read_ready_var_.wait(lk, [this]{return !this->ready_list_.empty();});
		out = ready_list_.front();
		ready_list_.pop_front();
	}
};

/// Port to be used with PooledChannel
template <class T>
class PortPooled
{
public:
	PortPooled() 
		: data_(0) {}
	PortPooled(std::shared_ptr<PooledChannel<T>> am, T * adata, bool aisreading) 
		: is_reading_(aisreading),data_(adata),manager_(am)
	{}
	PortPooled(const PortPooled & ) = delete;

	PortPooled(PortPooled && other) 
		: manager_(other.manager_),data_(other.data_),is_reading_(other.is_reading_)
	{
		other.data_ = 0;
	}

	T * data()
	{ 
		return data_;
	}

	const T * data() const 
	{ 
		return data_;
	}

	bool acquire(PooledChannel<T> & x)
	{
		commit();
		data_ = x.writerGet();
		manager_ = &x;
		is_reading_ = false;
		return data_ != 0;
	}

	// move assignment
	PortPooled & operator = (PortPooled && x)
	{
		commit();

		data_ = x.data_;
		manager_ = x.manager_;
		is_reading_ = x.is_reading_;
		return *this;
	}

	PortPooled & operator = (const PortPooled & x) = delete;

	void commit()
	{
		if(data_)
		{
			if(is_reading_)
				manager_->readerDone(data_);
			else
				manager_->writerDone(data_);
			data_ =0;
		}		
	}

	void discard()
	{
		if(data_)
		{
			if(is_reading_)
				manager_->readerDone(data_);
			else
				manager_->writerNotDone(data_);
			data_ =0;			
		}
	}

	~PortPooled()
	{
		commit();
	}

	bool is_reading_;
	T *  data_;
	std::shared_ptr<PooledChannel<T>> manager_;
};

// -------------------------------------------------------------------
// Execution
// -------------------------------------------------------------------

// http://www.orocos.org/stable/documentation/rtt/v2.x/api/html/classRTT_1_1base_1_1RunnableInterface.html

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
private:
	//std::shared_ptr<Activity> activity_;
	Activity * activity_;
};


/// Concrete class to execture the components  
class ExecutionEngine: public RunnableInterface
{
public:
	/// Constructor to set the current TaskContext to be executed
	ExecutionEngine(TaskContext *t) 
		: task_(t) {}
	virtual void init() override;
	virtual void step() override;
	virtual void finalize() override;
protected:
	TaskContext *task_;
	bool stopped_;
};

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
	Activity(SchedulePolicy policy, std::shared_ptr<RunnableInterface> r) 
		: policy_(policy), runnable_(r), active_(false), stopping_(false) {}
	/// Start the activity
	virtual void start() = 0;
	/// Stop the activity
	virtual void stop() = 0;

	virtual bool isPeriodic()
	{ 
		return policy_.timing_policy_ != SchedulePolicy::TRIGGERED;
	}
	/// In case of a TRIGGER activity starts one step of the execution
	virtual void trigger() = 0;
	/// Main execution function
	virtual void entry() = 0;

	virtual void join() = 0;

	bool isActive(){
		return active_;
	};

	SchedulePolicy::Policy getPolicyType()
	{
		return policy_.timing_policy_;
	}
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
	SequentialActivity(SchedulePolicy policy, std::shared_ptr<RunnableInterface> r = nullptr) 
		: Activity(policy, r) {}

	virtual void start() override;
	virtual void stop() override;
	virtual void trigger() override {};
	virtual void join() override;
protected:
	void entry() override;
}; 

/// Activity that run in its own thread
class ParallelActivity: public Activity
{
public:
	/// Simply call Activity constructor
	ParallelActivity(SchedulePolicy policy, std::shared_ptr<RunnableInterface> r = nullptr) 
		: Activity(policy, r) {}

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

extern Activity * createSequentialActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r);
extern Activity * createParallelActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r);


/// Manages all properties of a Task Context. Services is present because Task Context can have sub ones
class Service
{
public:
	friend class AttributeBase;
	friend class OperationBase;
	friend class PortBase;

	/// Initialize Service name
	Service(const std::string &n = "") : name_(n) {}

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
	coco::impl::mapkeys_t<std::string,AttributeBase*> getAttributeNames()
	{ 
		return coco::impl::mapkeys(attributes_);
	}
	/// Return a custo map to iterate over the values
	coco::impl::mapvalues_t<std::string,AttributeBase*> getAttributes()
	{ 
		return coco::impl::mapvalues(attributes_);
	}

	/// Add a port to its list
	bool addPort(PortBase *p);
	/// Return a port based on its name
	PortBase *getPort(std::string name);

	coco::impl::mapkeys_t<std::string,PortBase*> getPortNames()
	{ 
		return coco::impl::mapkeys(ports_);
	}
	coco::impl::mapvalues_t<std::string,PortBase*> getPorts()
	{
		return coco::impl::mapvalues(ports_);
	}

	/// Add an operation from an existing ptr
	bool addOperation(OperationBase *o);
	/// Create and add a new operation
	template <class Function, class Obj>
	bool addOperation(const std::string &name, Function  a, Obj b)
	{
		if (operations_[name])
		{
			COCO_ERR() << "An operation with name: " << name << " already exist";
			return false;
		}
		typedef typename coco::impl::getfunctioner<Function>::target target_t;
		auto x = coco::impl::bindthis(a, b);
		operations_[name] = new Operation<target_t>(this, name, x);
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

	coco::impl::mapkeys_t<std::string, OperationBase*> getOperationNames()
	{
		return coco::impl::mapkeys(operations_);
	}
	coco::impl::mapvalues_t<std::string, OperationBase*> getOperations(){
		return coco::impl::mapvalues(operations_);
	}

	/// Add a peer
	bool addPeer(TaskContext *p);
	std::list<TaskContext*> getPeers()
	{
		return peers_;
	}

	coco::impl::mapkeys_t<std::string,std::unique_ptr<Service> > getServiceNames()
	{ 
		return coco::impl::mapkeys(subservices_);
	}
	coco::impl::mapvalues_t<std::string,std::unique_ptr<Service> > getServices(){ 
		return coco::impl::mapvalues(subservices_);
	}
	/// returns self as provider
	Service * provides()
	{ 
		return this;
	}
	/// check for sub services
	Service * provides(const std::string &x); 

	void name(std::string name)
	{
		name_ = name;
	}
	const std::string & name() const
	{ 
		return name_;
	}

	void setInstantiationName(const std::string &name)
	{
		instantiation_name_ = name;
	}

	const std::string &instantiationName() const
	{
		return instantiation_name_;
	}
	
	void doc(std::string xdoc)
	{
		doc_ = xdoc;
	}
	const std::string & doc() const
	{
		return doc_;
	}
private:
	std::string name_;
	std::string instantiation_name_ = "";
	std::string doc_;

	std::list<TaskContext*> peers_;
	std::map<std::string, PortBase* > ports_; 
	std::map<std::string, AttributeBase*> attributes_;
	std::map<std::string, OperationBase*> operations_;
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
	friend class CocoLauncher;

	/// Set the activity that will manage the execution of this task
	void setActivity(Activity *activity) { activity_ = activity; }
	
	/// Start the execution
	virtual void start();
	/// Stop the execution of the component
	virtual void stop();
	/// In case of a TRIGGER task execute one step
	virtual void triggerActivity();

	virtual const std::type_info & type() const = 0;

	void setParent(TaskContext *t) 
	{ 
		if(!task_)
			task_ = t;
	}

	std::shared_ptr<ExecutionEngine>  engine() const
	{
		return engine_;
	}
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
	TaskContext *task_ = nullptr;
	Activity * activity_; // TaskContext is owned by activity
	TaskState state_;
};

template<class T>
class TaskContextT: public TaskContext
{
public:
	TaskContextT() 
	{
		name(type().name());
	}
	virtual const std::type_info & type() const override
	{
		return typeid(T);
	}
};

/// Class to create peer to be associated to taskcomponent
class PeerTask : public TaskContext
{
public:
	void init() {}
	void onUpdate() {}
};

template<class T>
class PeerTaskT : public PeerTask
{
public:
	PeerTaskT() 
	{
		//name(type().name());
		name(type().name() + std::to_string(peer_count_ ++));
	}
	virtual const std::type_info & type() const override
	{ 
		return typeid(T);
	}
	static int peer_count_;
};

template<class T>
int PeerTaskT<T>::peer_count_ = 0;

/// Specification of the component, as created by the macro 
class ComponentSpec
{
public:
	typedef std::function<TaskContext * ()> makefx_t;
	ComponentSpec(const std::string &name, makefx_t fx);

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
	static TaskContext * create(const std::string &name);
	/// adds a specification
	static void addSpec(ComponentSpec * s);
	/// adds a library
	static bool addLibrary(const std::string &l, const std::string &path );
	/// defines an alias. note that oldname should be present
	static void alias(const std::string &newname, const std::string &oldname);

    static impl::mapkeys_t<std::string, ComponentSpec *> componentsName();

private:
	static ComponentRegistry & get();

	TaskContext * create_(const std::string &name);

	void addSpec_(ComponentSpec *s);

	bool addLibrary_(const std::string &lib, const std::string &path );
	
	void alias_(const std::string &newname, const std::string &oldname);

    impl::mapkeys_t<std::string, ComponentSpec *> componentsName_();
	
	std::map<std::string,ComponentSpec*> specs;
	std::set<std::string> libs;
	std::map<std::string, std::shared_ptr<TaskContext> > contexts; // TODO but using mutex for protection
};

} // end of namespace coco


/// registration macro, the only thing needed
#define EZORO_REGISTER(T) \
    coco::ComponentSpec T##_spec = { #T, [] () -> coco::TaskContext* {return new T(); } };
