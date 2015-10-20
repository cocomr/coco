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

#include "loader.h"
#include "input_parser.h"
#include "xml_creator.h"
#include "util/timing.h"

coco::CocoLauncher *launcher = {nullptr};
std::atomic<bool> stop_execution = {false};

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
    if (launcher)
        launcher->killApp();
    stop_execution = true;
}

void printStatistics(int interval)
{
    while(!stop_execution)
    {
        sleep(interval);
        std::cout << "PRINTING PROFILING\n\n";
        COCO_PRINT_ALL_TIME
    }
}

void launchApp(std::string confing_file_path, bool profiling)
{
    launcher = new coco::CocoLauncher(confing_file_path.c_str());
    launcher->createApp(profiling);
    launcher->startApp();
    // TODO add condition variable
    while(!stop_execution)
    {
        sleep(5);
    }
}

int main(int argc, char **argv)
{
    signal(SIGSEGV, handler);
    signal(SIGBUS, handler);
    signal(SIGINT, terminate);

    InputParser options(argc, argv);

    std::string config_file = options.getString("config_file");
    if (!config_file.empty())
    {
        std::thread statistics;
        bool profiling = options.get("profiling");
        if (profiling)
        {
            int interval = options.getInt("profiling");
            std::cout << "INTERVAL: " << interval << std::endl;
            statistics = std::thread(printStatistics, interval);
        }
        launchApp(config_file, profiling);
        if (statistics.joinable())
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
