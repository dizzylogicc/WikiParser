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

using pugi::xml_parse_result;
using pugi::xml_node;
using pugi::xml_document;
using pugi::node_element;
using pugi::node_document;

namespace SimplestXml
{
	void XmlToString(xml_node& node, BString& string, bool fEscapeEntities = false, bool fIndented = false);
	xml_parse_result StringToXml(xml_document& doc, BString& string);
	bool WriteNodeToFile(const BString& fileName, xml_node& node, bool fEscapeEntities = false, bool fIndented = false);
	bool ReadNodeFromFile(const BString& fileName, xml_document& doc);
	
	void CopyChildrenToNode(xml_node& from, xml_node& to);
	void CopyChildrenBefore(xml_node& fromNode, xml_node& beforeNode);
	void InsertChildrenBefore(xml_node& from, xml_node& beforeNode);
	void RemoveAllChildren(xml_node& node);		//Removes all child nodes from a given node
	void RemoveAllAttributes(xml_node& node);	//Removes all attributes from a node
	void RemoveChildrenByName(xml_node& node, const BString& nameString, int& numRemoved);
	void RemoveChildrenByNameArray(xml_node& node, CHArray<BString>& names, int& numRemoved);
	void RemoveChildrenByNameIfPresent(xml_node& node, const BString& nameString,const CHArray<xml_node>& nodeArray, int& numRemoved);
	
	//Get value from a node and write it to result
	void ReadValueFromXML(xml_node& parent, BString& result, const BString& nodeName);
	void ReadValueFromXML(xml_node& parent, bool& result, const BString& nodeName);
	void ReadValueFromXML(xml_node& parent, int& result, const BString& nodeName);

	//Returns the first node with the specified name in the subtree, result is null if not found
	xml_node GetNodeByName(xml_node& treeRoot, const BString& nodeName);

	//If the name of the node matches, save the node handle to theArray
	void SaveIfNameMatches(xml_node& node, const BString& nodeName, CHArray<xml_node>& theArray);

	//Saves the node for matching nodename, matching childname and matching child value
	//Example: find nodes "template" with child "target" having node_pcdata with value "Main"
	//SaveIfNameChildValueMatches(node, "template", "target", "Main", result)
	void SaveIfNameChildValueMatches(	xml_node& node,
										const BString& nodeName,
										const BString& childName,
										const BString& childValue,
										bool fValueToLowercase,
										CHArray<xml_node>& result);

	//Whether node name, child name, and child value matches for this node
	bool NameChildValueMatches(	xml_node& node,
								const BString& nodeName,
								const BString& childName,
								const BString& childValue,
								bool fValueToLowercase);

	//Finds the first child with matching name, child name and child value
	xml_node FindChildByNameChildValue(	xml_node& node,
										const BString& nodeName,
										const BString& childName,
										const BString& childValue,
										bool fValueToLowercase);

	//recursive template with a callable type
	//calls a function for every node in the tree
	template<typename F>
	void ApplyToTree(xml_node& node, F&& funcToCall)
	{
		//Call the function 
		funcToCall(node);

		xml_node child=node.first_child();
		while(child)
		{
			ApplyToTree(child,funcToCall);
			child=child.next_sibling();
		}
	}

	//Will only act on element or document nodes that are part of the "element tree"
	//Will skip node_pcdata
	template<typename F>
	void ApplyToElementOrDocTree(xml_node& node, F&& funcToCall)
	{
		if(node.type()==node_element || node.type()==node_document) funcToCall(node);

		xml_node child=node.first_child();
		while(child)
		{
			if(node.type()==node_element || node.type()==node_document) ApplyToElementOrDocTree(child,funcToCall);
			child=child.next_sibling();
		}
	}

	//Will only act on element or document nodes that are part of the "element tree"
	//Will skip node_pcdata
	//Will skip subtrees with head node named exceptInName
	template<typename F>
	void ApplyToTreeExceptIn(xml_node& node, const BString& exceptInName, F&& funcToCall)
	{
		BString name=node.name();
		if(name==exceptInName) return;

		if(node.type()==node_element || node.type()==node_document) funcToCall(node);

		xml_node child=node.first_child();
		while(child)
		{
			if(node.type()==node_element || node.type()==node_document) ApplyToTreeExceptIn(child,exceptInName,funcToCall);
			child=child.next_sibling();
		}
	}
}

