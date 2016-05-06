
#include "graph_loader.h"


namespace coco
{

void GraphLoader::loadGraph(std::shared_ptr<TaskGraphSpec> app_spec);
{
	app_spec_ = app_spec;
	/* Load components */
	COCO_DEBUG("GraphLoader") << "Loading components";
	for (auto & task : app_spec_->tasks)
		loadTask(task);

	/* Make connections */
	COCO_DEBUG("GraphLoader") << "Making conenctions";
	for (auto & connection : app_spec_->connections)
		makeConnection(connection);
	

	/* Launch activities */
	COCO_DEBUG("GraphLoader") << "Starting Activities";
	for (auto & activity : app_spec_->activities)
		startActivity(activity);
}


void GraphLoader::loadTask(std::shared_ptr<TaskSpec> &task_spec)
{
	TaskContext * task = ComponentRegistry::create(task_spec->name, task_spec->istance_name);
	if (!task)
	{
		COCO_DEBUG("GraphLoader") << "Component " << task_spec->istance_name <<
                       " not found, trying to load from library";

		if (!ComponentRegistry::addLibrary(task_spec->library_name)
			COCO_FATAL() << "Failed to load "
	}

}

void GraphLoader::makeConnection(std::shared_ptr<ConnectionSpec> &connection_spec)
{

}

void GraphLoader::startActivity(std::shared_ptr<ActivitySpec> &activity_spec)
{

}

void GraphLoader::startApp()
{

}

void GraphLoader::waitToComplete()
{

}

void GraphLoader::terminateApp()
{

}





};