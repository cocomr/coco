/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#pragma once
#include <memory>
#include <string>
#include <vector>
#include <iomanip>

#include <boost/circular_buffer.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include "coco/connection.h"

#include "coco/task_impl.hpp"
#include "execution.h"

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
    ConnectionT(std::shared_ptr<InputPort<T> > in,
                std::shared_ptr<OutputPort<T> > out,
                ConnectionPolicy policy)
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
        : ConnectionT<T>(in, out, policy)
    {}
    /*! \brief Destroy remainig data
     */
    ~ConnectionDataL()
    {
        if (this->data_status_ == NEW_DATA)
        {
            value_.~T();
        }
    }

    FlowStatus data(T &data) final
    {
        std::unique_lock<std::mutex> mlock(this->mutex_);
        if (this->data_status_ == NEW_DATA)
        {
            data = value_;  // copy => std::move
            this->data_status_ = OLD_DATA;
            if (destructor_policy_)
            {
                value_.~T();  // destructor
                this->data_status_ = NO_DATA;
            }
            if (this->input_->isEvent())
                this->removeTrigger();

            /* Propagate timestamp to calculate latency */
            int long latency_time = this->output_->task()->latencyTimestamp();
            if (latency_time > 0)
                this->input_->task()->setLatencyTimestamp(latency_time);

            return NEW_DATA;
        }
        return this->data_status_;
    }

    bool addData(const T &input) final
    {
        std::unique_lock<std::mutex> mlock(this->mutex_);
        FlowStatus old_status = this->data_status_;
        if (destructor_policy_)
        {
            if (this->data_status_ == NEW_DATA)
            {
                value_ = input;
            }
            else
            {
                new (&value_) T(input);
                this->data_status_ = NEW_DATA;  // mark
            }
        }
        else
        {
            if (this->data_status_ == NO_DATA)
            {
                new (&value_) T(input);  // allocate new data in the given space
                this->data_status_ = NEW_DATA;
            }
            else
            {
                value_ = input;
                this->data_status_ = NEW_DATA;
            }
        }
        /* trigger if the input port is an event port */
        if (this->input()->isEvent() &&
            old_status != NEW_DATA )
            this->trigger();

        return true;
    }

    unsigned int queueLength() const final
    {
        return this->data_status_ == NEW_DATA ? 1 : 0;
    }
private:
    // TODO add the possibility to set this option from outside.
    bool destructor_policy_ = false;  // Specify wheter to keep old data, or to deallocate it
    union
    {
        T value_;
    };
    std::mutex mutex_;
};

/*! \brief Specialized class for the type T to manage ConnectionPolicy::DATA ConnectionPolicy::UNSYNC
 */
template <class T>
class ConnectionDataU : public ConnectionT<T>
{
public:
    ConnectionDataU(std::shared_ptr<InputPort<T> > in,
                    std::shared_ptr<OutputPort<T> > out,
                    ConnectionPolicy policy)
        : ConnectionT<T>(in, out, policy)
    {}
    /*! \brief Destroy remainig data
     */
    ~ConnectionDataU()
    {
        value_.~T();
    }

    FlowStatus data(T &data) final
    {
        if (this->data_status_ == NEW_DATA)
        {
            data = value_;  // copy => std::move
            this->data_status_ = OLD_DATA;
            if (destructor_policy_)
            {
                value_.~T();  // destructor
                this->data_status_ = NO_DATA;
            }
            if (this->input_->isEvent())
                this->removeTrigger();

            /* Propagate timestamp to calculate latency */
            int long latency_time = this->output_->task()->latencyTimestamp();
            if (latency_time > 0)
                this->input_->task()->setLatencyTimestamp(latency_time);

            return NEW_DATA;
        }
        return this->data_status_;
    }

    bool addData(const T &input) final
    {
        FlowStatus old_status = this->data_status_;
        if (destructor_policy_)
        {
            if (this->data_status_ == NEW_DATA)
            {
                value_ = input;
            }
            else
            {
                new (&value_) T(input);
                this->data_status_ = NEW_DATA;  // mark
            }
        }
        else
        {
            if (this->data_status_ == NO_DATA)
            {
                new (&value_) T(input);  // allocate new data in the given space
                this->data_status_ = NEW_DATA;
            }
            else
            {
                value_ = input;
                this->data_status_ = NEW_DATA;
            }
        }
        /* trigger if the input port is an event port */
        if (this->input_->isEvent() && old_status != NEW_DATA)
            this->trigger();
        return true;
    }

    unsigned int queueLength() const final
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

template <class T>
class ConnectionDataLF : public ConnectionT<T>
{
public:
    ConnectionDataLF(std::shared_ptr<InputPort<T> > in,
    std::shared_ptr<OutputPort<T> > out,
    ConnectionPolicy policy)
    : ConnectionT<T>(in, out, policy)
    {}


    FlowStatus data(T & data) final
    {
        bool new_data = queue_.pop(data);
        if (new_data)
        {
            /* Propagate timestamp to calculate latency */
            int long latency_time = this->output_->task()->latencyTimestamp();
            if (latency_time > 0)
                this->input_->task()->setLatencyTimestamp(latency_time);

            return NEW_DATA;
        }
        return NO_DATA;
    }

    bool addData(const T &input) final
    {
        if (!queue_.push(input))
        {
            return false;
        }
        if (this->input_->isEvent())
            this->trigger();

        return true;
    }

    unsigned int queueLength() const final
    {
        return this->data_status_ == NEW_DATA ? 1 : 0;
    }
private:
    boost::lockfree::spsc_queue<T, boost::lockfree::capacity<1> > queue_;
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
    FlowStatus newestData(T &data)
    {
        std::unique_lock<std::mutex> mlock(this->mutex_);
        bool status = false;
        while (!buffer_.empty())
        {
            data = buffer_.front();
            buffer_.pop_front();
            status = true;
        }
        if (status)
        {
            if (this->input_->isEvent())
                this->removeTrigger();

            /* Propagate timestamp to calculate latency */
            int long latency_time = this->output_->task()->latencyTimestamp();
            if (latency_time > 0)
                this->input_->task()->setLatencyTimestamp(latency_time);
        }
        return status ? NEW_DATA : NO_DATA;
    }

    FlowStatus data(T &data) final
    {
        std::unique_lock<std::mutex> mlock(this->mutex_);
        if (!buffer_.empty())
        {
            data = buffer_.front();
            buffer_.pop_front();
            if (this->input_->isEvent())
                this->removeTrigger();

            /* Propagate timestamp to calculate latency */
            int long latency_time = this->output_->task()->latencyTimestamp();
            if (latency_time > 0)
                this->input_->task()->setLatencyTimestamp(latency_time);

            return NEW_DATA;
        }
        return NO_DATA;
    }

    bool addData(const T &input) final
    {
        std::unique_lock<std::mutex> mlock(this->mutex_);

        if (buffer_.full())
        {
            if (this->policy_.data_policy == ConnectionPolicy::CIRCULAR)
                buffer_.pop_front();
            else
                return false;
        }
        buffer_.push_back(input);

        if (this->input_->isEvent() && !buffer_.full())
            this->trigger();

        return true;
    }
    unsigned int queueLength() const final
    {
        return buffer_.size();
    }
private:
    boost::circular_buffer<T> buffer_;
    std::mutex mutex_;
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
    FlowStatus newestData(T &data)
    {
        bool status = false;
        while (!buffer_.empty())
        {
            status = true;
            data = buffer_.front();
            buffer_.pop_front();
        }
        if (status)
        {
            if (this->input_->isEvent())
                this->removeTrigger();

            /* Propagate timestamp to calculate latency */
            int long latency_time = this->output_->task()->latencyTimestamp();
            if (latency_time > 0)
                this->input_->task()->setLatencyTimestamp(latency_time);
        }
        return status ? NEW_DATA : NO_DATA;
    }

    FlowStatus data(T & data) final
    {
        if (!buffer_.empty())
        {
            data = buffer_.front();
            buffer_.pop_front();
            if (this->input_->isEvent())
                this->removeTrigger();

            /* Propagate timestamp to calculate latency */
            int long latency_time = this->output_->task()->latencyTimestamp();
            if (latency_time > 0)
                this->input_->task()->setLatencyTimestamp(latency_time);

            return NEW_DATA;
        }
        else
            return NO_DATA;
    }

    bool addData(const T &input) final
    {
        if (buffer_.full())
        {
            if (this->policy_.data_policy == ConnectionPolicy::CIRCULAR)
                buffer_.pop_front();
            else
                return false;
        }
        buffer_.push_back(input);
        this->data_status_ = NEW_DATA;
        if (this->input_->isEvent() && !buffer_.full())
            this->trigger();

        return true;
    }

    unsigned int queueLength() const final
    {
        return buffer_.size();
    }
private:
    boost::circular_buffer<T> buffer_;
};

/**
 * Specialized class for the type T
 */
template <class T>
class ConnectionBufferLF : public ConnectionT<T>
{
public:
    ConnectionBufferLF(std::shared_ptr<InputPort<T> > in,
                       std::shared_ptr<OutputPort<T> > out,
                       ConnectionPolicy policy)
        : ConnectionT<T>(in, out, policy)
    {
        queue_ = new boost::lockfree::spsc_queue<T>(policy.buffer_size);
    }

    FlowStatus newestData(T & data)
    {
        bool once = false;
        while (queue_->pop(data))
            once = true;

        if (once)
        {
            /* Propagate timestamp to calculate latency */
            int long latency_time = this->output_->task()->latencyTimestamp();
            if (latency_time > 0)
                this->input_->task()->setLatencyTimestamp(latency_time);
        }
        return once ? NEW_DATA : NO_DATA;
    }

    FlowStatus data(T & data) final
    {
        bool new_data = queue_->pop(data);
        if (new_data)
        {
            /* Propagate timestamp to calculate latency */
            int long latency_time = this->output_->task()->latencyTimestamp();
            if (latency_time > 0)
                this->input_->task()->setLatencyTimestamp(latency_time);
            return NEW_DATA;
        }
        return NO_DATA;
    }

    bool addData(const T &input) final
    {
        if (!queue_->push(input))
        {
            if (this->policy_.data_policy == ConnectionPolicy::CIRCULAR)
            {
                T dummy;
                queue_->pop(dummy);
                queue_->push(input);
            }
            else
            {
                return false;
            }
        }
        if (this->input_->isEvent())
            this->trigger();

        return true;
    }

    unsigned int queueLength() const final
    {
        //return queue_.size();
        return 0;
    }
private:
    boost::lockfree::spsc_queue<T> *queue_;
};


/*! \brief Support strucut to create connection easily.
 */
template <class T>
struct MakeConnection
{
    static std::shared_ptr<ConnectionT<T> > fx(std::shared_ptr<InputPort<T> > &input,
                                               std::shared_ptr<OutputPort<T> > &output,
                                               ConnectionPolicy policy)
    {
        switch (policy.lock_policy)
        {
            case ConnectionPolicy::LOCKED:
                switch (policy.data_policy)
                {
                    case ConnectionPolicy::DATA:        return std::make_shared<ConnectionDataL<T> >(input, output, policy);
                    case ConnectionPolicy::BUFFER:      return std::make_shared<ConnectionBufferL<T> >(input, output, policy);
                    case ConnectionPolicy::CIRCULAR:    return std::make_shared<ConnectionBufferL<T> >(input, output, policy);
                }
                break;
            case ConnectionPolicy::UNSYNC:
                switch (policy.data_policy)
                {
                    case ConnectionPolicy::DATA:        return std::make_shared<ConnectionDataU<T> >(input, output, policy);
                    case ConnectionPolicy::BUFFER:      return std::make_shared<ConnectionBufferU<T> >(input, output, policy);
                    case ConnectionPolicy::CIRCULAR:    return std::make_shared<ConnectionBufferU<T> >(input, output, policy);
                }
                break;
            case ConnectionPolicy::LOCK_FREE:
                switch (policy.data_policy)
                {
                    case ConnectionPolicy::DATA:        return std::make_shared<ConnectionDataLF<T> >(input, output, policy);
                    case ConnectionPolicy::BUFFER:      return std::make_shared<ConnectionBufferLF<T> >(input, output, policy);
                    case ConnectionPolicy::CIRCULAR:    return std::make_shared<ConnectionBufferLF<T> >(input, output, policy);
                }
                break;
        }
        return nullptr;
    }
};

/*! \brief Factory to create the corect connection based on the policys.
 *  \param input The inpurt port.
 *  \param output The output port.
 *  \param policy The policy of the connection.
 *  \return Pointer to the new connection.
 */
template <class T>
std::shared_ptr<ConnectionT<T> > makeConnection(std::shared_ptr<InputPort<T> > input,
                                                std::shared_ptr<OutputPort<T> > output,
                                                ConnectionPolicy policy)
{
#ifdef PORT_INTROSPECTION
    // MakeConnection<T>::fx(input, ) given the task inspection create a new port for him and go
#endif
    return MakeConnection<T>::fx(input, output, policy);
}
/*! \brief Factory to create the corect connection based on the policys.
 *  \param output The output port.
 *  \param input The inpurt port.
 *  \param policy The policy of the connection.
 *  \return Pointer to the new connection.
 */
template <class T>
std::shared_ptr<ConnectionT<T> > makeConnection(std::shared_ptr<OutputPort<T> > output,
                                                std::shared_ptr<InputPort<T> > input,
                                                ConnectionPolicy policy)
{
#ifdef PORT_INTROSPECTION
    // MakeConnection<T>::fx(input, ) given the task inspection create a new port for him and go
#endif
    return MakeConnection<T>::fx(input, output, policy);
}

/*! \brief Specialization for managing \p InputPort's connections
 */
template <class T>
class ConnectionManagerInputT : public ConnectionManager
{
public:
    /*! \brief Read data from one connection, the policy depends on the specialization
     *  \param data Variable where to store the data from the connection
     *  \return Wheter new data was present in the connection
     */
    virtual FlowStatus read(T &data) = 0;
    /*! \brief Read the data from all the connections associated with the port
     *  \param data Vector where to store the data of all the connections. The order of the
     * connections is the same as specified in the launch file
     *  \return Wheter new data was present in the connections
     */
    virtual FlowStatus readAll(std::vector<T> &data) = 0;
    /*! \brief Used to retreive a specific connection
     *  \param idx Index of the desired connection
     *  \return Shared ptr to the connection
     */
    std::shared_ptr<ConnectionT<T> > connection(unsigned idx)
    {
        assert(idx < connections_.size() && "trying to access a connection out of bound");
        return std::static_pointer_cast<ConnectionT<T> >(connections_[idx]);
    }
};

/*! \brief Specialization for managing \p OutputPort's connections
 */
template <class T>
class ConnectionManagerOutputT : public ConnectionManager
{
public:
    /*! \brief Write data in the associated connections, the policy depends on the specialization
     *  \param data Variable to be written
     *  \return Wheter the write succeded
     */
    virtual bool write(const T &data) = 0;
    /*! \brief Write data in ports contained in a specific task
     *  \param data The variable to be written
     *  \param task_name The name of the task to wich we want to write data
     *  \return Wheter the write succeded
     */
    virtual bool write(const T &data, const std::string &task_name) = 0;
    /*! \brief Used to retreive a specific connection
     *  \param idx Index of the desired connection
     *  \return Shared ptr to the connection
     */
    std::shared_ptr<ConnectionT<T> > connection(unsigned idx)
    {
        assert(idx < connections_.size() && "trying to access a connection out of bound");
        return std::static_pointer_cast<ConnectionT<T> >(connections_[idx]);
    }
};
/*! \brief Default input connection manager
 */
template <class T>
class ConnectionManagerInputDefault : public ConnectionManagerInputT<T>
{
public:
    /*! \brief Read data from a connection with a Round Robin scheduling
     *  \param data Variable where to store the read value
     *  \return Wheter new data was present in the connections
     */
    FlowStatus read(T &data) final
    {
        size_t size = this->connections_.size();
        std::shared_ptr<ConnectionT<T> > conn;

        for (unsigned int i = 0; i < size; ++i)
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
    /*! \brief Read the data from all the connections associated with the port
     *  \param data Vector where to store the data of all the connections. The order of the
     * connections is the same as specified in the launch file
     *  \return Wheter new data was present in the connections
     */
    FlowStatus readAll(std::vector<T> &data) final
    {
        T toutput;
        data.clear();

        for (unsigned int i = 0; i < this->connections_.size(); ++i)
        {
            while (this->connection(i)->data(toutput) == NEW_DATA)
                data.push_back(toutput);
        }
        return data.empty() ? NO_DATA : NEW_DATA;
    }
private:
    unsigned int rr_index_ = 0;
};

template <class T>
class ConnectionManagerOutputDefault : public ConnectionManagerOutputT<T>
{
public:
    /*! \brief Write data in all the associated connections
     *  \param data Variable to be written
     *  \return Wheter the write succeded
     */
    bool write(const T &data) final
    {
        bool written = false;
        for (unsigned int i = 0; i < this->connections_.size(); ++i)
        {
            written = this->connection(i)->addData(data) || written;
        }
        return written;
    }
    /*! \brief Write data in ports contained in a specific task
     *  \param data The variable to be written
     *  \param task_name The name of the task to wich we want to write data
     *  \return Wheter the write succeded
     */
    bool write(const T &data, const std::string &task_name) final
    {
        for (unsigned int i = 0; i < this->connections_.size(); ++i)
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
     /*
      * The behaviour is exactly the same as of normal read. We read from one connection
      * iterating in a round robin fashion. The problem here is that there is no guarantee
      * on the order of delivery
      * The main problem is that there is no way to discard a message if it takes to much to be computed.
      * Suppose the rate is 30Hz the component needs 100ms to compute -> 4 instantiation to be sure.
      * If one of the farm worker stuck and spends 500ms computing, the gather will receive in any case that
      * data and it cannot discard it even if it is arrived with half second delay
      */
    FlowStatus read(T &data) final
    {
        unsigned int size = this->connections_.size();
        std::shared_ptr<ConnectionT<T> > conn;

        for (unsigned int i = 0; i < size; ++i)
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

    FlowStatus readAll(std::vector<T> &data) final
    {
        T toutput;
        data.clear();

        for (unsigned int i = 0; i < this->connections_.size(); ++i)
        {
            while (this->connection(i)->data(toutput) == NEW_DATA)
                data.push_back(toutput);
        }
        return data.empty() ? NO_DATA : NEW_DATA;
    }

private:
    unsigned int rr_index_ = 0;
};

template <class T>
class ConnectionManagerOutputFarm : public ConnectionManagerOutputT<T>
{
public:
    bool write(const T &data) final
    {
        /* Write to a connection which is empty and whose task is idle
         * auto tmp_rr_index_ = rr_index_; */
        unsigned int size = this->connections_.size();
        for (unsigned int i = 0; i < size; ++i)
        {
            auto conn_ptr = this->connection(rr_index_);
            if (!conn_ptr->hasNewData() && conn_ptr->input()->task()->state() == TaskState::IDLE)
            {
                return conn_ptr->addData(data);
            }
            rr_index_ = (rr_index_ + 1) % size;
        }
        /* If there are no idle components iterate and find one that has at least the connection empty */
        for (unsigned int i = 0; i < size; ++i)
        {
            auto conn_ptr = this->connection(rr_index_);
            if (!conn_ptr->hasNewData())
            {
                return conn_ptr->addData(data);
            }
            rr_index_ = (rr_index_ + 1) % size;
        }
        /* In this case there are no idle components neither with an empty queue, so return false */
        return false; // TODO decide what to do if there are no idle component
    }

    bool write(const T &data, const std::string &task_name) final
    {
        COCO_ERR() << "Don't use this function with a farm component!";
        return false;
    }
private:
    unsigned int rr_index_ = 0;
};


}  // end of namespace coco
