#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "loader.h"
#include "input_parser.h"

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

// void usage()
// {
//     std::cout << "Universal app to call any coco applications.\n";
//     std::cout << "Usage: cocoar_launcher [options]\n";
//     std::cout << "\t-x <configuration file>  (configuration file to launch an application)\n";
//     std::cout << "\t-p (enable the print of the statistics of the components)\n";
//     std::cout << "\t-l <library name> (library to be introspected to extract all its informaiton)\n";
//     std::cout << "\t-h (print this message and exit)\n";
//     std::cout << "\nThe options -x and -l are mutually exclusive and -x has higher priority\n";

// }

void launchApp(std::string confing_file_path, bool print_statistic)
{
    coco::CocoLauncher launcher(confing_file_path.c_str());
    launcher.createApp();    
    launcher.startApp();
    while(true)
    {
        sleep(10);
        if (print_statistic)
        {
            coco::util::ProfilerManager::getInstance()->printStatistics();
        }
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
        launchApp(config_file, print_stat);
        return 0;
    }
    std::string library_name = options.getString("lib");
    if (!library_name.empty())
    {
        COCO_INIT_LOG("");
        coco::printXMLSkeleton(library_name, "", true, true);
        return 0;
    }

    if (options.get("help"))
    {
        options.print();
        return 0;
    }
    //usage();
    return 1;
}
