#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "coco/coco.h"
#include "coco/coco_launcher.h"

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

char* getInputOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}
bool checkOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

void usage()
{
    std::cout << "Universal app to call any coco applications.\n";
    std::cout << "Usage: cocoar_launcher [options]\n";
    std::cout << "\t-x <configuration file>  (configuration file to launch an application)\n";
    std::cout << "\t-p (enable the print of the statistics of the components)\n";
    std::cout << "\t-l <library name> (library to be introspected to extract all its informaiton)\n";
    std::cout << "\t-h (print this message and exit)\n";
    std::cout << "\nThe options -x and -l are mutually exclusive and -x has higher priority\n";

}

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

    if (checkOptionExists(argv, argv + argc, "-h"))
    {
        usage();
        return 1;
    }
    char * config_file = getInputOption(argv, argv + argc, "-x");
    if (config_file)
    {
        bool print_statistics = checkOptionExists(argv, argv + argc, "-p");
        launchApp(config_file, print_statistics);
        return 0;
    }
    char * library_name = getInputOption(argv, argv + argc, "-l");
    if (library_name)
    {
        COCO_INIT_LOG("");
        coco::printXMLSkeleton(library_name, "", true, true);
        return 0;
    }
    usage();
    return 1;
}
/*
    bool print_statistic = false;
    switch (argc) {
        case 1:
            usage();
            return -1;
        case 2:
            if (strcmp(argv[1], "--help") == 0) {
                usage();
                return -1;
            }
            else {
                launchApp(argv[1], false);
            }
            break;
        case 3:
            std::cout << "argv[2]: " << argv[2] << std::endl;
            if (strcmp(argv[2], "1") == 0)
            {
                std::cout << "print statistic == true\n";
                print_statistic = true;
            }
            launchApp(argv[1], print_statistic);
        case 4:
            coco::printXMLSkeleton(argv[1], argv[2], argv[3]);
            break;
        case 5:
            coco::printXMLSkeleton(argv[1], argv[2], argv[3],true,false);
            break;
        default:
            usage();
            return -1;
    }   
    return 1;   
}
*/
