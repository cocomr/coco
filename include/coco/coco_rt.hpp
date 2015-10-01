#pragma once
#include "coco/coco_core.hpp"

namespace coco
{




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




template <class T>
struct MakeConnection
{
	static ConnectionT<T> * fx(InputPort<T> * input, OutputPort<T> * output,
							   ConnectionPolicy policy)
	{
		switch(policy.lock_policy_)
		{
			case ConnectionPolicy::LOCKED:
				switch (policy.data_policy_)
				{
					case ConnectionPolicy::DATA:		return new ConnectionDataL<T>(input, output, policy);
					case ConnectionPolicy::BUFFER:		return new ConnectionBufferL<T>(input, output, policy);
					case ConnectionPolicy::CIRCULAR:	return new ConnectionBufferL<T>(input, output, policy);
				}
				break;
			case ConnectionPolicy::UNSYNC:
				switch (policy.data_policy_)
				{
					case ConnectionPolicy::DATA:		return new ConnectionDataU<T>(input, output, policy);
					case ConnectionPolicy::BUFFER:		return new ConnectionBufferU<T>(input, output, policy);
					case ConnectionPolicy::CIRCULAR:	return new ConnectionBufferU<T>(input, output, policy);
				}
				break;
			case ConnectionPolicy::LOCK_FREE:
				switch (policy.data_policy_)
				{
					case ConnectionPolicy::DATA:
						policy.buffer_size_ = 1;
						policy.data_policy_ = ConnectionPolicy::CIRCULAR;
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



extern Activity * createSequentialActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r);
extern Activity * createParallelActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r);


}
