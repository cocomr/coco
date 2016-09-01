/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#include <string>

#include "coco/task.h"
#include "coco/connection.h"

namespace coco
{

ConnectionPolicy::ConnectionPolicy()
    : data_policy(DATA), lock_policy(LOCKED),
      buffer_size(1), init(false), transport(LOCAL)
{}

ConnectionPolicy::ConnectionPolicy(const std::string & policy,
                                   const std::string & lock,
                                   const std::string & transport_type,
                                   const std::string & size)
{
    // look here http://tinodidriksen.com/2010/02/16/cpp-convert-string-to-int-speed/
    buffer_size = atoi(size.c_str());  // boost::lexical_cast<int>(buffer_size);
    if (policy.compare("DATA") == 0)
        data_policy = DATA;
    else if (policy.compare("BUFFER") == 0)
        data_policy = BUFFER;
    else if (policy.compare("CIRCULAR") == 0)
        data_policy = CIRCULAR;
    else
        COCO_FATAL() << "Failed to parse connection policy data type: "
                     << policy;
    if (lock.compare("UNSYNC") == 0)
        lock_policy = UNSYNC;
    else if (lock.compare("LOCKED") == 0)
        lock_policy = LOCKED;
    else if (lock.compare("LOCK_FREE") == 0)
        lock_policy = LOCK_FREE;
    else
        COCO_FATAL() << "Failed to parse connection policy lock type: "
                     << lock;
    if (transport_type.compare("LOCAL") == 0)
        transport = LOCAL;
    else if (transport_type.compare("IPC") == 0)
        transport = IPC;
    else
        COCO_FATAL() << "Failed to parse connection policy transport type: "
                     << transport_type;
}

ConnectionBase::ConnectionBase(std::shared_ptr<PortBase> in,
                               std::shared_ptr<PortBase> out,
                               ConnectionPolicy policy)
    : input_(in), output_(out),
      data_status_(NO_DATA), policy_(policy)
{}

bool ConnectionBase::hasNewData() const
{
    return data_status_ == NEW_DATA;
}

bool ConnectionBase::hasComponent(const std::string &name) const
{
    if (input_->task()->instantiationName() == name)
        return true;
    if (output_->task()->instantiationName() == name)
        return true;
    return false;
}

void ConnectionBase::trigger()
{
    input_->triggerComponent();
}

void ConnectionBase::removeTrigger()
{
    input_->removeTriggerComponent();
}

bool ConnectionManager::addConnection(
        std::shared_ptr<ConnectionBase> connection)
{
    connections_.push_back(connection);
    return true;
}

bool ConnectionManager::hasConnections() const
{
    return connections_.size() != 0;
}

int ConnectionManager::queueLenght(int connection) const
{
    if (connection > static_cast<int>(connections_.size()))
        return 0;
    if (connection >= 0)
        return connections_[connection]->queueLength();

    unsigned int lenght = 0;
    for (auto & conn : connections_)
    {
        lenght += conn->queueLength();
    }
    return lenght;
}

int ConnectionManager::connectionsCount() const
{
    return connections_.size();
}


}  // end of namespace coco
