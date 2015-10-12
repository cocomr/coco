#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <ros/ros.h>
#include <ros/console.h>

#include "util/timing.h"
#include "loader.h"
#include "input_parser.h"
#include "xml_creator.h"

std::atomic<bool> stop_statistics_thread = {false};
void printStatistics()
{
    while(true)
    {
        if (stop_statistics_thread)
            break;
        sleep(5);
        
        COCO_PRINT_ALL_TIME
    }
}

void launchApp(std::string confing_file_path)
{
    coco::CocoLauncher launcher(confing_file_path.c_str());
    launcher.createApp();    
    launcher.startApp();
    
    ros::Rate loop_rate(1000);
    while (ros::ok())
    {
        ros::spinOnce();
        loop_rate.sleep();
    }
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "ros_coco_launcher");
    ros::NodeHandle n;

    InputParser options(argc, argv);

    std::string config_file = options.getString("config_file");
    if (!config_file.empty())
    {
        std::thread statistics;
        if (options.get("profiling"))
            statistics = std::thread(printStatistics);
        launchApp(config_file);
        stop_statistics_thread = true;
        statistics.join();
        return 0;
    }
    std::string library_name = options.getString("lib");
    if (!library_name.empty())
    {
        COCO_INIT_LOG("");
        coco::printXMLSkeletonLibrary(library_name, "", true, true);
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