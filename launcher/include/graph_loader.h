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

#include <unordered_map>
#include <exception>

#include "tinyxml2/tinyxml2.h"

#include "coco/register.h"
#include "graph_spec.h"

namespace coco
{

class GraphLoader
{
public:
    void loadGraph(std::shared_ptr<TaskGraphSpec> app_spec,
                   std::unordered_set<std::string> disabled_components);
    void enableProfiling(bool profiling);
	void startApp();
	void waitToComplete();
    void terminateApp();

    void printGraph(const std::string& filename) const;
    std::string graphSvg() const;
    bool writeSvg(const std::string& filename) const;

private:
    void loadSchedule(const SchedulePolicySpec &policy_spec, SchedulePolicy &policy);
    void startActivity(std::unique_ptr<ActivitySpec> &activity_spec);
    void startPipeline(std::unique_ptr<PipelineSpec> &pipeline_spec);
    void startFarm(std::unique_ptr<FarmSpec> &farm_spec);
    bool loadTask(std::shared_ptr<TaskSpec> &task_spec, std::shared_ptr<TaskContext> &task_owner);
    void makeConnection(std::unique_ptr<ConnectionSpec> &connection_spec);

	void checkTaskConnections() const;

    void createGraphPort(std::shared_ptr<PortBase> port, std::ofstream &dot_file,
                         std::unordered_map<std::string, int> &graph_port_nodes,
                         int &node_count) const;
    void createGraphPeer(std::shared_ptr<TaskContext> peer, std::ofstream &dot_file,
                         std::unordered_map<std::string, int> &graph_port_nodes,
                         int &subgraph_count, int &node_count) const;
    void createGraphConnection(std::shared_ptr<TaskContext> & task, std::ofstream &dot_file,
                               std::unordered_map<std::string, int> &graph_port_nodes) const;

private:
	std::shared_ptr<TaskGraphSpec> app_spec_;

    std::unordered_map<std::string, std::shared_ptr<TaskContext>> tasks_;
    std::vector<std::shared_ptr<Activity>> activities_;

    std::list<std::string> peers_;

	std::unordered_set<int> assigned_core_id_;

    std::unordered_set<std::string> disabled_components_;
};

}
