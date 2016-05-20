
#include "graph_loader.h"


namespace coco
{

void GraphLoader::enableProfiling(bool profiling)
{
    ComponentRegistry::enableProfiling(profiling);
}
void GraphLoader::loadGraph(std::shared_ptr<TaskGraphSpec> app_spec,
                            std::unordered_set<std::string> disabled_components)
{
	app_spec_ = app_spec;
    disabled_components_ = disabled_components;
    std::cout << "Disabled component: " << disabled_components_.size() << std::endl;
    /* Launch activitie
     * Activities and the component inside them, are guaranteed to be loaded,
     * with the same oredr as they are encountered in the xml file.
     * Components are loaded in the order they appear inside activities not
     * in the order they are declared.
     */
	COCO_DEBUG("GraphLoader") << "Starting Activities";
	for (auto & activity : app_spec_->activities)
		startActivity(activity);

    /* Make connections */
    COCO_DEBUG("GraphLoader") << "Making conenctions";
    for (auto & connection : app_spec_->connections)
        makeConnection(connection);

    // Removing the peers from the task list
//    for (auto it : peers_)
//    {
//        auto t = tasks_.find(it);
//        if (t != tasks_.end())
//          tasks_.erase(t);
//    }

    // For each activity specify which are the free core where to run
    std::list<unsigned> available_core_id;
    for (unsigned int i = 0; i < std::thread::hardware_concurrency(); ++i)
        if (assigned_core_id_.find(i) == assigned_core_id_.end())
            available_core_id.push_back(i);

    for (auto activity : activities_)
        activity->policy().available_core_id = available_core_id;

    coco::ComponentRegistry::setResourcesPath(app_spec_->resources_paths);
}

void GraphLoader::startActivity(std::shared_ptr<ActivitySpec> &activity_spec)
{
    SchedulePolicy policy;

    if (activity_spec->policy.type == "triggered")
        policy.scheduling_policy = SchedulePolicy::TRIGGERED;
    else if (activity_spec->policy.type == "periodic")
        policy.scheduling_policy = SchedulePolicy::PERIODIC;
    else
        COCO_FATAL() << "Schduele policy type: " << activity_spec->policy.type
                     << " is not know\n Possibilities are: triggered, periodic";

    policy.period_ms = activity_spec->policy.period;

    policy.affinity = -1;
    if (activity_spec->policy.affinity >= 0)
    {
        if (activity_spec->policy.affinity < std::thread::hardware_concurrency() &&
            assigned_core_id_.find(activity_spec->policy.affinity) == assigned_core_id_.end())
        {
            policy.affinity = activity_spec->policy.affinity;
            if (activity_spec->policy.exclusive)
                assigned_core_id_.insert(activity_spec->policy.affinity);
        }
        else
        {
            COCO_FATAL() << "Core " << activity_spec->policy.affinity
                         << " either doesn't exist or it has already been assigned\
                              exclusivly to another activity!";
        }
    }

    unsigned int task_count = 0;
    for (auto & task : activity_spec->tasks)
    {
        if (loadTask(task, nullptr))
        {
            ++task_count;
        }
    }

    // If all components in activity are disabled, don't instantiate the actvity.
    if (task_count == 0)
    {
        return;
    }

    Activity *activity = nullptr;
    if (activity_spec->is_parallel)
        activity = new ParallelActivity(policy);
    else
        activity = new SequentialActivity(policy);

     activities_.push_back(std::shared_ptr<Activity>(activity));

    for (auto & task_spec : activity_spec->tasks)
    {
        if (disabled_components_.count(task_spec->instance_name) != 0)
            continue;
        auto & task = tasks_[task_spec->instance_name];
        activity->addRunnable(task->engine());
        task->setActivity(activity);
    }
}

bool GraphLoader::loadTask(std::shared_ptr<TaskSpec> &task_spec, TaskContext * task_owner)
{
    // Issue: In this way, rightly, are disabled also all the peers of a given task.
    //std::cout <<
    if (disabled_components_.count(task_spec->instance_name) != 0)
    {
        COCO_DEBUG("GraphLoader") << "Task " << task_spec->instance_name << " disabled by launcher";
        return false;
    }

    COCO_DEBUG("GraphLoader") << "Loading " << (task_spec->is_peer ? "peer" : "task") << ": " << task_spec->instance_name;
    TaskContext * task = ComponentRegistry::create(task_spec->name, task_spec->instance_name);
	if (!task)
	{
        COCO_DEBUG("GraphLoader") << "Component " << task_spec->instance_name <<
                       " not found, trying to load from library";

        if (!ComponentRegistry::addLibrary(task_spec->library_name))
            COCO_FATAL() << "Failed to load library " << task_spec->library_name;

        task = ComponentRegistry::create(task_spec->name, task_spec->instance_name);
        if (!task)
        {
            COCO_FATAL() << "Failed to create component: " << task_spec->name;
        }
	}
    task->setName(task_spec->name);
    task->setInstantiationName(task_spec->instance_name);
    if (!task_spec->is_peer)
    {
        task->setEngine(std::make_shared<ExecutionEngine>(task));
    }

    if (tasks_.find(task_spec->instance_name) != tasks_.end())
        COCO_FATAL() << "Trying to instantiate two task with the same name: "
                     << task_spec->instance_name;

    tasks_[task_spec->instance_name].reset(task);

    COCO_DEBUG("GraphLoader") << "Loading attributes";
    for (auto & attribute : task_spec->attributes)
    {
        if (task->attribute(attribute.name))
            task->attribute(attribute.name)->setValue(attribute.value);
        else
            COCO_ERR() << "Attribute: " << attribute.name << " doesn't exist";
    }

    // Parsing possible peers
    COCO_DEBUG("GraphLoader") << "Loading possible peers";
    for (auto & peer : task_spec->peers)
        loadTask(peer, task);

    // TBD: better do that at the very end of loading process
    task->init();

    if (task_owner)
        task_owner->addPeer(task);

    return true;
}

void GraphLoader::makeConnection(std::shared_ptr<ConnectionSpec> &connection_spec)
{
    ConnectionPolicy policy(connection_spec->policy.data,
                            connection_spec->policy.policy,
                            connection_spec->policy.transport,
                            connection_spec->policy.buffersize);

    // if not present means the task has been disabled!
    auto src_task = tasks_.find(connection_spec->src_task->instance_name);
    auto dest_task = tasks_.find(connection_spec->dest_task->instance_name);
    if (src_task == tasks_.end() || dest_task == tasks_.end())
        return;

    PortBase * left = tasks_[connection_spec->src_task->instance_name]->port(connection_spec->src_port);
    PortBase * right = tasks_[connection_spec->dest_task->instance_name]->port(connection_spec->dest_port);
    if (left && right)
    {
        // TBD: do at the very end of the loading (first load then execute)
        left->connectTo(right, policy);
    }
    else
    {
        COCO_FATAL() << "Either Component src: " << connection_spec->src_task->instance_name
                     << " doesn't have port: " << connection_spec->src_port
                     << " or Component in: " << connection_spec->dest_task->instance_name
                     << " doesn't have port: " << connection_spec->dest_port;
    }

}

void GraphLoader::startApp()
{
    COCO_DEBUG("Loader") << "Starting the Activities!";
    if (activities_.size() <= 0)
    {
        COCO_FATAL() << "No app created, first run createApp()";
    }
    std::vector<std::shared_ptr<Activity> > seq_act_list;
    for (auto act : activities_)
    {
        if (dynamic_cast<SequentialActivity *>(act.get()))
        {
            seq_act_list.push_back(act);
            continue;
        }
        act->start();
    }
    if (seq_act_list.size() > 0)
    {
        if (seq_act_list.size() > 1)
            COCO_ERR() << "Only one sequential activity per application is allowed.\
                           Only the first will be run!";
        seq_act_list[0]->start();
    }
}


void GraphLoader::waitToComplete()
{
    for (auto activity : activities_)
    {
        activity->join();
    }
}

void GraphLoader::terminateApp()
{
    for (auto activity : activities_)
        activity->stop();
    waitToComplete();
}



void GraphLoader::printGraph(const std::string& filename) const
{
    std::string dot_file_name = filename + ".dot";
    std::ofstream dot_file(dot_file_name.c_str());

    dot_file << "digraph {\n";
    dot_file << "graph[rankdir=LR, center=true  ];\n";
    //dot_file << "rankdir=LR;\n";
    //dot_file << "splines=ortho;\n";
    int activity_count = 0;
    int subgraph_count = 0;
    int node_count = 0;
    std::unordered_map<std::string, int> graph_port_nodes;

    for (auto activity : activities_)
    {
        dot_file << "subgraph cluster_" << subgraph_count++ << "{\n"
                 << "color = blue;\n"
                 << "label = \"activity " << activity_count++ << ": "
                 << (activity->isPeriodic() ? "Periodic" : "Triggered");
        if (activity->isPeriodic())
            dot_file << " (" << activity->period() << " ms)";
        dot_file << " \";\n"; // TODO add schedule policy

        // Add the components
        for (auto& runnable : activity->runnables())
        {
            auto task = static_cast<ExecutionEngine *>(runnable.get())->task();
            dot_file << "subgraph cluster_" << subgraph_count++ << "{\n"
                     << "color = red;\n"
                     << "label = \"Component: " << task->name() << "\\nName: " <<  task->instantiationName() << "\";\n";

            // Add port
            for (auto port : impl::values_iteration(task->ports()))
            {
                createGraphPort(port, dot_file, graph_port_nodes, node_count);
            }

            // Add peer
            for (auto peer : task->peers())
            {
                createGraphPeer(peer, dot_file, graph_port_nodes, subgraph_count, node_count);
            }
            dot_file << "}\n";
        }
        dot_file << "}\n";
    }
    // Add connections
    for (auto task : impl::values_iteration(tasks_))
    {
        createGraphConnection(task.get(), dot_file, graph_port_nodes);
    }

    dot_file << "}" << std::endl;

    dot_file.close();

    std::string cmd = "dot " + dot_file_name + " -o " + filename + std::string(".pdf") + " -Tpdf";
    int res = std::system(cmd.c_str());

}

void GraphLoader::createGraphPort(PortBase *port, std::ofstream &dot_file,
                                   std::unordered_map<std::string, int> &graph_port_nodes,
                                   int &node_count) const
{
    dot_file << node_count << "[color="
             << (port->isOutput() ? "darkgreen" : "darkorchid4")
             << ", shape=ellipse, label=\"" << port->name();
    if (port->isEvent())
        dot_file << "\\n(event)";
    dot_file << "\"];\n";
    std::string name_id = port->task()->instantiationName() + port->name();
    graph_port_nodes[name_id] = node_count++;
}

void GraphLoader::createGraphPeer(TaskContext *peer, std::ofstream &dot_file,
                                   std::unordered_map<std::string, int> &graph_port_nodes,
                                   int &subgraph_count, int &node_count) const
{
    if (peer->ports().size() > 0 || peer->peers().size() > 0)
    {
        dot_file << "subgraph cluster_" << subgraph_count++ << "{\n"
                 << "color = red;\n"
                 << "style = rounded;\n"
                 << "label = \"Component: " << peer->name() << "\\nName: "
                 <<  peer->instantiationName() << "\";\n";
    }
    else
    {
        dot_file << node_count++ << "[color=red, shape=box, style=rounded, label = \"Component: "
                 << peer->name() << "\\nName: " <<  peer->instantiationName() << "\"];\n";
        return;
    }

    // Add port
    for (auto port : impl::values_iteration(peer->ports()))
    {
        if (port->isConnected())
            createGraphPort(port, dot_file, graph_port_nodes, node_count);
    }

    for (auto sub_peer : peer->peers())
        createGraphPeer(sub_peer, dot_file, graph_port_nodes, subgraph_count, node_count);

    dot_file << "}\n";
}

void GraphLoader::createGraphConnection(TaskContext *task, std::ofstream &dot_file,
                                         std::unordered_map<std::string, int> &graph_port_nodes) const
{
    for (auto port : impl::values_iteration(task->ports()))
    {
        if (!port->isOutput())
            continue;
        std::string this_id = task->instantiationName() + port->name();

        int src = graph_port_nodes.at(this_id);

        auto connections = port->connectionManager().connections();
        for (auto connection : connections)
        {
            std::string port_id = connection->input()->task()->instantiationName() + connection->input()->name();
            int dst = graph_port_nodes.at(port_id);
            dot_file << src << " -> " << dst << ";\n";
        }
    }
    for (auto peer : task->peers())
        createGraphConnection(peer, dot_file, graph_port_nodes);
}

};
