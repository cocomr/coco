#pragma once

namespace
{



struct ConnectionPolicyHandler
{
	std::string data;
	std::string policy;
	std::string transport;
	std::string buffersize;
};

struct ConnectionHandler
{
	TaskHandler * src_task;
	std::string src_port;
	TaskHandler * dest_task;
	std::string dest_port;
};

struct AttributeHandler
{
	std::string name;
	std::string value;
};

struct TaskHandler
{
	std::string name;
	std::string instantiation_name;
	std::string library_name;
	bool is_peer;

	std::vector<AttributeHandler> attributes;
	std::vector<TaskHandler *> peers;
};

struct SchedulePolicyHandler
{
	std::string policy;
	int period;
	int affinity;
};

struct ActivityHandler
{
	SchedulePolicyHandler policy;
	std::vector<TaskHandler *> tasks;
};

class AppDataHandler
{
	std::vector<TaskHandler> tasks;
	std::vector<ActivityHandler> activities;
	std::vector<ConnectionHandler> connections;

	std::vector<std::string> libraries_paths;
};



}