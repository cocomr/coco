#pragma once

#include <boost/lexical_cast.hpp>

#include "coco/util/accesses.hpp"

#include "coco/connection.h"
#include "coco/task.h"

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
    const std::type_info &asSig() final
    {
        return typeid(T);
    }
    /*!
     * \brief Used to set the value of the attribute and thus of the undeline variable.
     * \param value The value is read from the XML config file, so it is a string that
     * it is automatically converted to the type of the attribute. Only basic types are
     * allowed
     */
    void setValue(const std::string &value) final
    {
        value_ = boost::lexical_cast<T>(value);
    }
    /*!
     * \return a void ptr to the value variable. Can be used togheter with asSig to
     * retreive the variable
     */
    void *value() final
    {
        return & value_;
    }
    /*!
     * \return serialize the value of the attribute into a string
     */
    std::string toString() final
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
    const std::type_info &asSig() final
    {
        return typeid(T);
    }
    /*! \brief Set the valus of the vector. The supported format is CSV, with or without spaces.
     *  \param value A CSV line containing the values to be associated to the vector.
     */
    void setValue(const std::string &value) final
    {
        std::string new_value = value;
        auto pos = std::find(new_value.begin(), new_value.end(), ' ');
        while (pos != new_value.end())
        {
            new_value.erase(pos);
            pos = std::find(new_value.begin(), new_value.end(), ' ');
        }
        std::vector<Q> nv;
        for (auto p : coco::util::string_splitter(new_value, ','))
        {
            nv.push_back(boost::lexical_cast<Q>(p));
        }
        value_ = nv;
    }
    /*!
     * \return a void ptr to the value variable. Can be used togheter with asSig to
     * retreive the variable
     */
    void *value() final
    {
        return &value_;
    }
    /*!
     * \return serialize the value of the attribute into a string
     */
    std::string toString() final
    {
        std::stringstream ss;
        for (uint i = 0; i < value_.size(); i++)
        {
            if (i > 0)
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
        : Operation(task, name, coco::util::bind_this(fx, obj))
    {}

private:
    typedef typename coco::util::get_functioner<T>::fx Sig;
    /*!
     * \return the signature of the function
     */
    const std::type_info &asSig() final
    {
        return typeid(Sig);
    }
    /*!
     *  \return The function as void *. Used when invoking the function.
     */
    void *asFx() final
    {
        return reinterpret_cast<void *>(&fx_);
    }
#if 0
    /// invokation given params and return value
    virtual boost::any  call(std::vector<boost::any> & params)
    {
        if (params.size() != arity<T>::value)
        {
            std::cout << "argument count mismatch\n";
            throw std::exception();
        }
        return call_n_args<T>::call(fx_, params, make_int_sequence< arity<T>::value >{});
    }
#endif
private:
    std::function<T> fx_;
};

template <class T>
class ConnectionManagerInputT;
template <class T>
class ConnectionManagerOutputT;
template <class T>
class ConnectionManagerInputDefault;
template <class T>
class ConnectionManagerOutputDefault;
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
        createConnectionManager(true, ConnectionManagerType::DEFAULT);
    }
    /*!
     * \return The type_info of the data contained in the port
     */
    const std::type_info &typeInfo() const final { return typeid(T); }
    /*!
     * \brief Function to connect two ports
     * \param other The other port with which to connect
     * \param policy The policy of the connection
     * \return Wheter the connection has been successfully
     */
    bool connectTo(std::shared_ptr<PortBase> &other, ConnectionPolicy policy) final
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
        return std::static_pointer_cast<ConnectionManagerInputT<T> >(this->manager_)->read(data);
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
        return std::static_pointer_cast<ConnectionManagerInputT<T> >(this->manager_)->readAll(data);
    }
    /*!
     * \return True if the port has incoming new data;
     */
    bool hasNewData() const { return this->queueLength() > 0; }
private:
    friend class OutputPort<T>;
    friend class GraphLoader;

    /*! \brief Called by \ref connectTo(), does the actual connection once the type have been checked.
     *  \param other The other port to which to connect.
     *  \param policy The connection policy.
     *  \return Wheter the connection succeded. It can only fails wheter we are connecting two port of the same component.
     */
    bool connectToTyped(std::shared_ptr<OutputPort<T> > &other, ConnectionPolicy policy)
    {
        // Check that the two ports doesn't belong to the same task
        if (task_ == other->task())
            return false;

        std::shared_ptr<ConnectionBase> connection(makeConnection(
                                                       std::static_pointer_cast<InputPort<T> >(this->sharedPtr()),
                                                       other, policy));
        addConnection(connection);
        other->addConnection(connection);
        return true;
    }
    /*!
     * \brief Instantiate the ConnectionManager for the port. The connection manager is in charge
     * of managing incoming and outcoming connection and
     * \param type
     */
    void createConnectionManager(bool input, ConnectionManagerType type)
    {
        switch (type)
        {
            case ConnectionManagerType::DEFAULT:
                if (input)
                    this->manager_ = std::make_shared<ConnectionManagerInputDefault<T> >();
                else
                    this->manager_ = std::make_shared<ConnectionManagerOutputDefault<T> >();
                break;
            case ConnectionManagerType::FARM:

                break;
            default:
                COCO_FATAL() << "Invalid ConnectionManagerType " << static_cast<int>(type);
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
        createConnectionManager(false, ConnectionManagerType::DEFAULT);
    }

    const std::type_info& typeInfo() const final { return typeid(T); }

    bool connectTo(std::shared_ptr<PortBase> &other, ConnectionPolicy policy) final
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
        }
        else
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
        return std::static_pointer_cast<ConnectionManagerOutputT<T> >(manager_)->write(data);
    }
    /*! \brief Write only in a specific port contained in the task named \ref name.
     *  \param input The value to be written.
     *  \param name The name of a taks.
     */
    bool write(const T &data, const std::string &task_name)
    {
        return std::static_pointer_cast<ConnectionManagerOutputT<T> >(manager_)->write(data, task_name);
    }

private:
    friend class InputPort<T>;
    friend class GraphLoader;

    /*! \brief Called by \ref connectTo(), does the actual connection once the type have been checked.
     *  \param other The other port to which to connect.
     *  \param policy The connection policy.
     *  \return Wheter the connection succeded. It can only fails wheter we are connecting two port of the same component.
     */
    bool connectToTyped(std::shared_ptr<InputPort<T> > &other, ConnectionPolicy policy)
    {
        if (task_ == other->task())
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

    void createConnectionManager(bool input, ConnectionManagerType type)
    {
        switch (type)
        {
            case ConnectionManagerType::DEFAULT:
                if (input)
                    this->manager_ = std::make_shared<ConnectionManagerInputDefault<T> >();
                else
                    this->manager_ = std::make_shared<ConnectionManagerOutputDefault<T> >();
                break;
            case ConnectionManagerType::FARM:

                break;
            default:
                COCO_FATAL() << "Invalid ConnectionManagerType " << static_cast<int>(type);
                break;
        }
    }
};

}  // end of namespace coco
