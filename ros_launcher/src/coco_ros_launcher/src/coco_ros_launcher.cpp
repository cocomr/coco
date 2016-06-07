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

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <ros/ros.h>
#include <ros/console.h>

#include "xml_parser.h"
#include "graph_loader.h"
#include "xml_creator.h"
#include "util/timing.h"
#include "input_parser.h"
#include "web_server.h"

#include "legacy/loader.h"

std::shared_ptr<coco::GraphLoader> loader;
//Legacy
coco::CocoLauncher *launcher = {nullptr};

std::atomic<bool> stop_execution = {false};
std::mutex launcher_mutex, statistics_mutex;
std::condition_variable statistics_condition_variable;

void handler(int sig)
{
  void *array[10];
  size_t size;

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

void terminate(int sig)
{
    if (loader)
        loader->terminateApp();
    // Legacy
    if (launcher)
        launcher->killApp();

    stop_execution = true;
 
    statistics_condition_variable.notify_all();
    ros::shutdown();
}

void printStatistics(int interval)
{
    while(!stop_execution)
    {
        COCO_PRINT_ALL_TIME

        std::unique_lock<std::mutex> mlock(statistics_mutex);
        statistics_condition_variable.wait_for(mlock, std::chrono::seconds(interval));
    }
}


void launchApp(const std::string & config_file_path, bool profiling,
        const std::string &graph, int web_server_port,
        const std::string& web_server_root,
        std::unordered_set<std::string> disabled_component)
{
    std::shared_ptr<coco::TaskGraphSpec> graph_spec(new coco::TaskGraphSpec());
    coco::XmlParser parser;
    if (!parser.parseFile(config_file_path, graph_spec))
        exit(0);

    loader = std::make_shared<coco::GraphLoader>();
    loader->loadGraph(graph_spec, disabled_component);

    loader->enableProfiling(profiling);

    if (!graph.empty())
        loader->printGraph(graph);

    loader->startApp();
    COCO_LOG(0)<< "Application is running!";

    if (web_server_port > 0)
    {
        std::string graph_svg = loader->graphSvg();
        if (graph_svg.empty())
            COCO_FATAL() << "Failed to create svg graph from execution setup";
        if (!coco::WebServer::start(web_server_port, graph_svg, web_server_root))
        {
            COCO_FATAL()<< "Failed to initialize server on port: " << web_server_port << std::endl;
        }
    }
}

// Legacy
void launchAppLegacy(std::string confing_file_path, bool profiling, const std::string &graph)
{
    launcher = new coco::CocoLauncher(confing_file_path.c_str());
    launcher->createApp(profiling);
    if (!graph.empty())
        launcher->createGraph(graph);

    launcher->startApp();

    COCO_LOG(0) << "Application is running!";

    //std::unique_lock<std::mutex> mlock(launcher_mutex);
    // launcher_condition_variable.wait(mlock);
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "ros_coco_launcher");
    ros::NodeHandle n;

    signal(SIGSEGV, handler);
    signal(SIGBUS, handler);
    signal(SIGINT, terminate);

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
        
        launchApp(config_file, profiling, graph, port, root, disabled_component);

        ros::Rate rate(100);
        while (ros::ok())
        {
            ros::spinOnce();
            rate.sleep();
        }

        if (statistics.joinable())
        {
            statistics.join();
        }
        return 0;
    }
    // Legacy
    std::string legacy_config_file = options.getString("legacy_config_file");
    if (!legacy_config_file.empty())
    {
        std::thread statistics;
        
        bool profiling = options.get("profiling");
        if (profiling)
        {
            int interval = options.getInt("profiling");
            statistics = std::thread(printStatistics, interval);
        }
        
        std::string graph = options.getString("graph");

        
        launchAppLegacy(legacy_config_file, profiling, graph);

        ros::Rate rate(100);
        while (ros::ok())
        {
            ros::spinOnce();
            rate.sleep();
        }

        if (statistics.joinable())
        {
            statistics.join();
        }
        return 0;
    }

    std::string library_name = options.getString("lib");
    if (!library_name.empty())
    {
        COCO_INIT_LOG();
        coco::XMLCreator::printXMLSkeletonLibrary(library_name, "", true, true);
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