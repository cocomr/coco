/**
Copyright 2015, Filippo Brizzi"
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

@Author 
Filippo Brizzi, PhD Student
fi.brizzi@sssup.it
Emanuele Ruffaldi, PhD
e.ruffaldi@sssup.it

PERCRO, (Laboratory of Perceptual Robotics)
Scuola Superiore Sant'Anna
via Luigi Alamanni 13D, San Giuliano Terme 56010 (PI), Italy
*/

#pragma once

#include <unordered_map>
#include <exception>

#include "tinyxml2/tinyxml2.h"
#include "core_impl.hpp"
#include "register.h"

#include "graph_spec.h"

namespace coco
{

class GraphLoader
{
public:
	void loadGraph(std::shared_ptr<TaskGraphSpec> app_spec);
	void startApp();
	void waitToComplete();
    void terminateApp();

private:
	void loadTask(std::shared_ptr<TaskSpec> &task_spec);
	void makeConnection(std::shared_ptr<ConnectionSpec> &connection_spec);
	void startActivity(std::shared_ptr<ActivitySpec> &activity_spec);

	std::shared_ptr<TaskGraphSpec> app_spec_;

	std::unordered_map<std::string, TaskContext*> tasks_;
    std::vector<Activity *> activities_;

    std::list<std::string> peers_;

	std::unordered_set<int> assigned_core_id_;
};

};