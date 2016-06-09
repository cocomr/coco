#pragma once
#include <memory>
#include <string>
#include <vector>

namespace coco
{

/*! \brief Specify the policy of the connection between two ports.
 *  For every connection there is a policy specifying the buffer type,
 *  the lock type and the transport type. Connections are always non blocking.
 */
struct ConnectionPolicy
{
    /*! \brief Buffer policy
     */
    enum BufferPolicy
    {
        DATA,      //!< Buffer of lenght 1. Incoming data always override existing one.
        BUFFER,    //!< Buffer of lenght \ref buffer_size. If the buffer is full new data is discarded.
        CIRCULAR   //!< Circular FIFO buffer of lenght \ref buffer_size. If the buffer is full new data overrides the oldest one.
    };
    /*! \brief Lock policy for concurrent access management.
     */
    enum LockPolicy
    {
        UNSYNC,    //!< No resource access control policy
        LOCKED,    //!< Data access is regulated by mutexes
        LOCK_FREE  //!< NOT IMPLEMENTED YET (TBD)
    };
    /*! \brief Specifies if the connection is between two threads or between processes.
     */
    enum Transport
    {
        LOCAL, //!< Connection between two thread of the same process. Communication using shared memory.
        IPC    //!< NOT IMPLEMENTED YET (TBD)
    };

    BufferPolicy data_policy;
    LockPolicy lock_policy;
    int buffer_size; 		     //!< Size of the buffer
    bool init = false;
    Transport transport;
    //std::string name_id;

    /*! \brief Default constructor.
     *  Default values:
     *  \param data_policy = DATA
     *	\param lock_policy = LOCKED
     *  \param buffer_size = 1
     * 	\param init = 1
     *  \param transport = LOCAL
     */
    ConnectionPolicy();
    /*! \brief Construct a ConnectionPolicy object with the options parsed from string.
     */
    ConnectionPolicy(const std::string &policy, const std::string &lock,
                     const std::string &transport_type, const std::string &buffer_size);
};

#undef NO_DATA
/*! \brief Status of the data present in a connection buffer,
 */
enum FlowStatus
{
    NO_DATA,   //!< No data present in the buffer.
    OLD_DATA,  //!< There is data in the buffer but it has already been read.
    NEW_DATA   //!< New data in the buffer.
};

class PortBase;

/*! \brief Base class for connections.
 *  Contains the basic funcitons to manage a connection.
 */
class ConnectionBase
{
public:
    /*! Costructor of the connection.
     *  \param in Input port of the connection.
     *  \param out Output port of the connection.
     *  \param policy of the connection.
     */
    ConnectionBase(std::shared_ptr<PortBase> in,
                   std::shared_ptr<PortBase> out,
                   ConnectionPolicy policy);
    virtual ~ConnectionBase() {}

    /*!
     * \return If new data is present in the InputPort.
     */
    bool hasNewData() const;
    /*!
     * \param name name of component.
     * \return If one of the task involved in the connection has name \param name.
     */
    bool hasComponent(const std::string &name) const;
    /*!
     * \return Pointer to the input port.
     */
    const std::shared_ptr<PortBase> & input() const { return input_; }
    /*!
     * \return Pointer to the output port.
     */
    const std::shared_ptr<PortBase> & output() const { return output_; }
    /*!
     * \return The lenght of the queue in the connection
     */
     virtual unsigned int queueLength() const = 0;
protected:
    /*! \brief Call InputPort::triggerComponent() function to trigger the owner component execution.
     */
    void trigger();
    /*! \brief Once data has been read remove the trigger calling InputPort::removeTriggerComponent()
    */
    void removeTrigger();

    std::shared_ptr<PortBase> input_;
    std::shared_ptr<PortBase> output_;

    FlowStatus data_status_;
    ConnectionPolicy policy_;
};

/*!\brief Used to specify to the port factory which connection manager to instantiate.
 */
enum class ConnectionManagerType {DEFAULT = 0, FARM};
/*! Manages the connections of one PortBase
 *  Ports can have multiple connections associated to them.
 *  ConnectionManager keeps track of all these connections.
 *  In case of multiple incoming port manages the round robin
 *  schedule used to access all the connections
 */
class ConnectionManager
{
public:
    /*! Base constructor. Set the round robin index to 0
     * \param port The port owing it.
     */
    ConnectionManager();
    /*!
     * \param connection Shared pointer of the connection to be added at the \ref owner_ port.
     */
    bool addConnection(std::shared_ptr<ConnectionBase> connection);
    /*!
     * \return If the associated port has any active connection.
     */
    bool hasConnections() const;
    /*!
     * \param connection The connection id of which we want to know the lenght, if -1 return for all the
     * connections
     * \return The number of data in the connection specified by the parameter or the sum of the data
     * in all the connection if not specified
     */
    int queueLenght(int connection = -1) const;
    /*!
     * \return Number of connections.
     */
    int connectionsCount() const;

private:
    friend class GraphLoader;
    friend class CocoLauncher;
    const std::vector<std::shared_ptr<ConnectionBase>> & connections() const { return connections_; }

protected:
    int rr_index_; //!< round robin index to poll the connection when reading data
    //const PortBase *owner_; //!< PortBase pointer owning this manager
    std::vector<std::shared_ptr<ConnectionBase> > connections_; //!< List of ConnectionBase associate to \ref owner_
};


}
