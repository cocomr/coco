#pragma once

#include "graph_spec.h"
#include "register.h"
#include "util/logging.h"

namespace coco
{

class LibraryParser
{
public:
    LibraryParser()
    {
        COCO_INIT_LOG();
    }

    bool loadLibrary(const std::string &library,
                     std::shared_ptr<TaskGraphSpec> app_spec);

private:
    void loadComponent(const std::string &name);

    std::shared_ptr<TaskGraphSpec> app_spec_;
    std::string library_;

};



} // end of namespace coco
