/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#include "library_parser.h"
#include "coco/util/accesses.hpp"

namespace coco
{

bool LibraryParser::loadLibrary(const std::string &library,
                                std::shared_ptr<TaskGraphSpec> app_spec)
{
    if (!app_spec)
    {
        COCO_FATAL() << "The received TaskGraphSpec pointer is not initialized";
        return false;
    }
    app_spec_ = app_spec;
    library_ = library;

    if (!ComponentRegistry::addLibrary(library))
    {
        COCO_ERR() << "Failed to load library (ComponentRegistry::addLibrary): " << library;
        return false;
    }

    for (auto &component : util::keys_iteration(ComponentRegistry::components()))
    {
        COCO_DEBUG("LibraryParser") << " loading component: " << component;
        loadComponent(component);
    }
    COCO_DEBUG("LibraryParser") << " finished loading library " << library;
    
    return true;
}


void LibraryParser::loadComponent(const std::string &name)
{
    auto task = ComponentRegistry::create(name, name);

    std::shared_ptr<TaskSpec> task_spec = std::make_shared<TaskSpec>();
    task_spec->name = name;
    task_spec->instance_name = name;
    task_spec->library_name = library_;
    task_spec->is_peer = isPeer(task);
    
    for (auto &attribute : task->attributes())
    {
        task_spec->attributes.push_back((AttributeSpec(attribute.first, "")));
    }
    app_spec_->tasks[name] = task_spec;

   for (auto &port : util::values_iteration(task->ports()))
   {
        std::unique_ptr<ConnectionSpec> connection(new ConnectionSpec());
        if (!port->isOutput())
        {
            connection->dest_task = task_spec;
            connection->dest_port = port->name();
        }
        else
        {
            connection->src_task = task_spec;
            connection->src_port = port->name();
        }
        app_spec_->connections.push_back(std::move(connection));
   }
}

}  // end of namespace coco
