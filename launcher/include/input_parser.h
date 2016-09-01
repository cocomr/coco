/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#pragma once

#include <boost/program_options.hpp>

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
                ("disabled,d", boost::program_options::value<std::vector<std::string> >()->multitoken(),
                        "List of components that are going to be disabled in the execution")
                ("profiling,p", boost::program_options::value<int>()->implicit_value(5),
                    "Enable the collection of statistics of the executions. Use only during debug as it slow down the performances.")
                ("graph,g", boost::program_options::value<std::string>(),
                        "Create the graph of the varius components and of their connections.")
                ("xml_template,t", boost::program_options::value<std::string>(),
                        "Print the xml template for all the components contained in the library.")
                ("web_server,w", boost::program_options::value<int>()->implicit_value(7707),
                        "Instantiate a web server that allows to view statics about the executions.")
				("web_root,r", boost::program_options::value<std::string>(), "set document root for web server")
                ("latency,L", boost::program_options::value<std::vector<std::string> >()->multitoken(),
                    "Set the two task between which calculate the latency. Peer are not valid.");

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

    std::vector<std::string> getStringVector(std::string value)
    {
        if (vm_.count(value))
            return vm_[value].as<std::vector<std::string> >();
        return std::vector<std::string>();
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
