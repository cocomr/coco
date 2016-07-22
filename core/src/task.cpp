#include <string>

#include "coco/connection.h"
#include "coco/execution.h"
#include "coco/task_impl.hpp"
#include "coco/task.h"

namespace coco
{

// -------------------------------------------------------------------
// Attribute
// -------------------------------------------------------------------

AttributeBase::AttributeBase(TaskContext *task,
                             const std::string &name)
    : name_(name)
{
    std::shared_ptr<AttributeBase> shared_this(this);
    task->addAttribute(shared_this);
}

// --------------------------------------------------------------s-----
// Operation
// -------------------------------------------------------------------

OperationBase::OperationBase(TaskContext *task,
                             const std::string &name)
    : name_(name)
{
    std::shared_ptr<OperationBase> shared_this(this);
    task->addOperation(shared_this);
}

OperationInvocation::OperationInvocation(const std::function<void(void)> & f)
    : fx(f)
{}

// -------------------------------------------------------------------
// Port
// -------------------------------------------------------------------

PortBase::PortBase(TaskContext *task, const std::string &name,
                   bool is_output, bool is_event)
    : task_(task), name_(name), is_output_(is_output), is_event_(is_event)
{
    std::shared_ptr<PortBase> shared_this(this);
    task->addPort(shared_this);
}

bool PortBase::isConnected() const
{
    return manager_->hasConnections();
}

unsigned int PortBase::connectionsCount() const
{
    return manager_->connectionsCount();
}

unsigned int PortBase::queueLength(int connection) const
{
    return manager_->queueLenght();
}

void PortBase::triggerComponent()
{
    task_->triggerActivity(this->name_);
}

void PortBase::removeTriggerComponent()
{
    task_->removeTriggerActivity();
}

bool PortBase::addConnection(std::shared_ptr<ConnectionBase> &connection)
{
    if (!is_output_ && is_event_)
    {
        task_->addEventPort(name_);
    }

    manager_->addConnection(connection);
    return true;
}

std::shared_ptr<TaskContext> PortBase::task() const
{ return task_->sharedPtr(); }
// -------------------------------------------------------------------
// Service
// -------------------------------------------------------------------

Service::Service(const std::string &name)
    : name_(name)
{}

bool Service::addAttribute(std::shared_ptr<AttributeBase> &attribute)
{
    if (attributes_[attribute->name()])
    {
        COCO_ERR() << "An attribute with name: " << attribute->name()
                   << " already exist\n";
        return false;
    }
    attributes_[attribute->name()] = attribute;
    return true;
}

std::shared_ptr<AttributeBase> Service::attribute(const std::string &name)
{
    auto it = attributes_.find(name);
    if (it == attributes_.end())
    {
        return nullptr;
    }
    else
    {
        return it->second;
    }
}

bool Service::addPort(std::shared_ptr<PortBase> &port)
{
    if (ports_[port->name()])
    {
        COCO_ERR() << instantiationName() <<  ": A port with name: "
                   << port->name() << " already exist";
        return false;
    }
    else
    {
        ports_[port->name()] = port;
        return true;
    }
}

std::shared_ptr<PortBase> Service::port(const std::string &name)
{
    auto it = ports_.find(name);
    if (it == ports_.end())
            return nullptr;
        else
            return it->second;
}

bool Service::addOperation(std::shared_ptr<OperationBase> &operation)
{
    if (operations_[operation->name()])
    {
        COCO_ERR() << "An operation with name: " << operation->name() << " already exist";
        return false;
    }
    operations_[operation->name()] = operation;
    return true;
}

bool Service::hasPending() const
{
    return !asked_ops_.empty();
}

void Service::stepPending()
{
    auto op = asked_ops_.front();
    asked_ops_.pop_front();
    op.fx();
}

void Service::addPeer(std::shared_ptr<TaskContext> & peer)
{
    peers_.push_back(peer);
}

// const std::unique_ptr<Service> &Service::provides(const std::string &name)
// {
//   auto it = subservices_.find(name);
//   if(it == subservices_.end())
//   {
//        std::unique_ptr<Service> service(new Service(name));
//        subservices_[name] = std::move(service);
//        return subservices_[name];
//  }
//  else
//        return it->second;
//  return nullptr;
// }

TaskContext::TaskContext()
{
    state_ = TaskState::IDLE;
    std::unique_ptr<AttributeBase> attribute(new Attribute<bool>(this, "wait_all_trigger", wait_all_trigger_));
    att_wait_all_trigger_.swap(attribute);
}

void TaskContext::stop()
{}

bool TaskContext::isOnSameThread(const std::shared_ptr<TaskContext> &other) const
{
    return this->actvityId() == other->actvityId();
}

uint32_t TaskContext::actvityId() const
{
    return activity_->id();
}

void TaskContext::triggerActivity(const std::string &port_name)
{
    if (!wait_all_trigger_)
    {
        activity_->trigger();
        return;
    }

//    if (this->instantiationName() == "middle_sink")
//        std::cout << port_name << " " << event_port_num_ << " " << forward_check_ << std::endl;

    /* Check whether all the ports have been triggered.
     * This is done checking the size of the unordered_set.
     * If size equal total number of ports, trigger.
     * To avoid emptying the set, the check is reversed and items are removed
     * till size is zero and activity triggered.
     */
    std::unique_lock<std::mutex> lock(all_trigger_mutex_);
    if (forward_check_)
    {
        event_ports_.insert(port_name);
        if (event_ports_.size() == event_port_num_)
        {
            activity_->trigger();
            forward_check_ = false;
        }
    }
    else
    {
        event_ports_.erase(port_name);
        if (event_ports_.size() == 0)
        {
            activity_->trigger();
            forward_check_ = true;
        }
    }
}

void TaskContext::removeTriggerActivity()
{
    activity_->removeTrigger();
}

util::TimeStatistics TaskContext::timeStatistics()
{
    return engine_->timeStatistics();
}

void TaskContext::resetTimeStatistics()
{
    return engine_->resetTimeStatistics();
}

std::shared_ptr<ExecutionEngine> TaskContext::engine() const
{
    return engine_;
}

void TaskContext::setTaskLatencySource()
{
    engine_->latency_timer.source = true;
}
void TaskContext::setTaskLatencyTarget(std::shared_ptr<TaskContext> task)
{
    engine_->latency_timer.source_task = task;
    engine_->latency_timer.target = true;
}

uint32_t PeerTask::actvityId() const
{
    return father_->actvityId();
}

std::shared_ptr<ExecutionEngine> PeerTask::engine() const
{
    return father_->engine();
}

}  // end of namespace coco

