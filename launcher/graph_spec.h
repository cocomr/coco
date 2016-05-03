#pragma once

namespace coco
{

namespace graph
{


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
};

struct AttributeSpec
{
	std::string name;
	std::string type;
};

struct TaskSpec
{
	std::string name;
	std::string instance_name;
	std::string library_name; // Here already full with prefix and suffix, ready to be dlopen
	bool is_peer;

	std::vector<AttributeSpec> attributes;
	std::vector<std::shared<TaskSpec> > peers;
};

struct SchedulePolicySpec
{
	std::string policy;
	int period;
	int affinity;
};

struct ActivitySpec
{
	SchedulePolicySpec policy;
	std::vector<std::shared_ptr<TaskSpec> > tasks;
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

class TaskGraphSpec
{
	std::vector<std::shared_ptr<TaskSpec> > tasks; // Maybe unordered_map better
	std::vector<std::shared_ptr<ActivitySpec> > activities;
	std::vector<std::shared_ptr<ConnectionSpec> > connections;

	// std::vector<std::string> libraries_paths; // This may not be necessary
	std::vector<std::string> resources_paths;

	// TODO: add exported attribute and external ports
	std::vector<std::shared_ptr<ExportedAttributeSpec> > exported_attributes;
	std::vector<std::shared_ptr<ExportedPortSpec> > exported_ports;

	std::vector<std::shared_ptr<TaskGraphSpec> > sub_task_system;
};

}

}