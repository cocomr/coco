#pragma once

#include "coco/register.h"
#include "coco/util/logging.h"

#include "graph_spec.h"

namespace coco
{

class LibraryParser
{
public:
    bool loadLibrary(const std::string &library,
                     std::shared_ptr<TaskGraphSpec> app_spec);

private:
    void loadComponent(const std::string &name);

    std::shared_ptr<TaskGraphSpec> app_spec_;
    std::string library_;

};



} // end of namespace coco
