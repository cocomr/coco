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
#include "tinyxml2/tinyxml2.h"

#include "coco/register.h"

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
