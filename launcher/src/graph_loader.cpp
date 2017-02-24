/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#include <unistd.h>
#include "coco/util/accesses.hpp"

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
	/* Launch activitie
	 * Activities and the component inside them, are guaranteed to be loaded,
	 * with the same oredr as they are encountered in the xml file.
	 * Components are loaded in the order they appear inside activities not
	 * in the order they are declared.
	 */
	COCO_DEBUG("GraphLoader") << "Loading " << app_spec_->activities.size()
							  << "  Activities";
	for (auto & activity : app_spec_->activities)
        startActivity(activity);

    COCO_DEBUG("GraphLoader") << "Loading " << app_spec_->pipelines.size() << " Pipelines";
    for (auto & pipeline : app_spec_->pipelines)
        startPipeline(pipeline);

    COCO_DEBUG("GraphLoader") << "Loading " << app_spec_->farms.size() << " Farms";
    for (auto &farm: app_spec_->farms)
        startFarm(farm);
    // TODO Manage ConnectionManager

    /* Make connections */
    COCO_DEBUG("GraphLoader") << "Making connections";
    for (auto & connection : app_spec_->connections)
        makeConnection(connection);

	COCO_DEBUG("GraphLoader") <<
			"Checking that all the components have at least one port connected";
	checkTaskConnections();

    // For each activity specify which are the free core where to run
    std::list<unsigned> available_core_id;
    for (unsigned int i = 0; i < std::thread::hardware_concurrency(); ++i)
        if (assigned_core_id_.find(i) == assigned_core_id_.end())
            available_core_id.push_back(i);
    for (auto activity : activities_)
        activity->policy().available_core_id = available_core_id;

    ComponentRegistry::setResourcesPath(app_spec_->resources_paths);
    ComponentRegistry::setActivities(activities_);
}

void GraphLoader::checkTaskConnections() const
{
	for (auto &task : tasks_)
	{
		bool found = false;
		for (auto & port : task.second->ports_)
		{
			found = port.second->isConnected();
			if (found)
				break;
		}
		if (!found)
			COCO_ERR() << "Component " << task.second->instantiation_name_
					   << " doesn't have any port connected";
	}
}

void GraphLoader::loadSchedule(const SchedulePolicySpec &policy_spec, SchedulePolicy &policy)
{
	if (policy_spec.type == "triggered")
		policy.scheduling_policy = SchedulePolicy::TRIGGERED;
	else if (policy_spec.type == "periodic")
		policy.scheduling_policy = SchedulePolicy::PERIODIC;
	else
		COCO_FATAL()<< "Schduele policy type: " << policy_spec.type
					<< " is not know\n Possibilities are: triggered, periodic";

	policy.period_ms = policy_spec.period;
	policy.priority = policy_spec.priority;
    policy.affinity = -1;
    if (policy_spec.affinity >= 0)
    {
        if (policy_spec.affinity < (int)std::thread::hardware_concurrency() &&
            assigned_core_id_.find(policy_spec.affinity) == assigned_core_id_.end())
        {
            policy.affinity = policy_spec.affinity;
            if (policy_spec.exclusive)
                assigned_core_id_.insert(policy_spec.affinity);
        }
        else
        {
            COCO_FATAL() << "Core " << policy_spec.affinity
                         << " either doesn't exist or it has already"
					     << " been assigned exclusively to another activity!";
        }
    }

	if (policy_spec.realtime == "fifo")
		policy.realtime = SchedulePolicy::FIFO;
	else if (policy_spec.realtime == "rr")
		policy.realtime = SchedulePolicy::RR;
	else if (policy_spec.realtime == "deadline")
		policy.realtime = SchedulePolicy::DEADLINE;
	else
		policy.realtime = SchedulePolicy::NONE;

	policy.priority = policy_spec.priority;
	policy.runtime = policy_spec.runtime;
}

void GraphLoader::startActivity(std::unique_ptr<ActivitySpec> &activity_spec)
{
	SchedulePolicy policy;
	loadSchedule(activity_spec->policy, policy);

	unsigned int task_count = 0;
	for (auto & task_spec : activity_spec->tasks)
	{
		std::shared_ptr<TaskContext> null_task_ptr;
		if (loadTask(task_spec, null_task_ptr))
		{
			++task_count;
		}
	}
	// If all components in activity are disabled, don't instantiate the actvity.
	if (task_count == 0)
	{
		return;
	}

	std::shared_ptr<Activity> activity;
	if (activity_spec->is_parallel)
		activity = std::make_shared<ParallelActivity>(policy);
	else
		activity = std::make_shared<SequentialActivity>(policy);

	activities_.push_back(activity);

	for (auto & task_spec : activity_spec->tasks)
	{
		if (disabled_components_.count(task_spec->instance_name) != 0)
			continue;
		auto & task = tasks_[task_spec->instance_name];
		activity->addRunnable(task->engine());
		task->setActivity(activity);
	}
}

void GraphLoader::startPipeline(std::unique_ptr<PipelineSpec> &pipeline_spec)
{
	SchedulePolicy policy;
	policy.scheduling_policy = SchedulePolicy::TRIGGERED;

	unsigned int task_count = 0;
	for (auto & task : pipeline_spec->tasks)
	{
		std::shared_ptr<TaskContext> null_task_ptr;
		if (loadTask(task, null_task_ptr))
		{
			++task_count;
		}
	}
	// If all components in activity are disabled, don't instantiate the actvity.
	if (task_count == 0)
	{
		return;
	}

	if (pipeline_spec->parallel)
	{
		for (auto & task_spec : pipeline_spec->tasks)
		{
			if (disabled_components_.count(task_spec->instance_name) != 0)
				COCO_FATAL()<< "Cannot disable component "
				<< task_spec->instance_name
				<< " it is inside a pipeline";

            std::shared_ptr<Activity> activity = std::make_shared<
                    ParallelActivity>(policy);
            auto & task = tasks_[task_spec->instance_name];
            activity->addRunnable(task->engine());
            task->setActivity(activity);
            activities_.push_back(activity);
        }
    }
    else
    {
        std::shared_ptr<Activity> activity = std::make_shared<ParallelActivity>(
                policy);
        for (auto & task_spec : pipeline_spec->tasks)
        {
            if (disabled_components_.count(task_spec->instance_name) != 0)
            COCO_FATAL() << "Cannot disable component "
            << task_spec->instance_name
            << " it is inside a pipeline";

            auto & task = tasks_[task_spec->instance_name];
            activity->addRunnable(task->engine());
            task->setActivity(activity);
        }
        activities_.push_back(activity);
    }
}

void GraphLoader::startFarm(std::unique_ptr<FarmSpec> &farm_spec)
{
	
	// Check wheter any of the farm component is disabled.
	// In this case doesn't instantiate the farm
	if (disabled_components_.count(farm_spec->source_task->instance_name) != 0 ||
		disabled_components_.count(farm_spec->gather_task->instance_name) != 0)
		return;
	for (auto & task : farm_spec->pipelines[0]->tasks)
		if (disabled_components_.count(task->instance_name) != 0)
			return;

	// -----------------------
	// Load source task
	// -----------------------
	SchedulePolicy policy_source;
	loadSchedule(farm_spec->source_task_schedule, policy_source);

	std::shared_ptr<TaskContext> null_task_ptr;
	if (!loadTask(farm_spec->source_task, null_task_ptr))
		return;
	std::shared_ptr<Activity> activity_source = std::make_shared<ParallelActivity>(policy_source);
	activities_.push_back(activity_source);

	auto & source_task = tasks_[farm_spec->source_task->instance_name];
	activity_source->addRunnable(source_task->engine());
	source_task->setActivity(activity_source);
	source_task->port(farm_spec->source_port)->createConnectionManager(ConnectionManagerType::FARM);
	
	// -----------------------
	// Load gather task
	// -----------------------
	SchedulePolicy policy_gather;
	policy_gather.scheduling_policy = SchedulePolicy::TRIGGERED;

	if (!loadTask(farm_spec->gather_task, null_task_ptr))
		return;  // Component is disabled so nothing to be run

	std::shared_ptr<Activity> activity_gather = std::make_shared<ParallelActivity>(policy_gather);
	activities_.push_back(activity_gather);

	auto & gather_task = tasks_[farm_spec->gather_task->instance_name];
	activity_gather->addRunnable(gather_task->engine());
	gather_task->setActivity(activity_gather);
	if (!gather_task->port(farm_spec->gather_port)->isEvent())
		COCO_FATAL() << "Gather component " << farm_spec->gather_task->instance_name
					 << " input port: " << farm_spec->gather_port << " is not an event port";
	gather_task->port(farm_spec->gather_port)->createConnectionManager(ConnectionManagerType::FARM);

	// Load n pipelines
	startPipeline(farm_spec->pipelines[0]);
	for (unsigned int i = 1; i < farm_spec->num_workers; ++i)
	{
		std::unique_ptr<PipelineSpec> pipeline(new PipelineSpec());
		*pipeline = *(farm_spec->pipelines[0]);
		pipeline->tasks.clear();
		for (auto &task : farm_spec->pipelines[0]->tasks)
		{
			auto task_spec = app_spec_->cloneTaskSpec(task, "_" + std::to_string(i));
			pipeline->tasks.push_back(task_spec);
		}
		startPipeline(pipeline);
		app_spec_->addPipelineConnections(pipeline);
		farm_spec->pipelines.push_back(std::move(pipeline));
	}
	//farm_spec->pipelines = farm_pipelines;
	// Make connections
	// Simply change connection manager to the ports and add the connection to the the list
	ConnectionPolicySpec policy;
    policy.data = "DATA";
    policy.policy = "LOCKED";
    policy.transport = "LOCAL";
    policy.buffersize = "1";


    for (unsigned int i = 0; i < farm_spec->num_workers; ++i)
	{
	    std::unique_ptr<ConnectionSpec> connection_source(new ConnectionSpec());
	    connection_source->policy = policy;
	    connection_source->src_task = farm_spec->source_task;
	    connection_source->src_port = farm_spec->source_port;
	    connection_source->dest_task = farm_spec->pipelines[i]->tasks[0];
	    connection_source->dest_port = farm_spec->pipelines[i]->in_ports[0];
	    app_spec_->connections.push_back(std::move(connection_source));

	    std::unique_ptr<ConnectionSpec> connection_gather(new ConnectionSpec());
	    connection_gather->policy = policy;
	    connection_gather->src_task = farm_spec->pipelines[i]->tasks.back();
	    connection_gather->src_port = farm_spec->pipelines[i]->out_ports.back();
	    connection_gather->dest_task = farm_spec->gather_task;
	    connection_gather->dest_port = farm_spec->gather_port;
	    app_spec_->connections.push_back(std::move(connection_gather));
    }

}

bool GraphLoader::loadTask(std::shared_ptr<TaskSpec> & task_spec,
						   std::shared_ptr<TaskContext> & task_owner)
{
	// Issue: In this way, rightly, are disabled also all the peers of a given task.
	if (disabled_components_.count(task_spec->instance_name) != 0)
	{
		COCO_DEBUG("GraphLoader") << "Task " << task_spec->instance_name
								  << " disabled by launcher";
		return false;
	}

	COCO_DEBUG("GraphLoader") << "Loading "
							  << (task_spec->is_peer ? "peer" : "task") << ": "
							  << task_spec->instance_name << " (" << task_spec->name << ")";
	if (tasks_.find(task_spec->instance_name) != tasks_.end())
		COCO_FATAL() << "Trying to instantiate two task with the same name: "
					 << task_spec->instance_name;

	auto task = ComponentRegistry::create(task_spec->name,
			                              task_spec->instance_name);
	if (!task)
	{
		if(!task_spec->library_name.empty())
		{
			COCO_DEBUG("GraphLoader") << "Component " << task_spec->instance_name << " (" << task_spec->name << ")"
								      << " not found, trying to load library " << task_spec->library_name;

			if (!ComponentRegistry::addLibrary(task_spec->library_name))
			{
				COCO_FATAL() << "Failed to load library "
			                 << task_spec->library_name;
			}

			task = ComponentRegistry::create(task_spec->name,
					                         task_spec->instance_name);
		}

		if (!task)
		{
			COCO_FATAL() << "Failed to find component: " << task_spec->name << " in " << task_spec->library_name;
		}
	}

	task->setName(task_spec->name);
	task->setInstantiationName(task_spec->instance_name);
	if (!task_spec->is_peer)
	{
		task->setEngine(std::make_shared<ExecutionEngine>(task));
	}

	tasks_[task_spec->instance_name] = task;

	COCO_DEBUG("GraphLoader") << "Loading attributes";
	for (auto & attribute : task_spec->attributes)
	{
		if (task->attribute(attribute.name))
			task->attribute(attribute.name)->setValue(attribute.value);
		else
			COCO_ERR() << "Attribute: " << attribute.name << " doesn't exist";
	}

	// Parsing possible peers
	COCO_DEBUG("GraphLoader") << "Loading possible peers of "  << task_spec->instance_name ;
	for (auto & peer : task_spec->peers)
		loadTask(peer, task);

	// TBD: better do that at the very end of loading process
	COCO_DEBUG("GraphLoader") << "initing "  << task_spec->instance_name ;
	task->init();

	if (task_owner)
	{
		task_owner->addPeer(task);
		std::dynamic_pointer_cast<PeerTask>(task)->father_ = task_owner;
	}
	COCO_DEBUG("GraphLoader") << "done "  << task_spec->instance_name ;

	return true;
}

void GraphLoader::makeConnection(
		std::unique_ptr<ConnectionSpec> &connection_spec)
{
	ConnectionPolicy policy(connection_spec->policy.data,
                            connection_spec->policy.policy,
                            connection_spec->policy.transport,
                            connection_spec->policy.buffersize);

    // if not present means the task has been disabled!
    auto src_task = tasks_.find(connection_spec->src_task->instance_name);
    auto dest_task = tasks_.find(connection_spec->dest_task->instance_name);
    if (src_task == tasks_.end() || dest_task == tasks_.end())
	{
		COCO_ERR() << "Making connection: either src task " << connection_spec->src_task->instance_name
				   << " or dest task " << connection_spec->dest_task->instance_name << " doesn't exist";
		return;
	}

	if (src_task->second->isOnSameThread(dest_task->second))
		policy.lock_policy = ConnectionPolicy::UNSYNC;

    std::shared_ptr<PortBase> left = src_task->second->port(connection_spec->src_port);
    std::shared_ptr<PortBase>  right = dest_task->second->port(connection_spec->dest_port);

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
	COCO_DEBUG("Loader")<< "Starting the Activities!";
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
		COCO_ERR()
		<< "Only one sequential activity per application is allowed.\
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
				<< "color = blue;\n" << "label = \"activity "
				<< activity_count++ << ": "
				<< (activity->isPeriodic() ? "Periodic" : "Triggered");
		if (activity->isPeriodic())
			dot_file << " (" << activity->period() << " ms)";
		dot_file << " \";\n"; // TODO add schedule policy

		// Add the components
		for (auto& runnable : activity->runnables())
		{
			auto task = static_cast<ExecutionEngine *>(runnable.get())->task();
			dot_file << "subgraph cluster_" << subgraph_count++ << "{\n"
					<< "color = red;\n" << "label = \"Component: "
					<< task->name() << "\\nName: " << task->instantiationName()
					<< "\";\n";

			// Add port
            for (auto port : util::values_iteration(task->ports()))
			{
				createGraphPort(port, dot_file, graph_port_nodes, node_count);
			}

			// Add peer
			for (auto peer : task->peers())
			{
				createGraphPeer(peer, dot_file, graph_port_nodes,
						subgraph_count, node_count);
			}
			dot_file << "}\n";
		}
		dot_file << "}\n";
	}
	// Add connections
    for (auto task : util::values_iteration(tasks_))
	{
		createGraphConnection(task, dot_file, graph_port_nodes);
	}

	dot_file << "}" << std::endl;

	dot_file.close();

	std::string cmd = "dot " + dot_file_name + " -o " + filename
			+ std::string(".pdf") + " -Tpdf";
	if (std::system(cmd.c_str()) == -1)
	{
		COCO_ERR() << "error executing " << cmd;
	}
}

std::string GraphLoader::graphSvg() const
{
    char* temp = strdup("COCO_XXXXXX");
    int fd = mkstemp(temp);
    close(fd);
    if (!writeSvg(std::string(temp)))
        return "";
    std::string svgfile = std::string(temp) + ".svg";
    std::stringstream s;
    std::ifstream f(svgfile);
    char c;
    while (f.get(c))
    {
        s << c;
    }
    f.close();
    std::string graph_svg = s.str();
    remove(temp);
    remove((std::string(temp) + ".dot").c_str());
    remove(svgfile.c_str());
    free(temp);

    return graph_svg;
}

bool GraphLoader::writeSvg(const std::string& name) const
{
	std::string dot_file_name = name + ".dot";
	std::ofstream dot_file(dot_file_name.c_str());
	dot_file << "digraph {\n";
	dot_file << "graph[rankdir=LR, center=true, bgcolor=transparent];\n";
	dot_file << "subgraph cluster_border{\ncolor=white;\nbgcolor=white;\nshape=record;\n";
	int activity_count = 0;
	int subgraph_count = 0;
	int node_count = 0;
	std::unordered_map<std::string, int> graph_port_nodes;
	for (auto activity : activities_)
	{
		dot_file << "subgraph cluster_" << subgraph_count++ << "{\n"
				<< "color = blue;\n" << "label = \"activity "
				<< activity_count++ << ": "
				<< (activity->isPeriodic() ? "Periodic" : "Triggered");
		if (activity->isPeriodic())
			dot_file << " (" << activity->period() << " ms)";
		dot_file << " \";\n"; // TODO add schedule policy

		// Add the components
		for (auto& runnable : activity->runnables())
		{
			auto task = static_cast<ExecutionEngine *>(runnable.get())->task();
			dot_file << "subgraph cluster_" << subgraph_count++ << "{\n"
					<< "color = red;\nshape = record;\nstyle = rounded;\n"
					<< "label = \"" << task->instantiationName() << " ("
					<< task->name() << ")\"\n";
			// Add port
            for (auto port : util::values_iteration(task->ports()))
			{
				dot_file << node_count << "[color="
						<< (port->isOutput() ? "darkgreen" : "darkorchid4")
						<< ", shape=ellipse, label=\"" << port->name();
				if (port->isEvent())
					dot_file << "\\n(event)";
				dot_file << "\"];\n";
				std::string name_id = port->task()->instantiationName()
						+ port->name();
				graph_port_nodes[name_id] = node_count++;
			}
			// Add peer
			for (auto peer : task->peers())
			{
                if (peer->ports().size() > 0 || peer->peers().size() > 0)
				{
					dot_file << "subgraph cluster_" << subgraph_count++ << "{\n"
							<< "color = orange;\nshape=box;\nlabel = \"Component: "
							<< peer->name() << "\\nName: "
							<< peer->instantiationName() << "\";\n";
				}
				else
				{
					dot_file << node_count++
							<< "[color=orange, shape=box, label=\"Component: "
							<< peer->name() << "\\nName: "
							<< peer->instantiationName() << "\"];\n";
                    continue;
				}
				// Add port
                for (auto port : util::values_iteration(peer->ports()))
				{
                    //if (port->isConnected())
						createGraphPort(port, dot_file, graph_port_nodes,
								node_count);
				}

				for (auto sub_peer : peer->peers())
					createGraphPeer(sub_peer, dot_file, graph_port_nodes,
							subgraph_count, node_count);

				dot_file << "}\n";
			}
			dot_file << "}\n";
		}
		dot_file << "}\n";
	}
	dot_file << "}\n";

	// Add connections
    for (auto task : util::values_iteration(tasks_))
	{
		createGraphConnection(task, dot_file, graph_port_nodes);
	}
	dot_file << "}" << std::endl;
	dot_file.close();

	std::string cmd = "dot " + dot_file_name + " -o " + name + ".svg -Tsvg";
	return std::system(cmd.c_str()) >= 0;
}

void GraphLoader::createGraphPort(std::shared_ptr<PortBase> port,
		std::ofstream &dot_file,
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

void GraphLoader::createGraphPeer(std::shared_ptr<TaskContext> peer,
		std::ofstream &dot_file,
		std::unordered_map<std::string, int> &graph_port_nodes,
		int &subgraph_count, int &node_count) const
{
    std::cout << "PEER: " << peer->instantiationName() << std::endl;
    if (peer->ports().size() > 0 || peer->peers().size() > 0)
	{
		dot_file << "subgraph cluster_" << subgraph_count++ << "{\n"
				<< "color = red;\n" << "style = rounded;\n"
				<< "label = \"Component: " << peer->name() << "\\nName: "
				<< peer->instantiationName() << "\";\n";
	}
	else
	{
		dot_file << node_count++
				<< "[color=red, shape=box, style=rounded, label = \"Component: "
				<< peer->name() << "\\nName: " << peer->instantiationName()
				<< "\"];\n";
		return;
	}

	// Add port
    for (auto port : util::values_iteration(peer->ports()))
	{
        //if (port->isConnected())
			createGraphPort(port, dot_file, graph_port_nodes, node_count);
	}

	for (auto sub_peer : peer->peers())
		createGraphPeer(sub_peer, dot_file, graph_port_nodes, subgraph_count,
				node_count);

	dot_file << "}\n";
}

void GraphLoader::createGraphConnection(std::shared_ptr<TaskContext> &task,
		std::ofstream &dot_file,
		std::unordered_map<std::string, int> &graph_port_nodes) const
{
    for (auto port : util::values_iteration(task->ports()))
	{
		if (!port->isOutput())
			continue;
        std::string this_id = task->instantiationName() + port->name();

        int src = graph_port_nodes.at(this_id);

        auto connections = port->connectionManager()->connections();
		for (auto connection : connections)
		{
			std::string port_id =
					connection->input()->task()->instantiationName()
                            + connection->input()->name();

            int dst = graph_port_nodes.at(port_id);
            dot_file << src << " -> " << dst << ";\n";
		}
	}
	for (auto peer : task->peers())
		createGraphConnection(peer, dot_file, graph_port_nodes);
}

}
