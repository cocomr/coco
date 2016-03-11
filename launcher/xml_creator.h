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
#include "core.h"
#include "register.h"
#include "tinyxml2/tinyxml2.h"

namespace coco
{


struct scopedxml
{
	scopedxml(tinyxml2::XMLDocument *xml_doc, tinyxml2::XMLElement * apa, const std::string &atag): pa(apa)
	{
		e = xml_doc->NewElement(atag.c_str());
	}

	operator tinyxml2::XMLElement * ()  { return e; }

	tinyxml2::XMLElement *operator ->() { return e; }

	~scopedxml()
	{
		pa->InsertEndChild(e);
	}

	tinyxml2::XMLElement * e;
	tinyxml2::XMLElement * pa;
};	

tinyxml2::XMLElement* xmlnodetxt(tinyxml2::XMLDocument *xml_doc,
								 tinyxml2::XMLElement * pa,
								 const std::string &tag,
								 const std::string text);

class XMLCreator
{
public:
static bool printXMLSkeletonTask(std::string task_name, std::string task_library, 
						 std::string task_library_path, bool adddoc, bool savefile);

static void printXMLSkeletonLibrary(std::string com_library, std::string com_library_path,
				      		 bool adddoc, bool savefile);
};
}