#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace coco
{

struct AttributeSpec
{
    AttributeSpec(const std::string &_name = "",
                  const std::string &_value = "")
    {}
    std::string name;
	std::string value;
};

struct TaskSpec
{
	std::string name;
	std::string instance_name;
	std::string library_name; // Here already full with prefix and suffix, ready to be dlopen
	bool is_peer;

	std::vector<AttributeSpec> attributes;
	std::vector<std::shared_ptr<TaskSpec> > peers;
};

struct ConnectionPolicySpec
{
	std::string data;
	std::string policy;
	std::string transport;
	std::string buffersize;
};

struct ConnectionSpec
{
	std::shared_ptr<TaskSpec> src_task;
	std::string src_port;
	std::shared_ptr<TaskSpec> dest_task;
	std::string dest_port;

	ConnectionPolicySpec policy;
};

struct SchedulePolicySpec
{
	std::string type;
	std::string realtime;
	int period;
	int affinity;
	int priority;
	int runtime;
	bool exclusive;
};

struct ActivityBase
{
	virtual ~ActivityBase()
	{}
};

struct ActivitySpec : public ActivityBase
{
	SchedulePolicySpec policy;
	bool is_parallel;
	std::vector<std::shared_ptr<TaskSpec> > tasks;
};

struct PipelineSpec : public ActivityBase
{
	std::vector<std::shared_ptr<TaskSpec> > tasks;
    std::vector<std::string> out_ports;
    std::vector<std::string> in_ports;
    bool parallel;
};

struct FarmSpec : public ActivityBase
{
    //std::unique_ptr<PipelineSpec> pipeline;
    std::vector<std::unique_ptr<PipelineSpec> > pipelines;  // One pipeline for each worker
    std::shared_ptr<TaskSpec> source_task;
    SchedulePolicySpec source_task_schedule;
    std::string source_port;
    std::shared_ptr<TaskSpec> gather_task;
    std::string gather_port;
    unsigned int num_workers;
};

struct ExportedAttributeSpec
{
	std::string task_instance_name;
	std::string name;
};

struct ExportedPortSpec
{
	std::string task_instance_name;
	std::string name;
	bool is_output;
};

struct TaskGraphSpec
{
	std::string name;
	std::unordered_map<std::string, std::shared_ptr<TaskSpec> > tasks; // Maybe unordered_map better

    std::vector<std::unique_ptr<ActivitySpec> > activities;
    std::vector<std::unique_ptr<PipelineSpec> > pipelines;
    std::vector<std::unique_ptr<FarmSpec> > farms;

    std::vector<std::unique_ptr<ConnectionSpec> > connections;

	std::vector<std::string> resources_paths;

	// TODO: add exported attribute and external ports
	std::vector<std::shared_ptr<ExportedAttributeSpec> > exported_attributes;
	std::vector<std::shared_ptr<ExportedPortSpec> > exported_ports;
	std::vector<std::shared_ptr<TaskGraphSpec> > sub_task_system;

	inline std::shared_ptr<TaskSpec> cloneTaskSpec(std::shared_ptr<TaskSpec> task, const std::string &name_suffix = "")
	{
		std::shared_ptr<TaskSpec> task_spec(new TaskSpec());
		*task_spec = *task;
		task_spec->instance_name = task->instance_name + name_suffix;
		// Peers:
		task_spec->peers.clear();
		for (auto peer : task->peers)
		{
			task_spec->peers.push_back(cloneTaskSpec(peer, name_suffix));
		}
		return task_spec;
	}
	inline void addPipelineConnections(std::unique_ptr<PipelineSpec> &pipe_spec)
	{
	    ConnectionPolicySpec policy;
	    policy.data = "DATA";
	    if (pipe_spec->parallel)
	        policy.policy = "LOCKED";
	    else
	        policy.policy = "UNSYNC";
	    policy.transport = "LOCAL";
	    policy.buffersize = "1";

	    for (unsigned i = 0; i < pipe_spec->out_ports.size() - 1; ++i)
	    {
	        std::unique_ptr<ConnectionSpec> connection(new ConnectionSpec());
	        connection->policy = policy;
	        connection->src_task = pipe_spec->tasks[i];
	        connection->src_port = pipe_spec->out_ports[i];
	        connection->dest_task = pipe_spec->tasks[i + 1];
	        connection->dest_port = pipe_spec->in_ports[i + 1];
	        connections.push_back(std::move(connection));
	    }
	}

};

}
