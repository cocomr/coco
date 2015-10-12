#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "loader.h"
#include "input_parser.h"
#include "xml_creator.h"
#include "util/timing.h"

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

void launchApp(std::string confing_file_path, bool print_statistic)
{
    coco::CocoLauncher launcher(confing_file_path.c_str());
    launcher.createApp();    
    launcher.startApp();
    while(true)
    {
        sleep(10000000);
        // if (print_statistic)
        // {
        //     coco::util::ProfilerManager::getInstance()->printStatistics();
        //     COCO_PRINT_ALL_TIME
        // }
    }
}

int main(int argc, char **argv)
{
    signal(SIGSEGV, handler);
    signal(SIGBUS, handler);

    InputParser options(argc, argv);

    std::string config_file = options.getString("config_file");
    if (!config_file.empty())
    {
        bool print_stat = options.get("profiling");
        std::thread statistics(printStatistics);
        launchApp(config_file, print_stat);
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
