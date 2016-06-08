#pragma once
#include <memory>

#include <boost/circular_buffer.hpp>


#include "coco/connection.h"

#include "coco/task_impl.hpp"

namespace coco
{

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

template <class T>
class ConnectionManagerInputT : public ConnectionManager
{
public:
    virtual FlowStatus read(T &data) = 0;
    virtual FlowStatus readAll(std::vector<T> &data) = 0;
    std::shared_ptr<ConnectionT<T> > connection(unsigned idx)
    {
        assert(idx < connections_.size() && "trying to access a connection out of bound");
        return std::static_pointer_cast<ConnectionT<T> >(connections_[idx]);
    }
};

template <class T>
class ConnectionManagerOutputT : public ConnectionManager
{
public:
    virtual bool write(const T &data) = 0;
    virtual bool write(const T &data, const std::string &task_name) = 0;
    std::shared_ptr<ConnectionT<T> > connection(unsigned idx)
    {
        assert(idx < connections_.size() && "trying to access a connection out of bound");
        return std::static_pointer_cast<ConnectionT<T> >(connections_[idx]);
    }
};

template <class T>
class ConnectionManagerInputDefault : public ConnectionManagerInputT<T>
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
};

template <class T>
class ConnectionManagerOutputDefault : public ConnectionManagerOutputT<T>
{
public:
    virtual bool write(const T &data) override
    {
        bool written = false;
        for (int i = 0; i < this->connections_.size(); ++i)
        {
            written = this->connection(i)->addData(data) || written;
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

template <class T>
class ConnectionManagerInputFarm : public ConnectionManagerInputT<T>
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
};

template <class T>
class ConnectionManagerOutputFarm : public ConnectionManagerOutputT<T>
{
public:
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


}
