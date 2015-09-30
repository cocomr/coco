/**
 * Compact Framework Core
 * 2014-2015 Emanuele Ruffaldi and Filippo Brizzi @ Scuola Superiore Sant'Anna, Pisa, Italy
 */
#include <iostream>
#include <thread>
#include <chrono>

#include "coco/coco_core.hpp"

// dlopen cross platform
#ifdef WIN32
	#include <windows.h>
	typedef HANDLE dlhandle;
	#define DLLEXT ".dll"
	#define DLLPREFIX "lib"
	#define dlopen(x,y) LoadLibrary(x)
	#define dlsym(x,y) GetProcAddress(x,y)
	#define RTLD_NOW 0
	#define PATHSEP ';'
	#define DIRSEP '\\'
#else

	#include <dlfcn.h>
	typedef void* dlhandle;
	#define PATHSEP ':'
	#define DIRSEP '/'

	#ifdef __APPLE__
		#define DLLEXT ".dylib"
		#define DLLPREFIX "lib"
	#else
		#define DLLEXT ".so"
		#define DLLPREFIX "lib"
	#endif

#endif

/// as pointer to avoid issues of order in ctors
//static coco::ComponentRegistry *singleton;

namespace coco
{

AttributeBase::AttributeBase(TaskContext * p, const std::string &name)
	: name_(name) 
{
	p->addAttribute(this);
}

OperationBase::OperationBase(Service * p, const std::string &name) 
	: name_(name)
{
	p->addOperation(this);
}


PortBase::PortBase(TaskContext * p,const std::string &name, bool is_output, bool is_event)
	: task_(p), name_(p->name() + "_" + name), is_output_(is_output), is_event_(is_event)
{
	task_->addPort(this);
}

bool PortBase::addConnection(std::shared_ptr<ConnectionBase> connection) 
{
	manager_.addConnection(connection);
	return true;
}

void PortBase::triggerComponent()
{
	task_->triggerActivity();
}

std::string PortBase::taskName() const
{
    return task_->instantiationName();
}



bool Service::addAttribute(AttributeBase *a)
{
	if (attributes_[a->name()])
	{
		std::cerr << "An attribute with name: " << a->name() << " already exist\n";
		return false;
	}
	attributes_[a->name()] = a;
	return true;
}

AttributeBase *Service::getAttribute(std::string name) 
{
	auto it = attributes_.find(name);
	if(it == attributes_.end())
			return nullptr;
		else
			return it->second;
}

bool Service::addPort(PortBase *p) {
	if (ports_[p->name()]) {
		std::cerr << "A port with name: " << p->name() << " already exist\n";	
		return false;
	}
	else
	{
		ports_[p->name()] = p;
		return true;
	}
}

PortBase *Service::getPort(std::string name) 
{
	auto it = ports_.find(name);
	if(it == ports_.end())
			return nullptr;
		else
			return it->second;
}

bool Service::addOperation(OperationBase *o) {
	if (operations_[o->name()]) {
		std::cerr << "An operation with name: " << o->name() << " already exist\n";
		return false;
	}
	operations_[o->name()] = o;
	return true;
}

bool Service::addPeer(TaskContext *p) {
	peers_.push_back(p);
	return true;
}

Service * Service::provides(const std::string &x) {
	auto it = subservices_.find(x);
	if(it == subservices_.end())
	{
		Service * p = new Service(x);
		subservices_[x] = std::unique_ptr<Service>(p);
		return p;
	}
	else
		return it->second.get();
	return 0;
}

TaskContext::TaskContext() 
{
	activity_ = nullptr;
	state_ = STOPPED;
	//addOperation("stop", &TaskContext::stop, this);
}

void TaskContext::start()
{
	if (activity_ == nullptr)
	{
		COCO_FATAL() << "Activity not found! Set an activity to start a component!";
		return;
	}
	if (state_ == RUNNING)
	{
		COCO_ERR() << "Task already running";
		return;
	}
	state_ = RUNNING;
	activity_->start();
}

void TaskContext::stop()
{
	std::cout << "TASK STOP\n";
	if (activity_ == nullptr)
	{
		COCO_ERR() << "Activity not found! Nothing to be stopped";
		return;
	}
	else if (!activity_->isActive())
	{
		COCO_ERR() << "Activity not running! Nothing to be stopped";
		return;
	}
	state_ = STOPPED;
	activity_->stop();
}

void TaskContext::triggerActivity() 
{
	activity_->trigger();
}

}

