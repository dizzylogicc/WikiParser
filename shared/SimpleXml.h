/* Copyright (c) 2018 Peter Kondratyuk. All Rights Reserved.
*
* You may use, distribute and modify the code in this file under the terms of the MIT Open Source license, however
* if this file is included as part of a larger project, the project as a whole may be distributed under a different
* license.
*
* MIT license:
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
* documentation files (the "Software"), to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
* to permit persons to whom the Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or substantial portions
* of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
* CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*/

#pragma once

#include "pugixml.hpp"
#include "Array.h"
#include "SimplestXml.h"

using pugi::xml_parse_result;
using pugi::xml_node;
using pugi::xml_document;
using pugi::node_element;
using pugi::node_document;

namespace SimpleXml
{
	void RemoveNodesByName(xml_node& node, const BString& nameString, int& numRemoved);
	void RemoveNodesByName(xml_node& node, CHArray<BString>& names, int& numRemoved);
	void RemoveNodesByNameIfPresent(xml_node& node, const BString& nameString,const CHArray<xml_node>& nodeArray, int& numRemoved);
	bool HTMLtidyToXml(BString& string, int& numWarnings, int& numErrors);

	//Creates a list of xml_nodes with the provided name
	void GetNodesByName(xml_node& node, const BString& nodeName, CHArray<xml_node>& result);

	//Creates a list of nodes in the tree having nodeName and saves it into result
	//Does not search subtrees with the head node named exceptInName
	void GetNodesByNameExceptIn(xml_node& node, const BString& nodeName,
										const BString& exceptInName, CHArray<xml_node>& result);

	//Creates a list of nodes with matching name, child name, and the value of the child
	//Example: GetNodesByNameChildValue(node, "template", "target", "Main") - templates with the <target>Main</target> elements
	void GetNodesByNameChildValue(	xml_node& node,
									const BString& nodeName,
									const BString& childName,
									const BString& childValue,
									bool fValueToLowercase,
									CHArray<xml_node>& result);
}

