#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <ros/ros.h>
#include <ros/console.h>

#include "loader.h"
#include "input_parser.h"
#include "xml_creator.h"

void launchApp(std::string confing_file_path, bool print_statistic)
{
    coco::CocoLauncher launcher(confing_file_path.c_str());
    launcher.createApp();    
    launcher.startApp();
    
    ros::Rate loop_rate(5);
    while (ros::ok())
    {
        if (print_statistic)
        {
            coco::util::ProfilerManager::getInstance()->printStatistics();
        }
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
        bool print_stat = options.get("profiling");
        launchApp(config_file, print_stat);
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