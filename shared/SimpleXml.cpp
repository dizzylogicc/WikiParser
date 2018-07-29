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

#include "SimpleXml.h"
#include "Tidy.h"
#include <functional>
#include <buffio.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include "Common.h"

using namespace pugi;

//Removes nodes with the given name from a tree
void SimpleXml::RemoveNodesByName(xml_node& node, const BString& nameString, int& numRemoved)
{
	using namespace std::placeholders;

	SimplestXml::ApplyToElementOrDocTree(node,
				std::bind(&SimplestXml::RemoveChildrenByName,_1,std::cref(nameString),
							std::ref(numRemoved)
							)
				);
}

//Removes nodes with the given name from a tree
//If they are also present in the provided node array
void SimpleXml::RemoveNodesByNameIfPresent(xml_node& node, const BString& nameString,
											const CHArray<xml_node>& nodeArray, int& numRemoved)
{
	using namespace std::placeholders;

	SimplestXml::ApplyToElementOrDocTree(node,
				std::bind(&SimplestXml::RemoveChildrenByNameIfPresent,
							_1,
							std::cref(nameString),
							std::cref(nodeArray),
							std::ref(numRemoved)
							)
				);
}


//Removes nodes with the names in the array from a tree
void SimpleXml::RemoveNodesByName(xml_node& node, CHArray<BString>& names, int& numRemoved)
{
	using namespace std::placeholders;

	SimplestXml::ApplyToElementOrDocTree(node,
				std::bind(&SimplestXml::RemoveChildrenByNameArray,_1,std::ref(names),
							std::ref(numRemoved)
							)
				);
}

//Creates a list of nodes in the tree having nodeName and saves it into result
void SimpleXml::GetNodesByName(xml_node& node, const BString& nodeName, CHArray<xml_node>& result)
{
	using namespace std::placeholders;
	result.EraseArray();
	
	SimplestXml::ApplyToElementOrDocTree(node,
		std::bind(&SimplestXml::SaveIfNameMatches, _1, std::cref(nodeName), std::ref(result))
							);
}

//Creates a list of nodes with matching name, child name, and the value of the child
//Example: GetNodesByNameChildValue(node, "template", "target", "Main") - templates with the <target>Main</target> elements
void SimpleXml::GetNodesByNameChildValue(	xml_node& node,
											const BString& nodeName,
											const BString& childName,
											const BString& childValue,
											bool fValueToLowercase,
											CHArray<xml_node>& result)
{
	using namespace std::placeholders;
	result.EraseArray();


	SimplestXml::ApplyToElementOrDocTree(node, std::bind(&SimplestXml::SaveIfNameChildValueMatches,
												_1,
												std::cref(nodeName),
												std::cref(childName),
												std::cref(childValue),
												fValueToLowercase,
												std::ref(result)));
}

//Creates a list of nodes in the tree having nodeName and saves it into result
//Does not search subtrees with the head node named exceptInName
void SimpleXml::GetNodesByNameExceptIn(xml_node& node, const BString& nodeName,
										const BString& exceptInName, CHArray<xml_node>& result)
{
	using namespace std::placeholders;
	result.EraseArray();

	SimplestXml::ApplyToTreeExceptIn(node, exceptInName,
							std::bind(&SimplestXml::SaveIfNameMatches,_1,std::cref(nodeName),std::ref(result))
							);
}

//Fixes HTML markup to conform to XML specs
//Uses HTML Tidy
//Returns false on complete failure
//Warnings are probably fine, errors are more serious
bool SimpleXml::HTMLtidyToXml(BString& string, int& numWarnings, int& numErrors)
{
	numWarnings=0;
	numErrors=0;

	TidyBuffer output;
	TidyBuffer errbuf;
	int rc = -1;

	TidyDoc tdoc = tidyCreate();                     // Initialize "document"
	tidyBufInit( &output );
	tidyBufInit( &errbuf );

	tidyOptSetBool( tdoc, TidyXmlOut, yes );  // Convert to XML
	tidyOptSetBool(tdoc, TidyXmlTags, yes);	//The input is XML, not HTML
	tidyOptSetBool(tdoc, TidyForceOutput, yes);		//Force output even when there were errors
	tidyOptSetInt(tdoc, TidyDoctypeMode, TidyDoctypeOmit);		//No DOCTYPE declaration
	tidyOptSetBool(tdoc, TidyDropEmptyParas, no);		//Do not remove empty paragraphs
	tidyOptSetBool(tdoc, TidyMark, no);			//No generator info
	tidyOptSetBool(tdoc, TidyFixBackslash, no);	//Don't fix backslash
	tidyOptSetBool(tdoc, TidyFixComments, no);	//Don't fix comments
	tidyOptSetBool(tdoc, TidyFixUri, no);	//Don't fix uris
	tidyOptSetBool(tdoc, TidyLowerLiterals, no);	//Lower literals not important
	tidyOptSetBool(tdoc, TidyJoinStyles, no);		//There are no styles anyway
	tidyOptSetBool(tdoc, TidyQuoteAmpersand, no);	//Do not change & to &amp;
	tidyOptSetBool(tdoc, TidyQuoteMarks, no);	//Do not write &quot;
	tidyOptSetBool(tdoc, TidyQuoteNbsp, yes);	//Do write &nbsp;
	tidyOptSetInt(tdoc, TidyIndentContent, no);		//No content indentation
	tidyOptSetInt(tdoc, TidyWrapLen, 30000);		//Very large wrap limit
	tidyOptSetInt(tdoc, TidyTabSize, 1);		//tabs are replaced with a single space
	tidyOptSetBool(tdoc, TidyWrapAttVals, no);	//No wrapping
	tidyOptSetBool(tdoc, TidyWrapScriptlets, no);	//No wrapping
	tidyOptSetBool(tdoc, TidyWrapSection, no);	//No wrapping
	tidyOptSetBool(tdoc, TidyWrapAsp, no);	//No wrapping
	tidyOptSetBool(tdoc, TidyWrapJste, no);	//No wrapping
	tidyOptSetBool(tdoc, TidyWrapPhp, no);	//No wrapping
	tidyOptSetBool(tdoc, TidyAsciiChars, no);	//No converting &mdash; etc. to ascii
	tidyOptSetBool(tdoc, TidyHideComments, yes);	//Remove comments
	tidySetCharEncoding(tdoc, "raw");		//Encoding - raw (don't convert chars > 127)

	rc = tidySetErrorBuffer( tdoc, &errbuf );      // Capture diagnostics

	//Tidy must be fixed to avoid stack overflow on certain input in ParseXMLElement()
	if ( rc >= 0 ) rc = tidyParseString( tdoc, (const char*)string );           // Parse the input
	if ( rc >= 0 ) rc = tidyCleanAndRepair( tdoc );               // Tidy it up!

	//if ( rc >= 0 )rc = tidyRunDiagnostics( tdoc );               // Diagnostics write to errbuff
	
	if( rc >= 0 )
	{
		numWarnings=(int)tidyWarningCount(tdoc);
		numErrors=(int)tidyErrorCount(tdoc);
	}
	
	if ( rc >= 0 ) rc = tidySaveBuffer( tdoc, &output );          // Pretty Print
	if ( rc >= 0 ) string=(char*)output.bp;
	
	tidyBufFree( &output );
	tidyBufFree( &errbuf );
	tidyRelease( tdoc );
	
	if( rc >= 0 ) return true;
	else return false;
}