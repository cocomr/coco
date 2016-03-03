/**
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
#include "core.h"
#include <boost/circular_buffer.hpp>

namespace coco
{

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

	virtual const std::type_info & asSig() override
	{ 
		return typeid(T);
	}

	virtual void setValue(const std::string &value) override 
	{
        value_ = boost::lexical_cast<T>(value);    
	}

	virtual void * value() override
	{
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



template <class Q>
class Attribute<std::vector<Q> >:  public AttributeBase
{
public:
	using T = std::vector<Q>;
	Attribute(TaskContext * p, std::string name, T & rvalue) 
		: AttributeBase(p, name), value_(rvalue) {}
	
	Attribute & operator = (const T & x)
	{ 
		value_ = x;
		return *this;
	}

	virtual const std::type_info & asSig() override
	{ 
		return typeid(T);
	}

	virtual void setValue(const std::string &value) override 
	{
		std::vector<Q> nv;
		for(auto p : coco::stringutil::splitter(value,','))
    	{
    		nv.push_back(boost::lexical_cast<Q>(p));
    	}
    	value_ = nv;
	}

	virtual void * value() override
	{
		return & value_;
	}

	virtual std::string toString() override
	{
		std::stringstream ss;
		for(int i = 0; i < value_.size(); i++)
		{
			if(i > 0)
				ss << ',';
			ss << value_[i];
		}
		return ss.str();
	}
private:
	T & value_;
};

/**
 * Operator Class specialized for T as function holder (anything) 
 */
template <class T>
class Operation: public OperationBase
{
public:
	Operation(TaskContext* task, const std::string &name, const std::function<T> & fx)
		: OperationBase(task, name), fx_(fx)
	{}
	template<class Function, class Obj>
	Operation(TaskContext* task, const std::string &name, Function fx, Obj obj)
		: Operation(task, name, coco::impl::bind_this(fx, obj))
	{}
	
	typedef typename coco::impl::get_functioner<T>::fx Sig;
 	/// return the signature of the function
	virtual const std::type_info &asSig() override
	{
		return typeid(Sig);
	}

	virtual void *asFx() override
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
	std::function<T> fx_;
};

template <class T>
class ConnectionT;
template <class T>
class OutputPort;
// Class representing an input port containing data of type T
template <class T>
class InputPort: public PortBase 
{
public:
	/// Simply call PortBase constructor
	InputPort(TaskContext * p, const std::string &name, bool is_event = false) 
		: PortBase(p, name, false, is_event) {}
	/// Get the type of the Port variable
	const std::type_info &typeInfo() const override { return typeid(T); }
	/// Connect this port to an OutputPort
	bool connectTo(PortBase *other, ConnectionPolicy policy) override
	{
		// check if they have the same template
		if (typeInfo() == other->typeInfo())
		{
			// check if it is Output Port
			if (OutputPort<T> *output_port = dynamic_cast<OutputPort<T> *>(other))
			{
				return connectToTyped(output_port, policy);
			}
			else
			{
				COCO_FATAL() << "Destination port: " << other->name() << " is not an OutputPort!";
				return false;
			}
		} else {
			COCO_FATAL() << "Type mismatch between ports: " << this->name()
						 << " and " << other->name();
			return false;
		}
		return true;
	}
	/// Using a round robin schedule polls all its connections to see if someone has new data to be read
	FlowStatus read(T & output) 
	{
		int tmp_index = manager_.roundRobinIndex();
		int size = connectionsCount();
		std::shared_ptr<ConnectionT<T> > conn;
		for (int i = 0; i < size; ++i)
		{
			conn = connection(tmp_index % size);	
			if (conn->data(output) == NEW_DATA)
			{
				manager_.setRoundRobinIndex((tmp_index + 1) % size);
				return NEW_DATA;
			}
			tmp_index = (tmp_index + 1) % size;
		}
		return NO_DATA;
	}	

	/// Using a round robin schedule polls all its connections to see if someone has new data to be read
    /// If a connection is a buffer read all data in the buffer!
	FlowStatus readAll(std::vector<T> & output) 
	{
		T toutput; 
		output.clear();
		int size = connectionsCount();
		for (int i = 0; i < size; ++i)
		{
            while (connection(i)->data(toutput) == NEW_DATA)
				output.push_back(toutput);
		}
		return output.empty() ? NO_DATA : NEW_DATA;
	}	
private:
	/// Get the connection at position \p index
	std::shared_ptr<ConnectionT<T> > connection(int index)
	{
		return std::static_pointer_cast<ConnectionT<T> >(manager_.connection(index));
	}
	/// Connect the current port with \p other, 
	bool connectToTyped(OutputPort<T> *other, ConnectionPolicy policy)  
	{
		// Check that the two ports doesn't belong to the same task
		if(task_.get() == other->task())
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
	const std::type_info& typeInfo() const override { return typeid(T); }
	/// Connect with an InputPort
	bool connectTo(PortBase *other, ConnectionPolicy policy) override
	{
		// check if the they have the same template
		if (typeInfo() == other->typeInfo())
		{
			// check if it is Output Port
			if (InputPort<T> *o = dynamic_cast<InputPort<T> *>(other))
			{
				return connectToTyped(o,policy);
			}
			else
			{
				//std::cout << "Destination port is not an InputPort!\n";
				COCO_FATAL() << "Destination port: " << other->name() << " is not an InputPort!";
				return false;
			}
		} else {
			//std::cout << "Type mismatch between the two ports!\n";
			COCO_FATAL() << "Type mismatch between ports: " << this->name() << " and " << other->name();
			return false;
		}
		return true;
	}

	/// Writes in each of its connections \p input
	void write(T & input) 
	{
		for (int i = 0; i < connectionsCount(); ++i) 
		{
			connection(i)->addData(input);
		}
	}

    void write(T &input, const std::string &name)
    {
        auto conn = connection(name);
        if (conn)
            conn->addData(input);
        else
            COCO_ERR() << "Connection " << name << " doesn't exist";
    }

private:
	/// Get the connection at position \p index
	std::shared_ptr<ConnectionT<T> > connection(int index)
	{
		return std::static_pointer_cast<ConnectionT<T> >(manager_.connection(index));
	}
    /// Get the connection with component \p name
    std::shared_ptr<ConnectionT<T> > connection(const std::string &name)
    {
        return std::static_pointer_cast<ConnectionT<T> >(manager_.connection(name));
    }
	/// Connect the current port with \p other
	bool connectToTyped(InputPort<T> *other, ConnectionPolicy policy)
	{
		if(task_.get() == other->task())
			return false;
		std::shared_ptr<ConnectionBase> connection(makeConnection(other, this, policy));
		addConnection(connection);
		other->addConnection(connection);
		return true;		
	}
};

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
	virtual FlowStatus data(T & data) = 0;
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

	virtual FlowStatus data(T & data) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_);
		if(this->data_status_ == NEW_DATA)
		{
			data = value_; // copy => std::move
			this->data_status_ = OLD_DATA;
			if (destructor_policy_)
			{
				value_.~T();   // destructor 
				this->data_status_ = NO_DATA;	
			}
			if (this->input_->isEvent())
				this->removeTrigger();
			return NEW_DATA;
		}
		return this->data_status_; 
	}

	virtual bool addData(T & input) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_);
		FlowStatus old_status = this->data_status_;
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

		if(this->input_->isEvent() && old_status != NEW_DATA)
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

	virtual FlowStatus data(T & data) override
	{
		if(this->data_status_ == NEW_DATA) 
		{
			data = value_;
			value_.~T();
			this->data_status_ = NO_DATA;
			if (this->input_->isEvent())
				this->removeTrigger();
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
		buffer_.resize(policy.buffer_size);
	}
	/// Remove all data in the buffer and return the last value
	virtual FlowStatus newestData(T & data) 
	{
		bool status = false;
		while(!buffer_.empty())
		{
			status = true;
			data = buffer_.front();
			buffer_.pop_front();
		}
		if (status)
		{
			if (this->input_->isEvent())
				this->removeTrigger();
		}
		return status ? NEW_DATA : NO_DATA;
	}	

	virtual FlowStatus data(T & data) override
	{
		if(!buffer_.empty())
		{
			data = buffer_.front();
			buffer_.pop_front();
			if (this->input_->isEvent())
				this->removeTrigger();
			return NEW_DATA;
		}
		else 
			return NO_DATA;
	}

	virtual bool addData(T &input) override
	{
		if (buffer_.full())
		{
			if(this->policy_.data_policy == ConnectionPolicy::CIRCULAR)
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
		//buffer_.resize(policy.buffer_size);
		buffer_.set_capacity(policy.buffer_size);
	}
	/// Remove all data in the buffer and return the last value
	virtual FlowStatus newestData(T & data) 
	{
		std::unique_lock<std::mutex> mlock(this->mutex_);
		bool status = false;
		while(!buffer_.empty())
		{
			data = buffer_.front();
			buffer_.pop_front();
			status = true;
		}
		if (status)
		{
			if (this->input_->isEvent())
				this->removeTrigger();
		}
		return status ? NEW_DATA : NO_DATA;
	}	

	virtual FlowStatus data(T & data) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_);
		if(!buffer_.empty())
		{
			data = buffer_.front();
			buffer_.pop_front();
			if (this->input_->isEvent())
				this->removeTrigger();

			return NEW_DATA;
		} 
		return NO_DATA;
	}

	virtual bool addData(T &input) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_);
		bool do_trigger = true;
		if (buffer_.full())
		{
			if(this->policy_.data_policy == ConnectionPolicy::CIRCULAR)
			{
				buffer_.pop_front();
				do_trigger = false;
			}
			else
			{
				return false;
			}
		}
		buffer_.push_back(input);

		if(this->input_->isEvent() && do_trigger)
		{
			this->trigger();
		}
		
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
		queue_.reserve(policy.buffer_size);
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




template <class T>
struct MakeConnection
{
	static ConnectionT<T> * fx(InputPort<T> * input, OutputPort<T> * output,
							   ConnectionPolicy policy)
	{
		switch(policy.lock_policy)
		{
			case ConnectionPolicy::LOCKED:
				switch (policy.data_policy)
				{
					case ConnectionPolicy::DATA:		return new ConnectionDataL<T>(input, output, policy);
					case ConnectionPolicy::BUFFER:		return new ConnectionBufferL<T>(input, output, policy);
					case ConnectionPolicy::CIRCULAR:	return new ConnectionBufferL<T>(input, output, policy);
				}
				break;
			case ConnectionPolicy::UNSYNC:
				switch (policy.data_policy)
				{
					case ConnectionPolicy::DATA:		return new ConnectionDataU<T>(input, output, policy);
					case ConnectionPolicy::BUFFER:		return new ConnectionBufferU<T>(input, output, policy);
					case ConnectionPolicy::CIRCULAR:	return new ConnectionBufferU<T>(input, output, policy);
				}
				break;
			case ConnectionPolicy::LOCK_FREE:
				switch (policy.data_policy)
				{
					case ConnectionPolicy::DATA:
						policy.buffer_size = 1;
						policy.data_policy = ConnectionPolicy::CIRCULAR;
						return new ConnectionBufferL<T>(input, output, policy);
						//return new ConnectionBufferLF<T>(input, output, policy);
					case ConnectionPolicy::BUFFER:		return new ConnectionBufferL<T>(input, output, policy);
					case ConnectionPolicy::CIRCULAR:	return new ConnectionBufferL<T>(input, output, policy);	
					//case ConnectionPolicy::BUFFER:	  return new ConnectionBufferLF<T>(input, output, policy);
					//case ConnectionPolicy::CIRCULAR:	  return new ConnectionBufferLF<T>(input, output, policy);
				}
				break;
		}
		return nullptr;
		//throw std::exception();
	}
};

/// Factory fo the connection policy
template <class T>
ConnectionT<T> * makeConnection(InputPort<T> * input, OutputPort<T> * output,
								ConnectionPolicy policy)
{
#ifdef PORT_INTROSPECTION
	//MakeConnection<T>::fx(input, ) given the task inspection create a new port for him and go
#endif	
	return MakeConnection<T>::fx(input, output, policy);
}

template <class T>
ConnectionT<T> * makeConnection(OutputPort<T> * output, InputPort<T> * input,
								ConnectionPolicy policy)
{
#ifdef PORT_INTROSPECTION
	//MakeConnection<T>::fx(input, ) given the task inspection create a new port for him and go
#endif	
	return MakeConnection<T>::fx(input, output, policy);
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

}
