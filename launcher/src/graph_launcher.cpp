/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#endif

#include "xml_parser.h"
#include "library_parser.h"
#include "graph_loader.h"
#include "input_parser.h"

#include "coco/util/timing.h"
#include "coco/util/accesses.hpp"
#include "coco/web_server/web_server.h"
#include "coco/register.h"

std::shared_ptr<coco::GraphLoader> loader;

std::atomic<bool> stop_execution =
{ false };
std::mutex launcher_mutex, statistics_mutex;
std::condition_variable launcher_condition_variable,
		statistics_condition_variable;

void handler(int sig)
{
	void *array[10];
	size_t size;

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
#ifndef WIN32
	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	backtrace_symbols_fd(array, size, STDERR_FILENO);
#endif
	exit(1);
}

void terminate(int sig)
{
	if (loader)
		loader->terminateApp();

	stop_execution = true;
	launcher_condition_variable.notify_all();
	statistics_condition_variable.notify_all();
}

void printStatistics(int interval)
{
	while (!stop_execution)
	{
		std::cout << "Printing statistic for tasks:" << std::endl;
		for (auto &task : coco::ComponentRegistry::tasks())
		{
			if (coco::isPeer(task.second))
            	continue;
			std::cout << "Task: " << task.first << std::endl;
			std::cout << task.second->timeStatistics().toString() << std::endl;
		}

        std::unique_lock<std::mutex> mlock(statistics_mutex);
		statistics_condition_variable.wait_for(mlock, std::chrono::seconds(interval));
	}
}

void launchApp(const std::string & config_file_path, bool profiling,
		const std::string &graph, int web_server_port,
		const std::string& web_server_root,
		std::unordered_set<std::string> disabled_component,
	    std::vector<std::string> latency)
{
	std::shared_ptr<coco::TaskGraphSpec> graph_spec(new coco::TaskGraphSpec());
	coco::XmlParser parser;
	if (!parser.parseFile(config_file_path, graph_spec))
		exit(0);

	loader = std::make_shared<coco::GraphLoader>();
	loader->loadGraph(graph_spec, disabled_component);

	loader->enableProfiling(profiling);

	if (latency.size() != 0)
	{
		auto src_task = COCO_TASK(latency[0]);
		auto dst_task = COCO_TASK(latency[1]);

		if ((!src_task || !dst_task) || (coco::isPeer(src_task) || coco::isPeer(dst_task)))
			COCO_FATAL() << "To use latency specify the name of two valid task.";

		src_task->setTaskLatencySource();
		dst_task->setTaskLatencyTarget();
	}

	if (!graph.empty())
		loader->printGraph(graph);

	loader->startApp();
    COCO_DEBUG("GraphLauncher") << "Application is running!";

	if (web_server_port > 0)
	{
		std::string graph_svg = loader->graphSvg();
		if (graph_svg.empty())
		{
            COCO_ERR() << "Failed to create svg graph from execution setup";
		}
		if (!coco::WebServer::start(web_server_port, graph_spec->name,
				graph_svg, web_server_root))
		{
			COCO_FATAL()<< "Failed to initialize server on port: " << web_server_port << std::endl;
		}
	}

	std::unique_lock<std::mutex> mlock(launcher_mutex);
	launcher_condition_variable.wait(mlock);
}


int main(int argc, char **argv)
{
#ifndef WIN32
	signal(SIGSEGV, handler);
	signal(SIGBUS, handler);
	signal(SIGINT, terminate);
#endif
	InputParser options(argc, argv);

	std::string config_file = options.getString("config_file");
	if (!config_file.empty())
	{
		int port = -1;
		if (options.get("web_server"))
			port = options.getInt("web_server");
		std::string root = "";
		if (options.get("web_root"))
			root = options.getString("web_root");

		std::thread statistics;
		bool profiling = options.get("profiling");
		if (profiling && port == -1)
		{
			int interval = options.getInt("profiling");
			statistics = std::thread(printStatistics, interval);
		}

		std::string graph = options.getString("graph");

		std::vector<std::string> disabled = options.getStringVector("disabled");
		std::unordered_set<std::string> disabled_component;
		for (auto & d : disabled)
			disabled_component.insert(d);

		std::vector<std::string> latency = options.getStringVector("latency");
		if (latency.size() > 0 && latency.size() != 2)
			COCO_FATAL() << "To calculate latency specify the name of two task. [-l task1 task2]";

		launchApp(config_file, profiling, graph, port, root,
				disabled_component, latency);

		if (statistics.joinable())
		{
			statistics.join();
		}
		return 0;
	}

    std::string library_name = options.getString("xml_template");
	if (!library_name.empty())
	{
		COCO_INIT_LOG();
		coco::LibraryParser library_parser;
		auto graph_spec = std::make_shared<coco::TaskGraphSpec>();
		library_parser.loadLibrary(library_name, graph_spec);
		coco::XmlParser xml_parser;

		std::string xml_file = coco::util::string_splitter(library_name, '/').last();
		std::cout << "xml_file: " << xml_file << std::endl;
		if (!xml_parser.createXML(xml_file + ".xml", graph_spec))
			COCO_FATAL() << "Failed to create xml file from library: " << library_name << std::endl;
		
		return 0;
	}

	if (options.get("help"))
	{
		options.print();
		return 0;
	}

	options.print();
	return 1;
}
