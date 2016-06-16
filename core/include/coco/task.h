#pragma once
#include <memory>
#include <string>
#include <list>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>
#include <unordered_set>

#include "coco/util/logging.h"

namespace coco
{

class TaskContext;

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
    /*! \brief Serialize the value of the associated variable to a string.
     *  \return The value of the attribute as a string.
     */
    virtual std::string toString() = 0;
    /*!
     * \return The name of the attribute.
     */
    const std::string & name() const { return name_; }
    /*!
     * \return The eventual documanetation associated to the attribute.
     */
    const std::string & doc() const { return doc_; }
    /*!
     * \param doc Associates to the attribute a documentation.
     */
    void setDoc(const std::string & doc) { doc_ = doc; }
    /*! \brief Set the value of the attribute from its litteral value and therefore of the variable bound to it.
     *  \value The litteral value of the attribute that will be converted in the underlying variable type.
     */
    virtual void setValue(const std::string &value) = 0;    // get generic
    /*!
     * \return The type of the bound variable.
     */
    virtual const std::type_info & asSig() = 0;
    /*!
     * \return Pointer to the associated variable.
     */
    virtual void * value() = 0;
    /*!
     * \return Cast the underlaying variable to \ref T type and return it.
     */
    template <class T>
    T & as()
    {
        if (typeid(T) != asSig())
            throw std::exception();
        else
            return *reinterpret_cast<T*>(value());
    }
private:
    std::string name_;
    std::string doc_;
};

 /*! \brief Container for operation.
  *  Operations are used to embedd a component function inside and object that can
  *  used to call that function asynchronously or can be enqueued in the scheduling.
  */
class OperationBase
{
public:
    /*!
     *  \param service \ref Service containing this operation.
     *  \param name The name of the operation.
     */
    OperationBase(TaskContext *task, const std::string &name);
    /*!
     *  \return The eventual documentation attached to the operation
     */
    const std::string & doc() const { return doc_; }
    /*!
     *  \param doc The documentation to be attached to the operation
     */
    void setDoc(const std::string & doc) { doc_ = doc; }
    /*!
     *  \return The name of the operation.
     */
    const std::string & name() const { return name_; }

protected:
    friend class XMLCreator;
    friend class Service;
    /// commented for future impl
    // virtual boost::any  call(std::vector<boost::any> & params) = 0;

    /*!
     *  \return A pointer to the embedded function.
     */
    virtual void* asFx() = 0;
    /*!
     * \return The signature of the embedded function.
     */
    virtual const std::type_info & asSig() = 0;
    /*!
     *  \return The function if the signature matches, otherwise an exception is raised.
     */
private:
    template <class Sig>
    std::function<Sig> & as()
    {
        if (typeid(Sig) != asSig())
        {
            COCO_FATAL() << typeid(Sig).name() << " vs expected " << asSig().name();
            throw std::exception();
        }
        else
        {
            return *(std::function<Sig> *)(this->asFx());
        }
    }
    /*!
     *  \param name Set the name of the operation.
     */
    void setName(const std::string & name) { name_ = name; }
private:
    std::string name_;
    std::string doc_;
};

class ConnectionManager;
enum class ConnectionManagerType;
class ConnectionPolicy;
class ConnectionBase;

/*! \brief Base class to manage ports.
 *  Ports are used by components to exchange data.
 */
class PortBase : public std::enable_shared_from_this<PortBase>
{
public:
    /*!
     *  \param task The task containing this port.
     *  \param name The name of the port. Used to identify it. A component cannot have multiple port with the same name
     *  \param is_output Wheter the port is an output or an input port.
     *  \param is_event Wheter the port is an event port. Event port are used to create triggered components.
     */
    PortBase(TaskContext *task, const std::string &name,
             bool is_output, bool is_event);
    /*!
     *  \return Wheter this port is connected to at least another one.
     */
    bool isConnected() const;
    /*!
     *  \return Wheter this port is an event one.
     */
    bool isEvent() const { return is_event_; }
    /*!
     *  \return Wheter this port is an output port.
     */
    bool isOutput() const { return is_output_; }
    /*!
     *  \param doc Set the documentation related to the port.
     */
    void setDoc(const std::string &doc) { doc_ = doc; }
    /*!
     *  \return The documentation related to the port.
     */
    const std::string &doc() const { return doc_; }
    /*!
     *  \return The name of the port.
     */
    const std::string &name() const { return name_; }
    /*!
     *  \return Pointer to the task containing the port.
     */
    const TaskContext *task() const { return task_; }
    /*!
     *  \return The number of connections associated with this port.
     */
    unsigned int connectionsCount() const;
    /*!
      * \param connection The queue for the connection with the give id
      *   if -1 the longest queue among all connection is returned.
      * \return The lenght of the queue
      */
    unsigned int queueLength(int connection = -1) const;
    /*!
     *  \return The type info of the port type.
     */
    virtual const std::type_info &typeInfo() const = 0;
    /*!
     *  \param other The other port to which connect to.
     *  \param policy The policy of the connection.
     *  \return Wheter the connection was succesfully.
     */
    virtual bool connectTo(std::shared_ptr<PortBase> &other, ConnectionPolicy policy) = 0;
    /*!
     * \return The shared pointer for the class
     */
    std::shared_ptr<PortBase> sharedPtr()
    {
        return shared_from_this();
    }
protected:
    friend class ConnectionBase;
    friend class CocoLauncher;
    friend class GraphLoader;

    virtual void createConnectionManager(bool input, ConnectionManagerType type) = 0;

    /*! \brief Trigger the task owing this port to notify that new data is present in the port.
     */
    void triggerComponent();
    /*! \brief Once the data from the port has been read, remove the trigger from the task.
     */
    void removeTriggerComponent();
    /*!
     *  \param Set the name of the port.
     */
    void setName(const std::string &name) { name_ = name; }
    /*!
     * \param connection Add a connection to the port. In particular to the ConnectionManager of the port.
     */
    bool addConnection(std::shared_ptr<ConnectionBase> &connection);
    /*!
     *  \return The \ref ConnectionManager responsible for this port.
     */
    const std::shared_ptr<ConnectionManager> connectionManager() { return manager_; }

protected:
    std::shared_ptr<ConnectionManager> manager_;
    TaskContext *task_;
    std::string name_;
    std::string doc_;
    bool is_output_;
    bool is_event_;
};

// -------------------------------------------------------------------
// Execution
// -------------------------------------------------------------------

/*! \brief Support structure for enqueing operation in a component.
 */
struct OperationInvocation
{
    explicit OperationInvocation(const std::function<void(void)> &p);
    std::function<void(void)> fx;
};

// http://www.orocos.org/stable/documentation/rtt/v2.x/api/html/classRTT_1_1base_1_1RunnableInterface.html
/*! Base class for creating components.
 *  Manages all properties of a Task Context. Services is present because Task Context can have sub ones
 */
class Service : public std::enable_shared_from_this<Service>
{
public:
    /*! \brief The name of the task is always equal to the name of the derived class.
     *  \param name Name of the service.
     */
    explicit Service(const std::string &name = "");
    /*!
     *  \return The container of the operations to iterate over it.
     */
    const std::unordered_map<std::string, std::shared_ptr<OperationBase> > & operations() const { return operations_; }
    /*! \brief Enqueue an operation in the the task operation list, the enqueued operations will be executed before the onUpdate function.
     *  \param name The name of the operations.
     *  \param args The argument that the user want to pass to the operation's function.
     *  \return Wheter the operation is successfully enqueued. This function can fail if an operation with the given name doesn't exist.
     */
    template <class Sig, class ...Args>
    bool enqueueOperation(const std::string & name, Args... args)
    {
        // static_assert< returnof(Sig) == void
        std::function<Sig> fx = operation<Sig>(name);
        if (!fx)
            return false;
        asked_ops_.push_back(OperationInvocation(
                                // [=] () { fx(args...); }
                                std::bind(fx, args...)));
        return true;
    }
    /*! \brief Enqueue an operation in the the task operation list, the enqueued operations will be executed before the onUpdate function.
     *  Togheter with the operation allows to enqueue a callback that is called with the return value of the operation.
     *  \param return_fx The function that is called passing to it the return value of the operation \ref name,
     *  \param name The name of the operations.
     *  \param args The argument that the user want to pass to the operation's function.
     *  \return Wheter the operation is successfully enqueued. This function can fail if an operation with the given name doesn't exist.
     */
    template <class Sig, class ...Args>
    bool enqueueOperation(std::function<void(typename std::function<Sig>::result_type)> return_fx,
                          const std::string & name, Args... args)
    {
        std::function<Sig> fx = operation<Sig>(name);
        if (!fx)
            return false;
        auto p = this;
        auto ffx = std::bind(fx, args...);
        asked_ops_.push_back(OperationInvocation(
                                [this, ffx, p, return_fx] ()
                                {
                                    auto R = ffx();
                                    std::unique_lock<std::mutex> lock(op_mutex_);
                                    p->asked_ops_.push_back(
                                        OperationInvocation([R, return_fx] () { returnfx(R); }));
                                }));
        return true;
    }
    /*! Return the operation if name and signature match.
     *  \param name The name of the operation to be returned.
     *  \return An std::function object containing the operation if it exists, an empty container otherwise.
     */
    template <class Sig>
    std::function<Sig> operation(const std::string & name)
    {
        auto it = operations_.find(name);
        if (it == operations_.end())
            return std::function<Sig>();
        else
            return it->second->as<Sig>();
    }
    /*! \brief Return the list of peers. This can be used in a task to access to the operation of the peers.
     *  \return The list of peers
     */
    const std::list<std::shared_ptr<TaskContext> > & peers() const { return peers_; }
    /*! \brief The name of the derived instantiated class.
     *  This is usefull for example when iterating over the peers to search for one of a specific type.
     *  \return The name of the task, should be equal to the name of the derived class.
     */
    const std::string & name() const { return name_; }
    /*! \brief When there are multiple instantiation of the same task, this allows to identify between them.
     *  The loader guarantee that there cannot be two task with the same instantiation name even if of different type.
     *  \return The name of the specific instantiation of the task.
     */
    const std::string &instantiationName() const { return instantiation_name_;}
    /*!
     *  \param name The name of a port.
     *  \return The point to the port if a port with name exists, nullptr otherwise.
     */
    std::shared_ptr<PortBase> port(const std::string &name);
    /*! Allows to iterate over all the ports.
     *  \return The container of the ports.
     */
    const std::unordered_map<std::string, std::shared_ptr<PortBase> > & ports() const { return ports_; }
    /*! \brief Allows to set a documentation of the task.
     *  The documentation is written in the configuration file when generated automatically.
     *  \param doc The documentation associated with the service.
     */
    void setDoc(std::string doc) { doc_ = doc; }
    /*!
     *  \return The documentation associated with the service.
     */
    const std::string & doc() const { return doc_; }
//      /*!
//   *  \return Pointer to itself
//   */
//    std::shared_ptr<Service> provides() { return std::make_shared<this; }
//  /*! \brief Search in the list of subservices and return the one with the given name.
//   *  \param name Name of the subservice.
//   *  \return Pointer to the service if it exist, nullptr otherwise
//   */
//    const std::unique_ptr<Service> &provides(const std::string &name);
    /*!
     * \return The shared pointer for the class
     */
    std::shared_ptr<Service> baseSharedPtr()
    {
        return shared_from_this();
    }
private:
    friend class AttributeBase;
    friend class OperationBase;
    friend class PortBase;
    friend class ExecutionEngine;
    friend class CocoLauncher;
    friend class GraphLoader;
    friend class XMLCreator;
    friend class LibraryParser;
    /*! \brief Add an attribute to the component.
     *  \param attribute Pointer to the attribute to be added to the component.
     */
    bool addAttribute(std::shared_ptr<AttributeBase> &attribute);
    /*!
     *  \param name The name of an attribute.
     *  \return Pointer to the attribute with that name. Null if no attribute with name exists.
     */
    std::shared_ptr<AttributeBase> attribute(const std::string &name);
    /*!
     *  \param name The name of an attribute.
     *  \return Reference to the attribute with that name. Raise exception if no attribute with name exists.
     */
    template <class T>
    T & attributeRef(std::string name)
    {
        auto it = attributes_.find(name);
        if (it != attributes_.end())
            return it->second->as<T>();
        else
            throw std::exception();
    }
    /*! Allows to iterate over all the attributes.
     *  \return The container of the attributes.
     */
    const std::unordered_map<std::string, std::shared_ptr<AttributeBase> > & attributes() const { return attributes_; }
    /*!
     *  \param port The port to be added to the list of ports.
     *  \return Wheter a port with the same name exists. In that case the port is not added.
     */
    bool addPort(std::shared_ptr<PortBase> &port);
    /*!
     *  \param operation The operation to be added to the component.
     */
    bool addOperation(std::shared_ptr<OperationBase> &operation);
    /*!
     * \return Wheter the component has pending enqueued operation to be executed.
     */
    bool hasPending() const;
    /*! \brief Execute and remove the first pending operation.
     */
    void stepPending();
    /*! \brief Add a peer to the list of peers, one peer object cannot be associated to more than one task.
     *  \param peer Pointer to the peer.
     */
    void addPeer(std::shared_ptr<TaskContext> &peer);
//  /*! \brief Provides the map<name, ptr> of all the subservices
//   *  \return The map of the subservices associated with this task.
//   */
//  const std::unordered_map<std::string,std::unique_ptr<Service> > & services() const { return subservices_; }
    /*! \brief Set the name of the Task. This function is called during the loading phase
     *   with the name of the derived class being instantiated.
     *  The name should no be modified afterwards.
     *  \param name The name of the task
     */
    void setName(std::string name) { name_ = name; }
    /*! \brief This name is used to distinguish between multiple instantiation of the same task.
     *  Multiple tasks have the same name, so to distinguish between them \ref instantiation_name_ is used.
     *  This name must be set by the launcher and never be modified.
     *  \param name The custom name for this specific instantiation of the task.
     */
    void setInstantiationName(const std::string &name) { instantiation_name_ = name; }

private:
    std::string name_;
    std::string instantiation_name_ = "";
    std::string doc_;

    std::list<std::shared_ptr<TaskContext> > peers_;
    std::unordered_map<std::string, std::shared_ptr<PortBase> > ports_;
    std::unordered_map<std::string, std::shared_ptr<AttributeBase> > attributes_;
    std::unordered_map<std::string, std::shared_ptr<OperationBase> > operations_;
    std::list<OperationInvocation> asked_ops_;

    std::mutex op_mutex_;

//    std::unordered_map<std::string, std::unique_ptr<Service> > subservices_;
};

/*! \brief Specify the current state of a task.
 */
enum TaskState
{
    INIT            = 0,  //!< The task is executing the initialization function.
    PRE_OPERATIONAL = 1,  //!< The task is running the enqueued operation.
    RUNNING         = 2,  //!< The task is running normally.
    IDLE            = 3,  //!< The task is idle, either waiting on a timeout to expire or on a trigger.
    STOPPED         = 4,  //!< The task has been stopped and it is waiting to terminate.
};


class Activity;
class ExecutionEngine;

/*!
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
    /*! \brief Return the current state of the execution of the task.
     *  \return The state of the task.
     */
    TaskState state() const { return state_; }
    /*!
     *  \return The type of the task.
     */
    const std::type_info & type() const { return *type_info_; }
    /*! \brief Set the type of the task. This is called by COCO_REGISTER when instantiating the component.
     *  Once it is set can never be changed.
     */
    template<class T>
    void setType()
    {
        static bool set = false;
        if (!set)
        {
            type_info_ = &(typeid(T));
            set = true;
        }
    }
    /*!
     * \brief This function allows to know if two tasks are executing on the same thread.
     * \param other The other task that we want to check
     * \return wheter the two tasks are executing on the same thread
     */
    bool isOnSameThread(const std::shared_ptr<TaskContext> &other) const;
    /*!
     * \return The shared pointer for the class
     */
    std::shared_ptr<TaskContext> sharedPtr()
    {
        return std::static_pointer_cast<TaskContext>(baseSharedPtr());
    }

protected:
    friend class ExecutionEngine;

    /*! \brief Create an empty task
     */
    TaskContext();
    /*! \brief To be override by the user in the derived class.
     *  Initalization function called in the main thread, before passing the component to the activity thread.
     *  It is guaranteed that before any thread is spawned all the init function have been called and have compleated.
     */
    virtual void init() = 0;
    /*! \brief To be override by the user in the derived class.
     *  Called in the activity thread before entering in the main execution loop.
     *  There is no synchronization between config function of different tasks. This means that a task can enter in
     *  the main loop while another task is still in the onConfig function.
     */
    virtual void onConfig() = 0;
    /*! \brief To be override by the user in the derived class.
     *  The function called in the loop execution.
     */
    virtual void onUpdate() = 0;
    /*! \brief To be override by the user in the derived class.
     *  Called by the activity before terminationg. Can be used to safely release resources.
     */
    virtual void stop();

private:
    friend class CocoLauncher;
    friend class GraphLoader;
    friend class PortBase;
    /*! \brief Pass to the task the pointer to the activity using it.
     *  This is usefull for propagating trigger from port to activity.
     *  \param activity The pointer to the activity.
     */
    void setActivity(std::shared_ptr<Activity> & activity) { activity_ = activity; }
    /*! \brief When a trigger from an input port is recevied the trigger is propagated to the owing activity.
     */
    void triggerActivity(const std::string &port_name);
    /*! \brief When the data from a triggered input port is read, it decreases the trigger count from the owing activity.
     */
    void removeTriggerActivity();
    /*! \brief Pass to the task the pointer to the engine using managing the task.
     *  \param engine Shared pointer to the engine.
     */
    void setEngine(std::shared_ptr<ExecutionEngine> engine) { engine_ = engine; }
    /*! \brief Return the \ref ExecutionEngine owing the task.
     *  \return A shared pointer to the engine object owing the task.
     */
    std::shared_ptr<ExecutionEngine>  engine() const { return engine_; }
    /*! \brief Set the current state of the task.
     *  \param state The state.
     */
    void setState(TaskState state) { state_  = state; }  // TODO analyse concurrency issues.
    /*!
     * \brief Add the port to the list of events port. This is used when a component need to be trigged when all the
     * event ports have received data. This function is called when an event port is connected, so doesn't apply to
     * not connected port.
     * \param name The name of the port
     */
    void addEventPort(const std::string &name) { ++event_port_num_; }

private:
    std::shared_ptr<Activity> activity_;  // TaskContext is owned by activity
    std::atomic<TaskState> state_;
    const std::type_info *type_info_;
    std::shared_ptr<ExecutionEngine> engine_;  // ExecutionEngine is owned by activity

    std::unordered_set<std::string> event_ports_;
    unsigned int event_port_num_ = 0;
    bool forward_check_ = true;

    std::unique_ptr<AttributeBase> att_wait_all_trigger_;
    bool wait_all_trigger_ = false;
};

/*!
 * Class to create peer to be associated to taskcomponent
 */
class PeerTask : public TaskContext
{
public:
    void init() {}
    void onUpdate() {}
};

}  // end of namespace coco
