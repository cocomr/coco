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
        COCO_ERR() << "The received TaskGraphSpec pointer is not initialized";
        return false;
    }
    app_spec_ = app_spec;
    library_ = library;


    if (!ComponentRegistry::addLibrary(library))
    {
        COCO_ERR() << "Failed to load library: " << library;
        return false;
    }

    for (auto &component : util::keys_iteration(ComponentRegistry::components()))
    {
        COCO_DEBUG("LibraryParser") << " loading component: " << component;
        loadComponent(component);
    }

    return true;
}


void LibraryParser::loadComponent(const std::string &name)
{
    auto task = ComponentRegistry::create(name, name);

    std::shared_ptr<TaskSpec> task_spec = std::make_shared<TaskSpec>();
    task_spec->name = name;
    task_spec->library_name = library_;

    if (std::dynamic_pointer_cast<PeerTask>(task))
        task_spec->is_peer = true;
    else
        task_spec->is_peer = false;

    for (auto &attribute : task->attributes())
    {
        task_spec->attributes.push_back(std::move(AttributeSpec(attribute.first, "")));
    }

//    for (auto &port : util::values_iteration(task->ports()))
//    {

//    }
}



}
