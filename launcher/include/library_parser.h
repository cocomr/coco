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

#include "coco/register.h"
#include "coco/util/logging.h"

#include "graph_spec.h"

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
