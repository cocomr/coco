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

#include "xml_creator.h"
#include "coco/util/accesses.hpp"

namespace coco
{

tinyxml2::XMLElement* xmlnodetxt(tinyxml2::XMLDocument *xml_doc,
								 tinyxml2::XMLElement * pa,
								 const std::string &tag,
								 const std::string text)
{
	using namespace tinyxml2;
	XMLElement *xml_task = xml_doc->NewElement(tag.c_str());
	XMLText *task_text = xml_doc->NewText(text.c_str());
	xml_task->InsertEndChild(task_text);
	pa->InsertEndChild(xml_task);
	return xml_task;
}

bool XMLCreator::printXMLSkeletonTask(std::string task_name, std::string task_library, 
						 std::string task_library_path, bool adddoc, bool savefile)
{
	using namespace tinyxml2;
    
    std::shared_ptr<TaskContext> task = ComponentRegistry::create(task_name, task_name);
	if(!task)
    {
        std::cout << "Cannot create " << task_name << " " << task_library << std::endl;
		return false; 
    }

	tinyxml2::XMLDocument *xml_doc = new tinyxml2::XMLDocument();
	XMLElement *xml_package = xml_doc->NewElement("package");
	xml_doc->InsertEndChild(xml_package);
	{
		scopedxml xml_components(xml_doc,xml_package,"components");
		scopedxml xml_component(xml_doc,xml_components,"component");
		
//		XMLElement *xml_task = xmlnodetxt(xml_doc,xml_component,"task",task_name);
//		if(adddoc && !task->doc().empty())
//			XMLElement *xml_doca = xmlnodetxt(xml_doc,xml_component,"info",task->doc());

//		XMLElement *xml_lib = xmlnodetxt(xml_doc,xml_component,"library",task_library);
//		XMLElement *xml_libpath = xmlnodetxt(xml_doc,xml_component,"librarypath",task_library_path);
		{
			scopedxml xml_attributes(xml_doc,xml_component,"attributes");
			for (auto itr : task->attributes()) 
			{
				scopedxml xml_attribute(xml_doc,xml_attributes,"attribute");
				xml_attribute->SetAttribute("name", itr.second->name().c_str());
				xml_attribute->SetAttribute("value", "");
				if(adddoc)
                    xml_attribute->SetAttribute("type",itr.second->asSig().name());
				if(adddoc && !itr.second->doc().empty())
				{
//					XMLElement *xml_doca = xmlnodetxt(xml_doc,xml_attribute,"doc",itr.second->doc());
				}
			}
		}
		if(adddoc)
		{
			{
				scopedxml xml_ports(xml_doc,xml_component,"ports");
				for(auto itr : task->ports()) 
				{
					scopedxml xml_port(xml_doc,xml_ports,"ports");
					xml_port->SetAttribute("name", itr.second->name().c_str());
					xml_port->SetAttribute("type", itr.second->typeInfo().name());
					if(adddoc && !itr.second->doc().empty())
					{
//						XMLElement *xml_doca = xmlnodetxt(xml_doc,xml_port,"doc",itr.second->doc());
					}
				}
			}

			{
				scopedxml xml_ops(xml_doc,xml_component,"operations");
				for(auto itr: task->operations())
				{
					scopedxml xml_op(xml_doc,xml_ops,"operation");
					xml_op->SetAttribute("name", itr.second->name().c_str());
					xml_op->SetAttribute("type", itr.second->asSig().name());
					if(adddoc && !itr.second->doc().empty())
					{
//						XMLElement *xml_doca = xmlnodetxt(xml_doc,xml_op,"doc",itr.second->doc());
					}
				}
			}
		}
	} // components

	if(!adddoc)
	{

		// Adding connections
        std::cout << "Inserting connections\n";
        //COCO_LOG(1) << "Inserting connections";

		scopedxml xml_connections(xml_doc,xml_package,"connections");

		for(auto itr : task->ports()) 
		{
			scopedxml xml_connection(xml_doc,xml_connections,"connection");
			xml_connection->SetAttribute("data", "");
			xml_connection->SetAttribute("policy", "");
			xml_connection->SetAttribute("transport", "");
			xml_connection->SetAttribute("buffersize", "");
			scopedxml xml_src(xml_doc,xml_connection,"src");
			scopedxml xml_dest(xml_doc,xml_connection,"dest");

			if (itr.second->isOutput()) {
				xml_src->SetAttribute("task", task_name.c_str());
				xml_src->SetAttribute("port", itr.second->name().c_str());
				xml_dest->SetAttribute("task", "");
				xml_dest->SetAttribute("port", "");
			} else {
				xml_src->SetAttribute("task", "");
				xml_src->SetAttribute("port", "");
				xml_dest->SetAttribute("task", task_name.c_str());
				xml_dest->SetAttribute("port", itr.second->name().c_str());
			}
		}
	}


	if(savefile)
	{
		std::string out_xml_file = task_name + std::string(".xml");
		xml_doc->SaveFile(out_xml_file.c_str());	
	}
	else
	{
		XMLPrinter printer;
		xml_doc->Print(&printer);
        std::cout << printer.CStr();
        //COCO_LOG(1) << printer.CStr();
	}
	delete xml_doc;
    return true;
}

void XMLCreator::printXMLSkeletonLibrary(std::string com_library, std::string com_library_path,
				      		 bool adddoc, bool savefile)
{
    ComponentRegistry::addLibrary(com_library.c_str(), com_library_path.c_str());

    for (auto com_name : util::keys_iteration(ComponentRegistry::components()))
    {
        COCO_LOG(1) << "Creating xml template for component: " << com_name;
        printXMLSkeletonTask(com_name, com_library, com_library_path, adddoc, savefile);
    }
}

}
