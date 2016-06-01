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

/*! \brief Specialization for \ref AttributeBase, each attribute is templated on the type it contains. 
 */
template <class T>
class Attribute: public AttributeBase
{
public:
	/*! \brief Base constructor that associated the attribute at the task 
	 *  and the task variable at the at attribute.
	 *  \param task The component that contains the attribute.
	 *  \param name The name of the attribute.
	 *  \param rvalue The reference to the variable associated to the attribute
	 */
    Attribute(TaskContext *task, std::string name, T &rvalue)
        : AttributeBase(task, name), value_(rvalue)
    {}
	/*! \brief Allows to set the attribute value from a variable of the same type of the contained one.
     *  \param value The value to be associated to the attribute
	 */
    Attribute & operator = (const T &value)
	{ 
		value_ = value;
		return *this;
	}
    /*!
     * \return the signature of the type embedded in the attribute.
     */
    virtual const std::type_info &asSig() override
	{ 
		return typeid(T);
	}
    /*!
     * \brief Used to set the value of the attribute and thus of the undeline variable.
     * \param value The value is read from the XML config file, so it is a string that
     * it is automatically converted to the type of the attribute. Only basic types are
     * allowed
     */
	virtual void setValue(const std::string &value) override 
	{
        value_ = boost::lexical_cast<T>(value);    
	}
    /*!
     * \return a void ptr to the value variable. Can be used togheter with asSig to
     * retreive the variable
     */
    virtual void *value() override
	{
		return & value_;
	}
    /*!
     * \return serialize the value of the attribute into a string
     */
	virtual std::string toString() override
	{
		std::stringstream ss;
		ss << value_;
		return ss.str();
	}
private:
    T &value_;
};

/*! \brief Allows the automatical creation of an attribute containing a vector of dynamic size.
 *
 */
template <class Q>
class Attribute<std::vector<Q> >:  public AttributeBase
{
public:
	using T = std::vector<Q>;
	/*! \brief Base constructor that associated the attribute at the task 
	 *  and the task variable at the at attribute.
	 *  \param task The component that contains the attribute.
	 *  \param name The name of the attribute.
	 *  \param rvalue The reference to the variable (vector) associated to the attribute
	 */
    Attribute(TaskContext *task, std::string name, T &rvalue)
        : AttributeBase(task, name), value_(rvalue)
    {}
	/*! \brief Allows to set the attribute value from a variable of the same type of the contained one.
	 *  \param value The value to be associated to the attribute.
	 */
    Attribute &operator =(const T &x)
	{ 
		value_ = x;
		return *this;
	}
    /*!
     * \return the signature of the type embedded in the attribute.
     */
    virtual const std::type_info &asSig() override
	{ 
		return typeid(T);
	}
	/*! \brief Set the valus of the vector. The supported format is CSV, with or without spaces.
	 *  \param value A CSV line containing the values to be associated to the vector.
	 */
	virtual void setValue(const std::string &value) override 
	{
		std::string new_value = value;
		auto pos = std::find(new_value.begin(), new_value.end(), ' ');
		while (pos != new_value.end())
		{
			new_value.erase(pos);
			pos = std::find(new_value.begin(), new_value.end(), ' ');
		}
		std::vector<Q> nv;
		for(auto p : coco::stringutil::splitter(new_value,','))
    	{
    		nv.push_back(boost::lexical_cast<Q>(p));
    	}
    	value_ = nv;
	}
    /*!
     * \return a void ptr to the value variable. Can be used togheter with asSig to
     * retreive the variable
     */
    virtual void *value() override
	{
        return &value_;
	}
    /*!
     * \return serialize the value of the attribute into a string
     */
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
    T &value_;
};

/*! \brief Operator Class specialized for T as function holder (anything) 
 */
template <class T>
class Operation: public OperationBase
{
public:
	/*! \brief Base constructor, associated to the operation a base function.
	 *  \param task The compoenent containing the operations.
	 *  \param name The operation name.
	 *  \param fx The function associated to the operations.
	 */
    Operation(TaskContext *task, const std::string &name, const std::function<T> &fx)
        : OperationBase(task, name), fx_(fx)
	{}
	/*! \brief Allows to associate to the operation a Class function, with the object.
	 *  \param task The compoenent containing the operations.
	 *  \param name The operation name.
	 *  \param fx The class function associated to the operations.
	 *  \param obj The obj on which to call the function.
	 */
	template<class Function, class Obj>
    Operation(TaskContext *task, const std::string &name, Function fx, Obj obj)
        : Operation(task, name, coco::impl::bind_this(fx, obj))
	{}
	
private:
	typedef typename coco::impl::get_functioner<T>::fx Sig;
 	/*!
 	 * \return the signature of the function
 	 */
	virtual const std::type_info &asSig() override
	{
		return typeid(Sig);
	}
	/*!
	 *  \return The function as void *. Used when invoking the function.
	 */
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
class ConnectionManagerT;
template <class T>
class ConnectionManagerDefault;
template <class T>
class ConnectionT;
template <class T>
class OutputPort;
/*! \brief Class representing an input port containing data of type T
 */
template <class T>
class InputPort: public PortBase 
{
public:
	/*! \brief Simply call PortBase constructor
	 *  \param task The task containing this port.
	 *  \param name The name of the port. Used to identify it. A component cannot have multiple port with the same name
	 *  \param is_event Wheter the port is an event port. Event port are used to create triggered components.
	 */
    InputPort(TaskContext *task, const std::string &name, bool is_event = false)
        : PortBase(task, name, false, is_event)
    {
        createConnectionManager(ConnectionManager::DEFAULT);
    }

	const std::type_info &typeInfo() const override { return typeid(T); }
	
    bool connectTo(std::shared_ptr<PortBase> &other, ConnectionPolicy policy) override
	{
        assert(this->manager_ && "Before connecting a port, instantiate the ConnectionManager");

        // check if they have the same template
		if (typeInfo() == other->typeInfo())
		{
			// check if it is Output Port
            if (auto output_port = std::dynamic_pointer_cast<OutputPort<T> >(other))
			{
				return connectToTyped(output_port, policy);
			}
			else
			{
				COCO_FATAL() << "Destination port: " << other->name() << " is not an OutputPort!";
				return false;
			}
		}
		else
		{
			COCO_FATAL() << "Type mismatch between ports: " << this->name()
						 << " and " << other->name();
			return false;
		}
		return true;
	}
	/*! \brief Using a round robin schedule polls all its connections to see if someone has new data to be read
	 *	\param output The variable where to store the result. If no new data is available the value of \ref output is not changed.
	 *  \return The read result, wheter new data is present
	 */
    FlowStatus read(T &data)
	{
        assert(this->manager_ && "Before reading a port, instantiate the ConnectionManager");
        return std::static_pointer_cast<ConnectionManagerT<T> >(this->manager_)->read(data);
	}	

    /*! \brief It polls all the connections and read all the data at the same time, storing the result in a vector.
     *  \param output The vector is cleared and data is pushed in it,
     *         so at the end the size of the vector will be euqal to the number of connections with new data.
     *  \return Wheter at least one connection had new data.
     *          Basically wheter the lenght of the vector was greater than zero.
     */
    FlowStatus readAll(std::vector<T> &data)
	{
        assert(this->manager_ && "Before reading a port, instantiate the ConnectionManager");
        return std::static_pointer_cast<ConnectionManagerT<T> >(this->manager_)->readAll(data);
	}
	/*!
     * \return True if the port has incoming new data;
     */
    bool hasNewData() const { return this->queueLength() > 0; }
private:
	friend class OutputPort<T>;
    friend class GraphLoader;

	/*!
	 * \param index Get the connection with the given index.
	 */
//	std::shared_ptr<ConnectionT<T> > connection(int index)
//	{
//        return std::static_pointer_cast<ConnectionT<T> >(manager_.connection(index));
//	}
	/*! \brief Called by \ref connectTo(), does the actual connection once the type have been checked.
	 *  \param other The other port to which to connect.
	 *  \param policy The connection policy.
	 *  \return Wheter the connection succeded. It can only fails wheter we are connecting two port of the same component.
	 */ 
    bool connectToTyped(std::shared_ptr<OutputPort<T> > &other, ConnectionPolicy policy)
	{
		// Check that the two ports doesn't belong to the same task
        if(task_ == other->task())
			return false;
		
        std::shared_ptr<ConnectionBase> connection(makeConnection(
                                                       std::static_pointer_cast<InputPort<T> >(this->sharedPtr()),
                                                       other, policy));
		addConnection(connection);
		other->addConnection(connection);
		return true;		
	}

    void createConnectionManager(ConnectionManager::ConnectionManagerType type)
    {
        // TODO check if there is already an existing connection manager?
        if (this->manager_)
            return;
        switch(type)
        {
            case ConnectionManager::DEFAULT:
                this->manager_ = std::make_shared<ConnectionManagerDefault<T> >();
                break;
            case ConnectionManager::FARM:

                break;
            default:
                COCO_FATAL() << "Invalid ConnectionManagerType " << type;
                break;
        }
    }

};

/*! \brief Class representing an output port containing data of type T
 */
template <class T>
class OutputPort: public PortBase
{
public:
	 /*! \brief Simply call PortBase constructor
	 *  \param task The task containing this port.
	 *  \param name The name of the port. Used to identify it. A component cannot have multiple port with the same name
	 *  \param is_event Wheter the port is an event port. Event port are used to create triggered components.
	 */
    OutputPort(TaskContext *task, const std::string &name)
        : PortBase(task, name, true, false)
    {
        createConnectionManager(ConnectionManager::DEFAULT);
    }
	
	const std::type_info& typeInfo() const override { return typeid(T); }
	
    bool connectTo(std::shared_ptr<PortBase> &other, ConnectionPolicy policy) override
	{
		// check if the they have the same template
		if (typeInfo() == other->typeInfo())
		{
			// check if it is Output Port
            if (auto o = std::dynamic_pointer_cast<InputPort<T> >(other))
			{
                return connectToTyped(o, policy);
			}
			else
			{
				COCO_FATAL() << "Destination port: " << other->name() << " is not an InputPort!";
				return false;
			}
        } else
        {
			COCO_FATAL() << "Type mismatch between ports: " << this->name() << " and " << other->name();
			return false;
		}
		return true;
	}
	/*! \brief Write in each connection associated with this port.
	 *  \param input The value to be written in each output connection.
	 */
    bool write(const T &data)
	{
        return std::static_pointer_cast<ConnectionManagerT<T> >(manager_)->write(data);
	}
	/*! \brief Write only in a specific port contained in the task named \ref name.
	 *  \param input The value to be written.
	 *  \param name The name of a taks.
	 */
    bool write(const T &data, const std::string &task_name)
    {
        return std::static_pointer_cast<ConnectionManagerT<T> >(manager_)->write(data, task_name);
    }

private:
	friend class InputPort<T>;
    friend class GraphLoader;

//	std::shared_ptr<ConnectionT<T> > connection(int index)
//	{
//        return std::static_pointer_cast<ConnectionT<T> >(manager_.connection(index));
//	}

//    std::shared_ptr<ConnectionT<T> > connection(const std::string &name)
//    {
//        return std::static_pointer_cast<ConnectionT<T> >(manager_.connection(name));
//    }
	/*! \brief Called by \ref connectTo(), does the actual connection once the type have been checked.
	 *  \param other The other port to which to connect.
	 *  \param policy The connection policy.
	 *  \return Wheter the connection succeded. It can only fails wheter we are connecting two port of the same component.
	 */ 
    bool connectToTyped(std::shared_ptr<InputPort<T> > &other, ConnectionPolicy policy)
	{
        if(task_ == other->task())
        {
            COCO_FATAL() << "Trying to connect two ports of the same task " << task_->instantiationName();
            return false;
        }
        std::shared_ptr<ConnectionBase> connection(makeConnection(
                                                       other,
                                                       std::static_pointer_cast<OutputPort<T> >(this->sharedPtr()),
                                                       policy));
		addConnection(connection);
		other->addConnection(connection);
		return true;		
	}

    void createConnectionManager(ConnectionManager::ConnectionManagerType type)
    {
        if (this->manager_)
            return;
        // TODO check if there is already an existing connection manager?
        switch(type)
        {
            case ConnectionManager::DEFAULT:
                this->manager_ = std::make_shared<ConnectionManagerDefault<T> >();
                break;
            case ConnectionManager::FARM:

                break;
            default:
                COCO_FATAL() << "Invalid ConnectionManagerType " << type;
                break;
        }
    }
};

template <class T>
class ConnectionManagerT : public ConnectionManager
{
public:
    virtual FlowStatus read(T &data) = 0;
    virtual FlowStatus readAll(std::vector<T> &data) = 0;
    virtual bool write(const T &data) = 0;
    virtual bool write(const T &data, const std::string &task_name) = 0;
    std::shared_ptr<ConnectionT<T> > connection(unsigned idx)
    {
        assert(idx < connections_.size() && "trying to access a connection out of bound");
        return std::static_pointer_cast<ConnectionT<T> >(connections_[idx]);
    }
};

template <class T>
class ConnectionManagerDefault : public ConnectionManagerT<T>
{
public:
    virtual FlowStatus read(T &data) override
    {
        size_t size = this->connections_.size();
        std::shared_ptr<ConnectionT<T> > conn;

        for (int i = 0; i < size; ++i)
        {
            conn = this->connection(this->rr_index_ % size);

            this->rr_index_ = (this->rr_index_ + 1) % size;
            if (conn->data(data) == NEW_DATA)
            {
                return NEW_DATA;
            }
        }
        return NO_DATA;
    }

    virtual FlowStatus readAll(std::vector<T> &data) override
    {
        T toutput;
        data.clear();

        for (int i = 0; i < this->connections_.size(); ++i)
        {
            while (this->connection(i)->data(toutput) == NEW_DATA)
                data.push_back(toutput);
        }
        return data.empty() ? NO_DATA : NEW_DATA;
    }

    virtual bool write(const T &data) override
    {
        bool written = false;
        for (int i = 0; i < this->connections_.size(); ++i)
        {
            written = written || this->connection(i)->addData(data);
        }
        return written;
    }
    virtual bool write(const T &data, const std::string &task_name) override
    {
        for (int i = 0; i < this->connections_.size(); ++i)
        {
            if (this->connection(i)->hasComponent(task_name))
                return this->connection(i)->addData(data);
        }
        return false;
    }

};

/*! \brief Template class to manage the specialization of a connection.
 */
template <class T>
class ConnectionT : public ConnectionBase
{
public:
	/*! \brief Simply call ConnectionBase constructor with the templated ports.
	 */
    ConnectionT(std::shared_ptr<InputPort<T> > in, std::shared_ptr<OutputPort<T> > out, ConnectionPolicy policy)
        : ConnectionBase(in->sharedPtr(),
                         out->sharedPtr(),
                         policy)
    {}

	using ConnectionBase::ConnectionBase;
	/*! \brief Retreive data from the connection if present.
	 *  \param data The variable where to store the data.
	 *  \return If new data was present or not.
	 */
    virtual FlowStatus data(T &data) = 0;
	/*! \brief Add data to the connection. If the input port is of type event start the trigger process.
	 *  \param data The data to be written in the connection.
	 *  \return Wheter the write succeded. It may fail if the buffer is full.
	 */
    virtual bool addData(const T &data) = 0;
};

/*! \brief Specialized class for the type T to manage
 *  ConnectionPolicy::DATA ConnectionPolicy::LOCKED
 */
template <class T>
class ConnectionDataL : public ConnectionT<T>
{
public:
    ConnectionDataL(std::shared_ptr<InputPort<T> > in,
                    std::shared_ptr<OutputPort<T> > out,
                    ConnectionPolicy policy)
        : ConnectionT<T>(in, out, policy) {}
	/*! \brief Destroy remainig data
	 */
	~ConnectionDataL()
	{
		if(this->data_status_ == NEW_DATA) 
		{			
			value_.~T();  
		}
	}

    virtual FlowStatus data(T &data) override
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

    virtual bool addData(const T &input) override
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
				new (&value_) T(input); // allocate new data in the given space
				this->data_status_ = NEW_DATA; 
			}
			else
			{
				value_ = input;
				this->data_status_ = NEW_DATA; 
			}
		}
		// trigger if the input port is an event port
        if(this->input()->isEvent() &&
           old_status != NEW_DATA )
        {
            this->trigger();
        }


		return true;
	}

    virtual unsigned int queueLength() const override
    {
        return this->data_status_ == NEW_DATA ? 1 : 0;
    }
private:
	// TODO add the possibility to set this option from outside.
	bool destructor_policy_ = false; //!< Specify wheter to keep old data, or to deallocate it
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


/*! \brief Specialized class for the type T to manage ConnectionPolicy::DATA ConnectionPolicy::UNSYNC 
 */
template <class T>
class ConnectionDataU : public ConnectionT<T>
{
public:
    ConnectionDataU(std::shared_ptr<InputPort<T> > in,
                    std::shared_ptr<OutputPort<T> > out,
                    ConnectionPolicy policy)
        : ConnectionT<T>(in, out, policy) {}
	/*! \brief Destroy remainig data
	 */
	~ConnectionDataU()
	{
		value_.~T();
	}

    virtual FlowStatus data(T &data) override
	{
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

	virtual bool addData(const T &input) override
	{
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
				new (&value_) T(input); // allocate new data in the given space
				this->data_status_ = NEW_DATA; 
			}
			else
			{
				value_ = input;
				this->data_status_ = NEW_DATA; 
			}
		}
		// trigger if the input port is an event port
        if(this->input_->isEvent() && old_status != NEW_DATA)
			this->trigger();
		return true;
	}

    virtual unsigned int queueLength() const override
    {
        return this->data_status_ == NEW_DATA ? 1 : 0;
    }
private:
	bool destructor_policy_ = false;
	union
	{ 
		T value_;
	};
};


/*! \brief Specialized class for the type T to manage ConnectionPolicy::BUFFER/CIRCULAR_BUFFER ConnectionPolicy::UNSYNC 
 */
template <class T>
class ConnectionBufferU : public ConnectionT<T>
{
public:

    ConnectionBufferU(std::shared_ptr<InputPort<T> > in,
                      std::shared_ptr<OutputPort<T> > out,
                      ConnectionPolicy policy)
		: ConnectionT<T>(in, out, policy) 
	{
		buffer_.resize(policy.buffer_size);
	}
	/*! \brief Remove all data in the buffer and return the last value
	 *  \param data The variable where to store data
	 */
    virtual FlowStatus newestData(T &data)
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

	virtual bool addData(const T &input) override
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
        if(this->input_->isEvent() && !buffer_.full())
			this->trigger();

		return true;
	}

    virtual unsigned int queueLength() const override
    {
        return buffer_.size();
    }
private:
	boost::circular_buffer<T> buffer_;
};


/*! \brief Specialized class for the type T to manage ConnectionPolicy::BUFFER/CIRCULAR_BUFFER ConnectionPolicy::LOCKED
 */
template <class T>
class ConnectionBufferL : public ConnectionT<T>
{
public:
    ConnectionBufferL(std::shared_ptr<InputPort<T> > in,
                      std::shared_ptr<OutputPort<T> > out,
                      ConnectionPolicy policy)
		: ConnectionT<T>(in, out, policy)
	{
		buffer_.set_capacity(policy.buffer_size);
	}
	/*! \brief Remove all data in the buffer and return the last value
	 *  \param data The variable where to store data
	 */
    virtual FlowStatus newestData(T &data)
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

    virtual FlowStatus data(T &data) override
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

	virtual bool addData(const T &input) override
	{
		std::unique_lock<std::mutex> mlock(this->mutex_);

		if (buffer_.full())
		{
			if(this->policy_.data_policy == ConnectionPolicy::CIRCULAR)
			{
				buffer_.pop_front();
			}
			else
			{
				return false;
			}
		}
		buffer_.push_back(input);

        if(this->input_->isEvent() && !buffer_.full())
		{
			this->trigger();
		}
		
		return true;
    }
    virtual unsigned int queueLength() const override
    {
        return buffer_.size();
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

/*! \brief Support strucut to create connection easily. 
 */
template <class T>
struct MakeConnection
{
    static std::shared_ptr<ConnectionT<T> > fx(std::shared_ptr<InputPort<T> > &input,
                                               std::shared_ptr<OutputPort<T> > &output,
                                               ConnectionPolicy policy)
	{
		switch(policy.lock_policy)
		{
			case ConnectionPolicy::LOCKED:
				switch (policy.data_policy)
				{
                    case ConnectionPolicy::DATA:		return std::make_shared<ConnectionDataL<T> >(input, output, policy);
                    case ConnectionPolicy::BUFFER:		return std::make_shared<ConnectionBufferL<T> >(input, output, policy);
                    case ConnectionPolicy::CIRCULAR:	return std::make_shared<ConnectionBufferL<T> >(input, output, policy);
				}
				break;
			case ConnectionPolicy::UNSYNC:
				switch (policy.data_policy)
				{
                    case ConnectionPolicy::DATA:		return std::make_shared<ConnectionDataU<T> >(input, output, policy);
                    case ConnectionPolicy::BUFFER:		return std::make_shared<ConnectionBufferU<T> >(input, output, policy);
                    case ConnectionPolicy::CIRCULAR:	return std::make_shared<ConnectionBufferU<T> >(input, output, policy);
				}
				break;
			case ConnectionPolicy::LOCK_FREE:
				switch (policy.data_policy)
				{
					case ConnectionPolicy::DATA:
						policy.buffer_size = 1;
						policy.data_policy = ConnectionPolicy::CIRCULAR;
                        return std::make_shared<ConnectionBufferL<T> >(input, output, policy);
						//return new ConnectionBufferLF<T>(input, output, policy);
                    case ConnectionPolicy::BUFFER:		return std::make_shared<ConnectionBufferL<T> >(input, output, policy);
                    case ConnectionPolicy::CIRCULAR:	return std::make_shared<ConnectionBufferL<T> >(input, output, policy);
					//case ConnectionPolicy::BUFFER:	  return new ConnectionBufferLF<T>(input, output, policy);
					//case ConnectionPolicy::CIRCULAR:	  return new ConnectionBufferLF<T>(input, output, policy);
				}
				break;
		}
		return nullptr;
	}
};

/*! \brief Factory to create the corect connection based on the policys.
 *  \param input The inpurt port.
 *  \param output The output port.
 *	\param policy The policy of the connection.
 *  \return Pointer to the new connection.
 */
template <class T>
std::shared_ptr<ConnectionT<T> > makeConnection(std::shared_ptr<InputPort<T> > input,
                                                std::shared_ptr<OutputPort<T> > output,
                                                ConnectionPolicy policy)
{
#ifdef PORT_INTROSPECTION
	//MakeConnection<T>::fx(input, ) given the task inspection create a new port for him and go
#endif	
	return MakeConnection<T>::fx(input, output, policy);
}
/*! \brief Factory to create the corect connection based on the policys.
 *  \param output The output port.
 *  \param input The inpurt port.
 *	\param policy The policy of the connection.
 *  \return Pointer to the new connection.
 */
template <class T>
std::shared_ptr<ConnectionT<T> > makeConnection(std::shared_ptr<OutputPort<T> > output,
                                                std::shared_ptr<InputPort<T> > input,
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
                manager_.readerDone(data_);
			else
                manager_.writerDone(data_);
			data_ =0;
		}		
	}

	void discard()
	{
		if(data_)
		{
			if(is_reading_)
                manager_.readerDone(data_);
			else
                manager_.writerNotDone(data_);
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
