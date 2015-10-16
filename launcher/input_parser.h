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
				("profiling,p", "Enable the collection of statistics of the executions. Use only during debug as it slow down the performances.")
				("graph,g", "Create the graph of the varius components and of their connections.")
				("lib,l", boost::program_options::value<std::string>(), 
						"Print the xml template for all the components contained in the library.");

		//boost::program_options::options_description hidden("Hidden options");
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
	boost::program_options::options_description description_ = {"Universal app to call any coco applications.\
																\nUsage: coco_launcher [options]"};
	boost::program_options::variables_map vm_;
};