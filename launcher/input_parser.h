#pragma once

#include <boost/program_options.hpp>
//#include <boost/any.hpp>

class InputParser
{
public:
    InputParser(int argc, char ** argv)
        : argc_(argc), argv_(argv)
    {
        
        description_.add_options()
                ("help,h", "Print help message and exit.")
                ("config_file,x", boost::program_options::value<std::string>(),
                        "Xml file with the configurations of the application.")
                ("legacy_config_file,z", boost::program_options::value<std::string>(),
                        "Xml file with the legacy configurations of the application.")
                ("profiling,p", boost::program_options::value<int>()->implicit_value(5),
                    "Enable the collection of statistics of the executions. Use only during debug as it slow down the performances.")
                ("graph,g", boost::program_options::value<std::string>(),
                        "Create the graph of the varius components and of their connections.")
                ("lib,l", boost::program_options::value<std::string>(), 
                        "Print the xml template for all the components contained in the library.");

        boost::program_options::store(boost::program_options::command_line_parser(argc_, argv_).
                options(description_).run(), vm_);
        boost::program_options::notify(vm_);
    }

    bool get(std::string value)
    {
        if(vm_.count(value))
            return true;
        return false;
    }

    std::string getString(std::string value)
    {
        if(vm_.count(value))
            return vm_[value].as<std::string>();
        return "";
    }

    float getFloat(std::string value)
    {
        if(vm_.count(value))
            return vm_[value].as<float>();
        return std::numeric_limits<double>::quiet_NaN();
    }

    int getInt(std::string value)
    {
        if (vm_.count(value))
            return vm_[value].as<int>();
        return std::numeric_limits<int>::quiet_NaN();
    }
    void print()
    {
        std::cout << description_ << std::endl;
    }

    void print(std::ostream & os)
    {
        description_.print(os);
    }

private:
    int argc_;
    char ** argv_;
    boost::program_options::options_description description_ = {"Universal app to call any coco applications.\n"
                                                                "Usage: coco_launcher [options]\n"
                                                                "Use COCO_PREFIX_PATH to specify lookup paths for resources and libraries\n"
                                                            };
    boost::program_options::variables_map vm_;
};