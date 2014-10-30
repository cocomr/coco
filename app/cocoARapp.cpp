#include "ezoro.hpp"

void usage() {
    std::cout << "Universal app to call any coco applications.\n";
    std::cout << "To start an application just pass the xml configuration file.\n";
    std::cout << "To print the xml skeleton of a component pass:\n";
    std::cout << "\t task name + task library name (without prefix and suffix) + task library path\n";
}

void launchApp(std::string confing_file_path) {
    coco::CocoLauncher launcher(confing_file_path.c_str());
    launcher.createApp();
    launcher.startApp();
    while(true) {
        sleep(5);
        std::cout << "Statistics\n";
        coco::LoggerManager::getInstance()->printStatistics();
    }
}


int main(int argc, char **argv) {
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
                launchApp(argv[1]);
            }
            break;
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