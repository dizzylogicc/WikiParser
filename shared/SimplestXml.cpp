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

#include "SimplestXml.h"
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include "CommonUtility.h"

using namespace pugi;

namespace pugi
{
	/////////////////Extention for pugixml memory writer
	struct xml_memory_writer: pugi::xml_writer
	{
	    char* buffer;
	    size_t capacity;
	    size_t result;

	    xml_memory_writer(): buffer(0), capacity(0), result(0){}
		xml_memory_writer(char* buffer, size_t capacity): buffer(buffer), capacity(capacity), result(0){}

	    size_t written_size() const
	    {
	        return result < capacity ? result : capacity;
	    }

	    virtual void write(const void* data, size_t size)
	    {
	        if (result < capacity)
			{
				size_t chunk = (capacity - result < size) ? capacity - result : size;
				memcpy(buffer + result, data, chunk);
			}
			result += size;
		}
	};

	char* node_to_buffer_heap(pugi::xml_node node, bool fEscapeEntities = false, bool fIndented = false)
	{
		unsigned int options;
		if(fIndented) options = format_indent;
		else options = format_raw;

		if(!fEscapeEntities) options = options | format_no_escapes;

	    // first pass: get required memory size
	    xml_memory_writer counter;
		node.print(counter, "\t" , options);

	    // allocate necessary size (+1 for null termination)
	    char* buffer = new char[counter.result + 1];

	    // second pass: actual printing
	    xml_memory_writer writer(buffer, counter.result);
	    node.print(writer, "\t", options);

	    // null terminate
	    buffer[writer.written_size()] = 0;

	    return buffer;
	}
} //////////////End extension for memory writer


//Save XML node to the provided string
void SimplestXml::XmlToString(xml_node& node, BString& string, bool fEscapeEntities, bool fIndented)
{
	char* heap_buf = node_to_buffer_heap(node, fEscapeEntities, fIndented);
	string=heap_buf;
    delete[] heap_buf;
}

bool SimplestXml::ReadNodeFromFile(const BString& fileName, xml_document& doc)
{
	BString buffer;
	if(!CommonUtility::ReadStringFromFile(buffer, fileName)) return false;
	xml_parse_result result = StringToXml(doc, buffer);
	return result;
}

bool SimplestXml::WriteNodeToFile(const BString& fileName, xml_node& node, bool fEscapeEntities, bool fIndented)
{
	BString buffer;
	XmlToString(node, buffer, fEscapeEntities, fIndented);
	return CommonUtility::WriteStringToFile(buffer, fileName);
};

xml_parse_result SimplestXml::StringToXml(xml_document& doc, BString& string)
{
	xml_parse_result res=doc.load_buffer(string,string.GetLength(),parse_ws_pcdata);
	return res;
}

//Copy all child nodes from a node into another XML node
void SimplestXml::CopyChildrenToNode(xml_node& from, xml_node& to)
{
	xml_node cur=from.first_child();
	while(cur)
	{
		to.append_copy(cur);
		cur=cur.next_sibling();
	}
}

//Copy all child nodes in front of another node
void SimplestXml::CopyChildrenBefore(xml_node& fromNode, xml_node& beforeNode)
{
	xml_node cur=fromNode.first_child();
	while(cur)
	{
		beforeNode.parent().insert_copy_before(cur,beforeNode);
		cur=cur.next_sibling();
	}
}

void SimplestXml::InsertChildrenBefore(xml_node& from, xml_node& beforeNode)
{
	xml_node cur=from.first_child();
	while(cur)
	{
		beforeNode.parent().insert_copy_before(cur,beforeNode);
		cur=cur.next_sibling();
	}
}

//Removes all child nodes from a given node
void SimplestXml::RemoveAllChildren(xml_node& node)
{
	xml_node child=node.first_child();

	while(child)
	{
		xml_node curNode=child;
		child=child.next_sibling();
		node.remove_child(curNode);
	}
}

//Removes all atributes from a node
void SimplestXml::RemoveAllAttributes(xml_node& node)
{
	xml_attribute attrib=node.first_attribute();
	while(attrib)
	{
		xml_attribute cur=attrib;
		attrib=attrib.next_attribute();
		node.remove_attribute(cur);
	}
}

//Removes children with matching names from a node
void SimplestXml::RemoveChildrenByName(xml_node& node, const BString& nameString, int& numRemoved)
{
	xml_node child=node.first_child();

	while(child)
	{
		xml_node curChild=child;
		child=child.next_sibling();

		if(curChild.type()==node_element && nameString==curChild.name())
		{
			node.remove_child(curChild);
			numRemoved++;
		}
	}
}

//Removes children with matching names from a node
//If they are also present in the provided 
void SimplestXml::RemoveChildrenByNameIfPresent(xml_node& node, const BString& nameString,
												const CHArray<xml_node>& nodeArray, int& numRemoved)
{
	xml_node child=node.first_child();

	while(child)
	{
		xml_node curChild=child;
		child=child.next_sibling();

		if(curChild.type()==node_element && nameString==curChild.name() && nodeArray.IsPresent(curChild))
		{
			node.remove_child(curChild);
			numRemoved++;
		}
	}
}

//Removes children with the names in the array from a node
void SimplestXml::RemoveChildrenByNameArray(xml_node& node, CHArray<BString>& names, int& numRemoved)
{
	xml_node child=node.first_child();

	while(child)
	{
		xml_node curChild=child;
		child=child.next_sibling();

		if(curChild.type()==node_element && names.IsPresent(curChild.name()))
		{
			node.remove_child(curChild);
			numRemoved++;
		}
	}
}







//Returns the first node with the specified name in the subtree, result is null if not found
xml_node SimplestXml::GetNodeByName(xml_node& treeRoot, const BString& nodeName)
{
	BString name=treeRoot.name();
	if(name==nodeName) return treeRoot;

	xml_node curChild;
	xml_node result;	//null node on creation
	for(curChild=treeRoot.first_child(); curChild; curChild=curChild.next_sibling())
	{
		if(curChild.type()!=node_element && curChild.type()!=node_document) continue;

		result=GetNodeByName(curChild,nodeName);
		if(result) break;
	}

	return result;
}



//Saves the node for matching nodename, matching childname and matching child value
//Example: find nodes "template" with child "target" having node_pcdata with value "Main"
//SaveIfNameChildValueMatches(node, "template", "target", "Main", result)
void SimplestXml::SaveIfNameChildValueMatches(	xml_node& node,
												const BString& nodeName,
												const BString& childName,
												const BString& childValue,
												bool fValueToLowercase,
												CHArray<xml_node>& result)
{
	if( NameChildValueMatches(node,nodeName,childName,childValue,fValueToLowercase) ) result.AddAndExtend(node);
}

//Finds the first child with matching name, child name and child value
xml_node SimplestXml::FindChildByNameChildValue(	xml_node& node,
												const BString& nodeName,
												const BString& childName,
												const BString& childValue,
												bool fValueToLowercase)
{
	xml_node curChild=node.first_child();
	while(curChild)
	{
		if( NameChildValueMatches(curChild,nodeName,childName,childValue,fValueToLowercase) ) return curChild;

		curChild=curChild.next_sibling();
	}

	return xml_node();	//return null xml_node if not found
}
	

//Whether node name, child name, and child value matches for this node
bool SimplestXml::NameChildValueMatches(	xml_node& node,
										const BString& nodeName,
										const BString& childName,
										const BString& childValue,
										bool fValueToLowercase)
{
	if( node.type() == node_element && nodeName == node.name() )
	{
		

		xml_node childNode=node.child(childName);
		if( childNode )
		{
			BString val = childNode.first_child().value();
			val.Trim();

			if(fValueToLowercase) val.MakeLower();

			if(val==childValue) return true;
		}
	}

	return false;
}

//If the name of the node matches, save the node handle to theArray
void SimplestXml::SaveIfNameMatches(xml_node& node, const BString& nodeName, CHArray<xml_node>& theArray)
{
	if(nodeName==node.name()) theArray.AddAndExtend(node);
}

void SimplestXml::ReadValueFromXML(pugi::xml_node& parent, BString& result, const BString& nodeName)
{
	xml_node node=parent.child(nodeName);
	if(node) result=node.text().as_string();
}

void SimplestXml::ReadValueFromXML(pugi::xml_node& parent, bool& result, const BString& nodeName)
{
	xml_node node=parent.child(nodeName);
	if(node) result=node.text().as_bool();
}

void SimplestXml::ReadValueFromXML(pugi::xml_node& parent, int& result, const BString& nodeName)
{
	xml_node node=parent.child(nodeName);
	if(node) result=node.text().as_int();
}