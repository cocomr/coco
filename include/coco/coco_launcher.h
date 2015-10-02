/**
 * Compact Framework Core
 * 2014-2015 Emanuele Ruffaldi and Filippo Brizzi @ Scuola Superiore Sant'Anna, Pisa, Italy
 */
#pragma once
#include <unordered_map>
#include "coco_core.h"
#include "coco_register.h"
#include "tinyxml2/tinyxml2.h"
#include <exception>

namespace coco
{
    struct LComponentBase
    {

        virtual PortBase * getInPort(const std::string & x) = 0;
        virtual PortBase * getOutPort(const std::string & x) = 0;
        virtual std::string name() const = 0;
        PortBase * getPort(const std::string & x)
        {
            PortBase * p = getInPort(x);
            if(p) return p;
            return getOutPort(x);
        }
    };

    struct LRealComponent: public LComponentBase
    {
        LRealComponent(TaskContext * t): real(t) {}

        TaskContext * real;

        virtual std::string name() const { return real->name(); }
        virtual PortBase * getInPort(const std::string & x) { return real->getPort(x); }
        virtual PortBase * getOutPort(const std::string & x) { return real->getPort(x); }
    };

    struct LVirtualComponent: public LComponentBase
    {
    public:
        std::string name_;
        std::map<std::string,PortBase*> inports;
        std::map<std::string,PortBase*> outports;

        virtual std::string name() const { return name_; }
        PortBase * getInPort(const std::string & x) { return inports[x]; }

        PortBase * getOutPort(const std::string & x) { return outports[x]; }

    };


/**
 * Launcher class that takes a XML file and creates the network
 */
class CocoLauncher
{
public:
    CocoLauncher(const std::string &config_file);

    bool createApp();
    void startApp();
    // Used in the editor
private:
    void parseLogConfig(tinyxml2::XMLElement *logconfig);
    void parsePaths(tinyxml2::XMLElement *paths);
    void parseComponent(tinyxml2::XMLElement *component, bool is_peer = false);
    void parseSchedule(tinyxml2::XMLElement *schedule_policy, TaskContext *t);
    void parseAttribute(tinyxml2::XMLElement *attributes, TaskContext *t);
    std::string checkResource(const std::string &value);
    void parsePeers(tinyxml2::XMLElement *peers, TaskContext *t);
    void parseConnection(tinyxml2::XMLElement *connection);

    bool parseFile(tinyxml2::XMLDocument & doc, bool top);

    const std::string &config_file_;
    tinyxml2::XMLDocument doc_;

    //std::map<std::string, std::shared_ptr<LComponentBase> > tasks_;
    std::map<std::string, TaskContext*> tasks_;
    //std::map<std::string, TaskContext*> real_tasks_;
    std::list<std::string> peers_;

    std::vector<std::string> resources_paths_;
    std::string libraries_path_;
};

class CocoLoader
{
public:
    std::unordered_map<std::string, TaskContext *> addLibrary(std::string library_file_name);
    void clearTasks() { tasks_.clear(); }
    std::unordered_map<std::string, TaskContext *> tasks() { return tasks_; }
    TaskContext* task(std::string name)
    {
        if (tasks_.find(name) != tasks_.end())
            return tasks_[name];
        return nullptr;
    }
private:
    std::unordered_map<std::string, TaskContext *> tasks_;
};

void printXMLSkeleton(std::string task_library, std::string task_library_path,
                      bool adddoc = false, bool savefile=true);
    
} // end of namespace coco

