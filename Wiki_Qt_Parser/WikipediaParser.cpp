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

#include "WikipediaParser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "WordTrace.h"
#include "DizzyUtility.h"

#include "QtUtils.h"

#include "boost/bind.hpp"

using namespace pugi;
using namespace SimpleXml;
using namespace SimplestXml;
using namespace DizzyUtility;

CWikipediaParser::CWikipediaParser(const BString& parserFolder, bool fEncodedFile):
errorMapGeneral(100,true),
errorMapRedirects(100,true),
errorMapTemplates(100,true),
errorMapArtDisambigs(100,true),
retainedTemplates(10)
{
	//Read the parser data - it can either be in plain text files in a directory
	//Or in a serialized file, in which case "parserFolder" is really a file name
	if(!fEncodedFile) ReadPlainParserData(parserFolder);
	else Load(parserFolder);
	
	//Protected template names - all other templates are removed after page parse
	retainedTemplates.AddWord(BString("main"));
	retainedTemplates.AddWord(BString("see also"));
	retainedTemplates.AddWord(BString("aircontent"));

	//Tag names that are removed during initial cleanup
	tagNamesForCleanup.ResizeArray(9);
	tagNamesForCleanup << "ref"
						<< "references"
						<< "math"
						<< "code"
						<< "syntaxhighlight"
						<< "nowiki"
						<< "noinclude"
						<< "pre"
						<< "br";


	//Targets (in lowercase) for various disambiguation templates
	//If such a template is present, the page is a disambiguation page
	disambigTargets.ResizeArray(10);
	disambigTargets << "disambiguation"
					<< "disambiguation cleanup"
					<< "dab"
					<< "disamb"
					<< "disambig"
					<< "surname"
					<< "school disambiguation"
					<< "hndis"
					<< "geodis"
					<< "hospital disambiguation";

	//Element node names that are skipped when section nodes are cleaned up with Tidy individually
	//After page cleanup and section cleanup have failed
	skipInNodeCleanup.ResizeArray(6);
	skipInNodeCleanup << "link"
						<< "extLink"
						<< "media"
						<< "category"
						<< "interwiki"
						<< "target";
}

void CWikipediaParser::Serialize(BArchive& archive)
{
	archive & languageMap
			& convertTable
			& convertMap
			& imExtensionMap
			& infoImageMarkers
			& infoCaptionMarkers;
}

void CWikipediaParser::ReadPlainParserData(const BString& folder)
{
	//Load interwiki language prefixes
	CHArray<BString> temp;
	temp.ReadStrings(folder+"Language prefixes.txt");
	languageMap.CreateFromArray(temp);

	//Tables for convert template processing
	convertTable.ReadStrings(folder+"Convert template.txt");
	convertMap.CreateFromArray(convertTable[0]);

	//Image extensions that we can handle
	temp.ReadStrings(folder+"Image extensions - included.txt");
	imExtensionMap.CreateFromArray(temp);

	//Load infobox image and caption markers
	temp.ReadStrings(folder+"Infobox image params for parser.txt");
	infoImageMarkers.CreateFromArray(temp);
	temp.ReadStrings(folder+"Infobox caption params for parser.txt");
	infoCaptionMarkers.CreateFromArray(temp);

	Save(folder+"pdata.cfg");
}

//Write all errors to the provided file stream
void CWikipediaParser::WriteReport(std::ostream& report)
{
	report<<"\nPARSER OUTPUT\n";
	report<<"All pages:\n";
	WriteErrorMap(report,errorMapGeneral);

	report<<"\nRedirects:\n";
	WriteErrorMap(report,errorMapRedirects);

	report<<"\nInfobox templates:\n";
	WriteErrorMap(report,errorMapTemplates);

	report<<"\nArticles and disambiguations:\n";
	WriteErrorMap(report,errorMapArtDisambigs);
}

//Private function writing one error map to file stream
void CWikipediaParser::WriteErrorMap(std::ostream& report, CBidirectionalMap<BString>& theErrorMap)
{
	theErrorMap.SortByFrequencies();
	for(int i=0;i<theErrorMap.Count();i++){report<<theErrorMap[i]<<" - "<<theErrorMap.freqArr[i]<<"\n";}
}

//Clean individual nodes with HTML Tidy - when the page cleanup and section cleanup has failed
//Will call TidyAndClean()
void CWikipediaParser::TidyAndCleanNode(xml_node& node)
{
	//Run this function on all children that are element nodes and are not in skipInNodeCleanup
	xml_node child=node.first_child();

	while(child)
	{
		xml_node curChild=child;
		child=child.next_sibling();

		if(curChild.type()==node_element && !skipInNodeCleanup.IsPresent(curChild.name()))
		{
			//Write this child to string, try HTML Tidy, and if it fails, call this function recursively on the child
			//and try again.
			//if that fails, remove the node
			BString string;
			XmlToString(curChild,string);

			bool tidyRes=TidyAndClean(string,"Node-level cleanup: ");

			if(!tidyRes)
			{
				TidyAndCleanNode(curChild);
				XmlToString(curChild,string);
				tidyRes=TidyAndClean(string,"Retrying node-level cleanup: ");
			}

			if(tidyRes)		//success, place the processed child before the current one
			{
				xml_document tempDoc;
				StringToXml(tempDoc,string);
				
				node.insert_copy_before(tempDoc.first_child(),curChild);
			}

			node.remove_child(curChild);	//remove the old node
		}
	}
}

//Looks for one of the disambiguation templates
//In the tree structure and returns true if found
//otherwise false
bool CWikipediaParser::IsDisambiguationPage(xml_node& node)
{
	BString target;

	xml_node child=node.first_child();
	while(child)
	{
		if(child.type()==node_element)
		{
			if(BString(child.name())=="template")
			{
				target=child.child("target").first_child().value();
				target.MakeLower();

				if(disambigTargets.IsPresent(target)) return true;
			}
			else
			{
				if(IsDisambiguationPage(child)) return true;
			}
		}

		child=child.next_sibling();
	}

	return false;
}





bool CWikipediaParser::ParseBraces(const BString& text, char left, char right,CHArray<int>& openArr,
								CHArray<int>& closeArr, CHArray<int>& setStart, CHArray<int>& setLength,CHArray<char>& markup)
{
	//Parses braces in the text, such as left='{' and right='}'
	//Stores opening position for each brace in openArr and closing position in closeArr
	//Stores the start position of each brace set in setStart, and its length (i.g., 3 for "{{{...}}}") in setLength
	//OpenArr and closeArr are sorted in the order in which the braces are opened
	//Markup is an array of the same length as text that replaces all opening braces with a positive integer
	//all closing braces with a negative integer, where integer is the number of braces in a set,
	//and all non-brace characters with zeros

	int length=text.GetLength();
	int numBraces=0;				
	int numBracesClose=0;
	for(int i=0;i<length;i++)		//counting braces
	{
		if(text[i]==left) numBraces++;
		if(text[i]==right) numBracesClose++;
	}
	//check for mismatched braces
	if(numBraces!=numBracesClose)
	{
		AddError(BString("Critical section error: mismatched ")+left+"..."+right+" braces.");
		return false;
	}

	//resize all arrays
	openArr.ResizeArray(numBraces);
	closeArr.ResizeArray(numBraces);
	setStart.ResizeArray(numBraces);	//the index in openArr where the curly set starts
	setLength.ResizeArray(numBraces);	//the number of braces in a set
	markup.ResizeArray(length,true);
	markup=0;

	if(numBraces==0) return true;		//no braces found - we are done

	CHArray<int> stack(numBraces);
	for(int i=0;i<length;i++)
	{
		if(text[i]==left) stack.Push(i);
		if(text[i]==right) 
		{
			if(!stack.fDataPresent())	//trying to pop an empty stack - error: a closing brace without an opening one
			{
				AddError(BString("Critical section error: closing ")+right+" without opening "+left+".");
				return false;		
			}

			closeArr.AddPoint(i);
			openArr.AddPoint(stack.Pop());
		}
	}

	CHArray<int> perm;
	openArr.SortPermutation(perm);
	openArr.Permute(perm);	//Sort opening and closing positions in the order in which the braces are opened
	closeArr.Permute(perm);

	//compute distances between neighboring opening and closing brackets and find multiple brace sets
	CHArray<int> neighbors(numBraces-1,true);
	neighbors=0;
	for(int i=1;i<numBraces;i++)
	{
		int openDist=openArr[i]-openArr[i-1];		//if they are next to each other, the distance will be 1
		int closeDist=closeArr[i-1]-closeArr[i];	//and so will this distance

		if(openDist==1 && closeDist==1) neighbors[i-1]=1;	//these are neighbors
	}
	//every isolated 1 in "neighbors" means double brace set, isolated 11 - tripple brace set, and so on
	//every 00 will mean a single brace
	
	//Find the start point and length of every brace set
	int curLength=1;
	int curStart=0;
	for(int i=0;i<(numBraces-1);i++)
	{
		if(neighbors[i]) curLength++; //brace set continues
		else		//brace set ends
		{
			setStart.AddPoint(curStart);
			setLength.AddPoint(curLength);
			curStart=i+1;
			curLength=1;
		}
	}
	setStart.AddPoint(curStart);		//final set
	setLength.AddPoint(curLength);
	int numSets=setStart.GetNumPoints();

	//Create markup array
	for(int i=0;i<numSets;i++)
	{
		int curStart=openArr[setStart[i]];
		int curEnd=closeArr[setStart[i]];
		int curLength=setLength[i];

		for(int j=0;j<curLength;j++)
		{
			markup[curStart+j]=curLength;
			markup[curEnd-j]=-curLength;
		}
	}

	return true;
}

//Processes '', ''' and ''''' tags
void CWikipediaParser::ProcessBoldItalic(BString& text)
{
	//Ascii number for ' is 39

	int textLength=text.GetLength();
	CHArray<char> markup(textLength,true);	//number of apostrophes starting at that symbol, single apostrophes are ignored
	markup=0;

	//Find apostrophe runs of length 2, 3 or 5
	for(int i=0;i<textLength;i++)
	{
		int numApos=0;
		int startPos=i;
		while(i<textLength && text[i]==39)
		{
			numApos++;
			i++;
		}

		if(numApos==2 || numApos==3 || numApos==5)	//all other apostrophe runs are ignored
		{
			markup[startPos]=(char)numApos;
		}
	}

	//Check that all apos runs come in pairs 2-2, 3-3 or 5-5
	//If not, remove all apostrophes from the text and return
	bool fCorrect=true;
	int lastSeen;
	bool fBeginFound=false;
	for(int i=0;i<textLength;i++)
	{
		if(markup[i]!=0)
		{
			if(!fBeginFound)
			{
				lastSeen=markup[i];
				fBeginFound=true;
			}
			else
			{
				if(lastSeen!=markup[i])		//error - no matching pair
				{
					fCorrect=false;
					break;
				}
				else						//matching pair found
				{
					fBeginFound=false;
				}
			}
		}
	}

	if(!fCorrect || fBeginFound)	//no matching pair or last pair not closed
	{
		AddError("Non-critical section error: error parsing '', ''', ''''' tags.");
		text.Remove('\'');
		return;
	}

	//Count all tags and mark all closing tags with a minus sign
	int numTags=0;
	char sign=1;
	for(int i=0;i<textLength;i++)
	{
		if(markup[i]!=0)
		{
			numTags++;
			markup[i]*=sign;
			sign*=-1;
		}
	}

	//Put in <b>, <i> tags
	CHArray<char> boldBegin("<b>",3);
	CHArray<char> boldEnd("</b>",4);
	CHArray<char> itBegin("<i>",3);
	CHArray<char> itEnd("</i>",4);

	CHArray<char> newText(textLength+20*numTags+1);
	for(int i=0;i<textLength;i++)
	{
		if(markup[i]==0) {newText.AddPoint(text[i]);continue;}
		if(markup[i]==2) {newText.AddFromArray(itBegin);i+=1;continue;}
		if(markup[i]==-2) {newText.AddFromArray(itEnd);i+=1;continue;}
		if(markup[i]==3) {newText.AddFromArray(boldBegin);i+=2;continue;}
		if(markup[i]==-3) {newText.AddFromArray(boldEnd);i+=2;continue;}
		if(markup[i]==5) {newText.AddFromArray(boldBegin);newText.AddFromArray(itBegin);i+=4;continue;}
		if(markup[i]==-5) {newText.AddFromArray(itEnd);newText.AddFromArray(boldEnd);i+=4;continue;}
	}
	newText.AddPoint(0);
	text=newText.arr;

	return;
}

//Parses a section selected by ParseArticle
bool CWikipediaParser::ParseSection(const BString& theSection, xml_document& output, bool fAlreadyCleaned)
{
	BString text=theSection;

	if(!fAlreadyCleaned)	//if page-level cleaning failed
	{
		//Attempt to clean the text with HTML Tidy and
		//remove <ref>, <math>, <code> etc. tags and their content

		//If cleaning on the section fails, we will do the cleaning at the node level after all the parsing is complete
		fAlreadyCleaned=TidyAndClean(text,"Section cleanup: ");
	}

	int textLength=text.GetLength();
		
	//Step 1: parse curly braces
	CHArray<int> openArr, closeArr, setStart, setLength;
	CHArray<char> markup;
	//Stores opening position for each brace in openArr and closing position in closeArr
	//Stores the start position of each brace set in setStart, and its length (i.g., 3 for "{{{...}}}") in setLength
	//OpenArr and closeArr are sorted in the order in which the braces are opened
	bool ret=ParseBraces(text,'{','}',openArr,closeArr,setStart,setLength,markup);
	if(!ret) return false;

	int numSets=setStart.GetNumPoints();
	if(setLength.Max()>3)
	{
		AddError("Critical section error: more than 3 curly braces in a set.");
		return false;
	}

	//Remove from markup the sets of single braces that are not tables
	for(int i=0;i<numSets;i++)
	{
		if(setLength[i]!=1) continue;

		int curStart=openArr[setStart[i]];
		int curEnd=closeArr[setStart[i]];

		if(text[curStart+1]!='|' || text[curEnd-1]!='|') 
		{
			markup[curStart]=0;
			markup[curEnd]=0;
		}
	}

	CHArray<char> newText(textLength+1+numSets*140);		//70 additional symbols allocated for tags replacing {{...}}
	CHArray<char> tableBegin("<wTable>",8);
	CHArray<char> tableEnd("</wTable>",9);
	CHArray<char> templateBegin("<template>",10);
	CHArray<char> templateEnd("</template>",11);
	CHArray<char> curlyThreeBegin("<curlyThree>",12);
	CHArray<char> curlyThreeEnd("</curlyThree>",13);
	
	for(int i=0;i<textLength;i++)
	{
		char curMarkup=markup[i];
		if(curMarkup==0) newText.AddPoint(text[i]);
		if(curMarkup==1) {newText.AddFromArray(tableBegin);}
		if(curMarkup==-1) {newText.AddFromArray(tableEnd);}
		if(curMarkup==2) {newText.AddFromArray(templateBegin);i++;}
		if(curMarkup==-2) {newText.AddFromArray(templateEnd);i++;}
		if(curMarkup==3) {newText.AddFromArray(curlyThreeBegin);i+=2;}
		if(curMarkup==-3) {newText.AddFromArray(curlyThreeEnd);i+=2;}
	}
	newText.AddPoint(0);
	text=newText.arr;
	textLength=text.GetLength();

	//Step 2: parse square braces
	ret=ParseBraces(text,'[',']',openArr,closeArr,setStart,setLength,markup);
	if(!ret) return false;
	
	numSets=setStart.GetNumPoints();
	if(setLength.Max()>2)
	{
		AddError("Critical section error: more than 2 square braces in a set.");
		return false;
	}

	newText.ResizeArray(textLength+1+numSets*70);		//70 additional symbols allocated for tags replacing [[...]]
	CHArray<char> extLinkBegin("<extLink>",9);		//external link, [...]
	CHArray<char> extLinkEnd("</extLink>",10);
	CHArray<char> linkBegin("<link>",6);			//internal link, [[...]]
	CHArray<char> linkEnd("</link>",7);
		
	for(int i=0;i<textLength;i++)
	{
		char curMarkup=markup[i];
		if(curMarkup==0) {newText.AddPoint(text[i]);}
		if(curMarkup==1) {newText.AddFromArray(extLinkBegin);}
		if(curMarkup==-1) {newText.AddFromArray(extLinkEnd);}
		if(curMarkup==2) {newText.AddFromArray(linkBegin);i++;}
		if(curMarkup==-2) {newText.AddFromArray(linkEnd);i++;}
	}
	newText.AddPoint(0);
	text=newText.arr;

	//Step 3: parse all templates - extract their names and parameters
	xml_document doc;
	if(!StringToXml(doc,text))
	{
		AddError("Critical section error: XML parsing error after template/link delimiting.");
		return false;
	}
	ParseTemplates(doc);

	//Step 3b: process bold and italic markers - '', ''', '''''
	XmlToString(doc,text);
	ProcessBoldItalic(text);
	textLength=text.GetLength();

	if(!StringToXml(doc,text))
	{
		AddError("Critical section error: XML parsing error after bold/italic delimiting.");
		return false;
	}
	
	//Step 4: parse all links
	ParseLinks(doc);

	//Remove LF from all elements in the head node - i.e., from all template and link elements already delimited
    xml_node tempFirstChild = doc.first_child();
    RemoveLFfromChildElements(tempFirstChild);

	//Write XML to text
	XmlToString(doc,text);
	textLength=text.GetLength();

	//Step 5: insert paragraph tags
	text.Replace("\x0A\x0A","\x0A</par><par>\x0A");		//every LF-LF is paragraph closing and opening
	int pos1, pos2;
	pos1=text.Find('>',0);							//the position of the closing brace on the opening head node tag
	text.Insert(pos1+1,"<par>");					//open first paragraph
	pos2=text.ReverseFind('<');						//the position of the opening brace on the closing head node tag
	text.Insert(pos2,"</par>");						//close last paragraph

	//At this point, all content is within unnested <par> tags

	//Convert back to XML
	if(!StringToXml(output,text))
	{
		AddError("Critical section error: parse error after inserting <par> tags.");
		return false;
	}

	//Step 5A:
	//If both page-level and section-level Tidy cleanup has failed,
	//Do the cleanup at the node level now
	//Attempts to clean each element node, and deletes it on failure
	if(!fAlreadyCleaned)
	{
        xml_node temp = output.first_child();
        TidyAndCleanNode(temp);
		fAlreadyCleaned=true;
	}

	//Step 6: for every paragraph, insert list element tags
	xml_node curPar=output.first_child().child("par");
	while(curPar)
	{
		InsertListElInParagraph(curPar);
		curPar=curPar.next_sibling();
	}

	//Move all lists out of <par> and into <list>
	MoveListElToLists(output);

	return true;
}

//Inserts <listEl> tags into paragraphs - called by ParseSection()
bool CWikipediaParser::InsertListElInParagraph(xml_node& parNode)
{
	BString text;
	XmlToString(parNode,text);
	int textLength=text.GetLength();

	RemoveAllChildren(parNode);

	//Create list markup and count list elements
	//0-not a list character, 1-list character to copy, 2-list character to omit
	CHArray<char> listMarkup(textLength,true);
	listMarkup=0;

	int listElCount=0;
	int pos2=text.ReverseFind('<');		//closing tag position of the head node
	for(int i=0;i<pos2;i++)
	{
		if(text[i]==10 && IsListSymbol(text[i+1]))	//Start of list el found
		{
			listElCount++;
			listMarkup[i]=2;
			i++;
			while(i<pos2 && IsListSymbol(text[i]))	//Mark elements to be omitted
			{
				listMarkup[i]=2;
				i++;
			}
			while(i<pos2 && text[i]!=10)		//mark elements to be included
			{
				listMarkup[i]=1;
				i++;
			}
			if(text[i]==10) i--;
		}
	}

	//Insert <listEl> tags
	CHArray<char> listElBegin("<listEl>",8);
	CHArray<char> listElEnd("</listEl>",9);
	CHArray<char> newText(textLength+1+50+20*listElCount);
	
	for(int i=0;i<textLength;i++)
	{
		if(i>0 && listMarkup[i-1]==1 && listMarkup[i]!=1)	//this is the end of a list element
		{
			newText.AddFromArray(listElEnd);
		}

		if( i<(textLength-1) && listMarkup[i]==2 && listMarkup[i+1]==1)	//this is the beginning of a list element
		{
			newText.AddFromArray(listElBegin);
		}

		if(listMarkup[i]!=2) newText.AddPoint(text[i]);		//copy character
	}
	newText.AddPoint(0);
	text=newText.arr;

	//Now all LFs can be deleted
	text.Remove(char(10));
	textLength=text.GetLength();

	//Try parsing the text with the inserted <listEl> tags
	xml_document doc;
	if(!StringToXml(doc,text))
	{
		AddError("Critical paragraph error: XML parsing error after inserting <listEl> tags.");
		return false;
	}

    xml_node temp = doc.first_child();
    CopyChildrenToNode(temp,parNode);
	return true;
}

//Removes LF from the type-element children of the node 
void CWikipediaParser::RemoveLFfromChildElements(xml_node& node)
{
	xml_node child=node.first_child();

	while(child)
	{
		xml_node curChild=child;
		child=child.next_sibling();

		if(curChild.type()==node_element)
		{
			BString string;
			XmlToString(curChild,string);

			int numRemoved=string.Remove('\x0A');
			if(numRemoved>0)
			{
				xml_document tempDoc;
				StringToXml(tempDoc,string);
				node.insert_copy_before(tempDoc.first_child(),curChild);
				node.remove_child(curChild);
			}
		}
	}
}

//Creates the markup that shows whether the character is inside or outside of any elements
//(excluding the head element)
//1-outside, 0-inside
//head node - <firstPara>, or <secTitle>, or <secContent> - must not have attributes
void CWikipediaParser::CreateElementOutsideMarkup(xml_document& doc, CHArray<char>& markup, int textLength)
{
	xml_node node1=doc.first_child();	//main node - <firstPara>, or <secTitle>, or <secContent> - must not have attributes
	BString node1Name=node1.name();
	int nameLength=node1Name.GetLength();
	int count=0;
	count+=(nameLength+2);	//number of symbols to write the name of the main node

	//will mark whether the character is inside elements or not: 1-outside, 0-inside
	markup.ResizeArray(textLength,true);
	markup=0;
	
	xml_node node2=node1.first_child();
	BString curString;
	while(node2)
	{
		XmlToString(node2,curString);
		int curLength=curString.GetLength();

		int nextCount=count+curLength;
		if(node2.type()==node_pcdata)
		{
			for(int i=count;i<nextCount;i++)
			{
				markup[i]=1;
			}
		}

		count=nextCount;
		node2=node2.next_sibling();
	}	//markup is done
}

//For a section, ensure that lists are in section-level <list> elements
//Renames paragraphs with listEls into <list> elements
//Non-recursive, operates on top-level paragraphs only (all paragraphs are top-level in the section)
void CWikipediaParser::MoveListElToLists(xml_document& section)
{
	//the root node is <firstPara>, <secTitle> or <secContent>, and there are only paragraphs inside of them
	xml_node secNode=section.first_child();
	xml_node curPara=secNode.first_child();
	xml_node curElement;	//current element in the paragraph, can be a <listEl> or something else

	while(curPara)	//iterate over paragraphs
	{
		curElement=curPara.first_child();

		bool fListElFirst;		//whether the first element is listEl or not
		if(BString(curElement.name())=="listEl") fListElFirst=true;
		else fListElFirst=false;

		if(fListElFirst) curPara.set_name("list");

		curElement=curElement.next_sibling();

		while(curElement)	//iterate over elements in the paragraph
		{
			bool fListElCur;
			if(BString(curElement.name())=="listEl") fListElCur=true;
			else fListElCur=false;
			
			if(fListElFirst==fListElCur)	//This element is of the same type, continue to the next element
			{
				curElement=curElement.next_sibling();
				continue;
			}
			else	//listEl-not listEl mismatch
			{
				//create a new par after the current one
				xml_node newPara=secNode.insert_child_after("par",curPara);
				xml_node remNode;

				//and move all remaining elements into it
				while(curElement)
				{
					newPara.append_copy(curElement);
					remNode=curElement;
					curElement=curElement.next_sibling();
					curPara.remove_child(remNode);
				}
			}
		}

		curPara=curPara.next_sibling();
	}
}



bool CWikipediaParser::IsListSymbol(char symbol)
{
	if(symbol=='*' || symbol=='#' || symbol==':' || symbol==';') return true;
	else return false;
}

//Attempts to use HTML Tidy and remove <code>, <math> and <ref> tags
//Returns whether it was successful
//Leaves text unchanged if not successful
//Error prefix is used in reporting the errors - can be anything
bool CWikipediaParser::TidyAndClean(BString& text, const BString& errorPrefix)
{
	BString textCopy=text;
	textCopy="<wrap>"+textCopy+"</wrap>";

	textCopy.Replace("&lt;","<");
	textCopy.Replace("&gt;",">");
	textCopy.Replace("&quot;","\"");

	//Remove all kinds of <br>
	textCopy.Replace("<BR>","");
	textCopy.Replace("<br>","");
	textCopy.Replace("<BR />","");
	textCopy.Replace("<br />","");
	textCopy.Replace("</br>","");
	textCopy.Replace("</BR>","");

	//Remove center tags - half of them aren't closed!
	textCopy.Replace("<center>","");
	textCopy.Replace("</center>","");

	//Protect LF from HTML Tidy - HandleCRLF() must have been called first!
	textCopy.Replace("\x0A","xxLF");

	//Protect spaces next to < and > from Tidy - it eats them otherwise
	textCopy.Replace("> ",">xxSp");
	textCopy.Replace(" <","xxSp<");

	//Protect & by replacing it with &amp;
	//Tidy will choke on it otherwise
	textCopy.Replace("&","&amp;");

	//Run tidy
	int numWarnings, numErrors;
	bool tidyRes=HTMLtidyToXml(textCopy,numWarnings,numErrors);

	if(numErrors==0 && numWarnings==0 && tidyRes) AddError(errorPrefix+"success.");
	else
	{
		AddError(errorPrefix+"HTML Tidy errors or warnings.");
		return false;
	}

	//Remove all unnecessary CR and LF that tidy has inserted
	textCopy.Remove('\x0A');
	textCopy.Remove('\x0D');

	//Create pugi XML document
	xml_document tempDoc;
	if(!StringToXml(tempDoc,textCopy))
	{
		AddError(errorPrefix+"HTML tidy output could not be parsed by pugi.");
		return false;
	}

	int numRemoved;
	RemoveNodesByName(tempDoc,tagNamesForCleanup,numRemoved);
	XmlToString(tempDoc,textCopy);
	text=textCopy.Mid(6,textCopy.GetLength()-13);	//remove <wrap> and </wrap>

	text.Replace("xxLF","\x0A");	//Bring LF back
	text.Replace("xxSp"," ");		//bring back spaces
	RemoveAmpOnce(text);			//bring back ampersands
	return true;
}

bool CWikipediaParser::ParseArticle(BString& page, xml_document& output)
{
	//parses a page
	//receives a mediawiki-formatted string that starts with <page> and ends with </page>
	//the string is the XML for a page from a wikipedia dump

	//Note 1: For now, does not process <nowiki>, <pre> tags
	//Note 2: Lumps all kinds of lists and indentation together (LF + #*:;)

	//Replace &amp;nbsp; with space
	page.Replace("&amp;nbsp;"," ");

	//We will replace &amp; but we'll need to add it back and remove it again in the pieces
	//Processed by HTML tidy
	//Pugixml will parse standalone & without any problems
	page.Replace("&amp;","&");

	//Remove special words
	page.Replace("__NOTOC__","");
	page.Replace("__TOC__","");

	//Create an XML document for the page and add title
	xml_document doc;
	doc.append_child("page");

	//Set the current error map to the GeneralMap
	curErrorMap=&errorMapGeneral;
	AddError("Page parse started.");

	//Preprocess: extract title
	int pos1, pos2;
	pos1=page.Find("<title>",0);
	if(pos1>=0) pos2=page.Find("</title>",pos1);
	if(pos1==-1 && pos2==-1)
	{
		AddError("Critical page error: no <title> or </title> tags.");
		return false;
	}

	BString url=page.Mid(pos1+7,pos2-pos1-7);	//extract page url
	BString title=url;							//title is just another copy of the url - legacy code

	//Put url and title into the page XML
	doc.child("page").append_child("url").append_child(node_pcdata).set_value(url);
	doc.child("page").append_child("title").append_child(node_pcdata).set_value(title);

	//namespace of the page: -2=Media, -1=Special, 0-article, redirect, disambig, 10-template, etc.
	//-10 is the error return
	int nSpace=GetNamespace(page);
	if(nSpace==-10)	//error extracting namespace
	{
		CopyChildrenToNode(doc,output);
		return false;
	}

	//Only nSpace 0 and 10 (template) are processed further
	//If it is not an infobox template, ignore it by switching to an unused nSpace number
	BString pageType;
	if(nSpace==10)
	{
		pageType="template";
		if(url.Left(16)!="Template:Infobox") nSpace=50;
	}

	if(nSpace!=0 && nSpace!=10) pageType="other";
	
	//If the page is of the "unnecessary" type, add type and return
	if(pageType=="other")
	{
		doc.child("page").append_attribute("type").set_value(pageType);
		CopyChildrenToNode(doc,output);
		return true;
	}

	//By now, the page is an article, a redirect, a disambiguation or a template

	//Preprocess: extract text without any tags
	pos1=page.Find("<text",0);
	if(pos1>=0) pos2=page.Find("</text>",pos1);
	if(pos1==-1 || pos2==-1)
	{
		AddError("Critical page error: no <text> or </text> tags.");
		CopyChildrenToNode(doc,output);
		return false;
	}
	
	pos1=page.Find(">",pos1);pos1++;				//find closing brace for <text>
	BString text="\n"+page.Mid(pos1,pos2-pos1);		//insert leading CR and extract text

	//Process redirects
	//They are NOT cleaned with HTML Tidy first
	BString textLower=text;
	textLower.MakeLower();
	if( nSpace==0 && textLower.Find("#redirect")!=-1 )
	{
		//Set the correct error map
		curErrorMap=&errorMapRedirects;
		AddError("Redirect parse started.");

		pageType="redirect";
		doc.child("page").append_attribute("type").set_value(pageType);

		//Remove the # to avoid parsing it as a list element
		text.Remove('#');

		//Wrap the text and parse it as if it was a section
		text="<text>"+text+"</text>";
		xml_document parsed;
		bool fSuccess=ParseSection(text,parsed,true);
				
		BString redirectTarget=GetNodeByName(parsed,"link").child("target").first_child().value();
		if(!fSuccess || redirectTarget=="") {AddError("Could not parse a redirect page.");return false;}
	
		doc.child("page").append_attribute("target").set_value(redirectTarget);
		CopyChildrenToNode(doc,output);
		return true;
	}

	//Replace all CR-LF with LF
	//and limit all LF runs to 2
	HandleCRLF(text);

	//Remove comments
	//Comments after section headings interfere with sectioning
	common.RemoveBracketedByStrings(text,"&lt;!--","--&gt;");

	//By now, the page is either a template or article/disambig
	//Set the correct error map
	if(pageType=="template")
	{
		curErrorMap=&errorMapTemplates;
		AddError("Infobox template parse started");
	}
	else
	{
		curErrorMap=&errorMapArtDisambigs;
		AddError("Article or disambig parse started.");
	}

	//Attempt to clean the text with HTML Tidy and
	//remove <ref>, <math>, <code> etc. tags and their content
	//fCleaned holds the result, on failure we'll try to clean each section individually
	//and if that fails, the cleaning will be done at node level
	bool fCleaned=TidyAndClean(text,"Full page cleanup: ");
	int textLength=text.GetLength();


	//Process templates
	if(pageType=="template")
	{
		if(!fCleaned){AddError("HTML Tidy errors or warnings in a template.");return false;}

		//Wrap the text and parse it as if it was a section
		text="<text>"+text+"</text>";
		xml_document parsed;
		if(!ParseSection(text,parsed,true)){AddError("Could not parse a template page.");return false;}

		doc.child("page").append_attribute("type").set_value(pageType);
		doc.child("page").append_copy(parsed.first_child());
		CopyChildrenToNode(doc,output);
		return true;
	}

	//By now, this is either an article or a disambiguation

	//find positions of section headings in text
	CBidirectionalMap<int> hBeginMap(2000);
	CHArray<int> hEnd(2000,true);
	CHArray<int> hLevel(2000,true);
	for(int level=2;level<7;level++)
	{
		BString equals="";
		for(int i=0;i<level;i++) equals+="=";

		BString leftMarker="\x0A"+equals;		//0a===
		BString rightMarker=equals+"\x0A";		//===0a

		int pos1;
		int pos2=0;
		while(1)					//find left markers
		{
			pos1=text.Find(leftMarker,pos2);
			if(pos1==-1) break;		//no more such left markers

			pos2=text.Find(rightMarker,pos1+1);
			int crPos=text.Find("\x0A",pos1+1);

			if(pos2==-1 || crPos<pos2)	//no corresponding right marker found
			{
				pos2=pos1+1; continue;
			}

			//At this point, satisfactory left and right markers are found
			int index=hBeginMap.AddWordGetIndex(pos1);
			hEnd[index]=pos2;
			hLevel[index]=level;		//will be set at the highest level
		}
	}
	int numSections=hBeginMap.GetNumPoints();
	hEnd.SetNumPoints(numSections);
	hLevel.SetNumPoints(numSections);

	//Compute where section text begins and ends
	//There are a total of numSections+1 text runs
	CHArray<int> breaks(2*(numSections+1));
	breaks.AddPoint(0);
	for(int i=0;i<numSections;i++)
	{
		breaks.AddPoint(hBeginMap[i]);
		breaks.AddPoint(hEnd[i]+hLevel[i]+1);
	}
	breaks.AddPoint(textLength);

	//Append text node
	xml_node textNode=doc.child("page").append_child("text");

	//Call ParseSection on each section heading and section text
	for(int i=0;i<(numSections+1);i++)
	{
		BString curString;
		xml_document parsed;

		if(i==0)	//if it is the first paragraph
		{
			int secLength=breaks[1]-breaks[0];
			if(secLength>0)
			{
				curString="<firstPara>"+text.Mid(breaks[0],breaks[1]-breaks[0])+"</firstPara>";
				if(ParseSection(curString,parsed,fCleaned))
				{
					textNode.append_copy(parsed.first_child());
					AddError("Section 0 parsed successfully.");
				}
				else
				{
					AddError("Section 0 discarded because of critical section error.");
				}
			}
		}
		else		//all other sections
		{
			xml_node secNode=textNode.append_child("section");
			xml_attribute attr=secNode.append_attribute("level");
			attr.set_value(hLevel[i-1]);

			int secLength=hEnd[i-1]-hBeginMap[i-1]-hLevel[i-1]-1;
			if(secLength>0)
			{
				BString curTitleString=text.Mid(hBeginMap[i-1]+hLevel[i-1]+1,secLength);
				curTitleString.Trim();

				curString="<secTitle>"+curTitleString+"</secTitle>";
				if(ParseSection(curString,parsed,fCleaned))
				{
					//Move all the children out of the <par> node in <secTitle>
                    xml_node temp1 = parsed.first_child().first_child();
                    xml_node temp2 = parsed.first_child();
                    CopyChildrenToNode(temp1,temp2);

					parsed.first_child().remove_child(parsed.first_child().first_child());
					//Put the secTitle into the document structure
					secNode.append_copy(parsed.first_child());
				}
				else
				{
					AddError("Section title discarded because of critical error.");
				}
			}

			secLength=breaks[2*i+1]-breaks[2*i];
			if(secLength>0)
			{
				curString="<secContent>"+text.Mid(breaks[2*i]-1,secLength+1)+"</secContent>";		//-1, +1 to capture the leading LF of the section
				if(ParseSection(curString,parsed,fCleaned))
				{
					secNode.append_copy(parsed.first_child());
					AddError("Section (not 0) parsed successfully.");
				}
				else
				{
					AddError("Section (not 0) discarded because of critical section error.");
				}
			}
		}
	}

	//Create the correct tree out of sections
	xml_node rootNode=doc.first_child().child("text");
	xml_node secNode=rootNode.first_child();
	CHArray<xml_node> lastNodes(6); //stack for levels 2,3,4,5,6
	while(secNode)
	{
		if(BString(secNode.name())=="section")
		{
			//If this section does not have a content node, add it
			xml_node contNode=secNode.child("secContent");
			if(!contNode) contNode=secNode.append_child("secContent");

			int secLevel=secNode.attribute("level").as_int();
			
			while(!lastNodes.IsEmpty() &&
					lastNodes[lastNodes.GetNumPoints()-1].attribute("level").as_int() >= secLevel)
			{
				lastNodes.Pop();
			}

			if(!lastNodes.IsEmpty())	//this section gets attached to content of another section
			{
				xml_node newNode=lastNodes[lastNodes.GetNumPoints()-1].child("secContent").append_copy(secNode);
				lastNodes.Push(newNode);
				xml_node del=secNode;
				secNode=secNode.next_sibling();
				rootNode.remove_child(del);
			}
			else			//This section remains in the root node
			{
				lastNodes.Push(secNode);
				secNode=secNode.next_sibling();
			}
		}
		else	//not a section - it's the first paragraph, probably
		{
			secNode=secNode.next_sibling();
		}
	}

	//Check if this is a disambiguation or an article and set page type in XML
	if(IsDisambiguationPage(doc)) pageType="disambig";
	else pageType="article";
	doc.child("page").append_attribute("type").set_value(pageType);

	//Check whether this is a list page
	//Disambiguation pages are not considered list pages
	if(IsListPage(doc))
	{
		doc.child("page").append_attribute("list").set_value("yes");
	}

	//Postprocessing
	CopyChildrenToNode(doc,output);

	RemoveLeadingLists(output);
	ProcessGalleryTags(output);
	ProcessSpecialTemplates(output);
	ConditionalRemoveNodes1(output);
	MoveImagesToEndOfSections(output);

	return true;
}

//Converts all <gallery> tags to gallery templates
void CWikipediaParser::ProcessGalleryTags(xml_node& node)
{
	//Find galleries
	CHArray<xml_node> galleries(10);
	GetNodesByName(node,"gallery",galleries);

	//This will convert the gallery tags to Template:Gallery
	//And call TemplateGallery() on them
	//Will delete the <gallery> nodes on failure
	for(int i=0;i<galleries.Count();i++) GalleryTagToTemplate(galleries[i]);

	//If some gallery tags remain, this is due to parse error
	//remove them
	int numRemoved=0;
	RemoveChildrenByName(node,"gallery",numRemoved);
}

//Converts a <gallery> tag to gallery template
void CWikipediaParser::GalleryTagToTemplate(xml_node& galleryNode)
{
	//We'll make a Template:Gallery out of this node and call TemplateGallery() on it
	BString string;
	XmlToString(galleryNode,string);

	int pos1=string.Find(">",0);
	int pos2=string.Find("</gallery>", pos1);
	if(pos1==-1 || pos2==-1) return;

	BString content=string.Mid(pos1+1,pos2-pos1-1);
	content.Replace("File:","|File:");
	content.Replace("Image:","|File:");
	content.Replace("file:","|File:");
	content.Replace("image:","|File:");
	content.Replace("||","|");
	content.Replace("|","</param><param>");

	content="<text><param>"+content+"</param></text>";
	xml_document galleryDoc;
	if(!StringToXml(galleryDoc,content))
	{
		AddError("Error parsing parameter XML in a <gallery> tag.");
		return;
	}

	//Clear the gallery tag, rename it to template, and place all the content in it
	galleryNode.set_name("template");
	RemoveAllChildren(galleryNode);
	RemoveAllAttributes(galleryNode);
	galleryNode.append_child("target").append_child(node_pcdata).set_value("gallery");

    xml_node temp = galleryDoc.first_child();
    CopyChildrenToNode(temp,galleryNode);
}

//Removes leading <list> element from <firstPara>
//If it exists, it is a notice
//Will remove <list> in front of any printable paragraph
void CWikipediaParser::RemoveLeadingLists(xml_document& doc)
{
	xml_node curNode=doc.child("page").child("text").child("firstPara").first_child();
	
	while(curNode)
	{
		BString name=curNode.name();

		if(name=="par")
		{
			BString content;
			WriteContentToString(curNode,content);
			content.Trim();

			//this is a printable paragraph, we encountered it and should stop deleting lists
			if(content!="") return;
		}

		if(name=="list") curNode.parent().remove_child(curNode);

		curNode=curNode.next_sibling();
	}
}

//Places all images in each section after the last text paragraph or list
//Does nothing with the text in subsections
void CWikipediaParser::MoveImagesToEndOfSections(xml_document& doc)
{
	xml_node textNode=doc.child("page").child("text");
	if(!textNode) return;

	//Call MoveImagesToEnd() on each section content, including firstPara
    xml_node firstPara = textNode.child("firstPara");
    MoveImagesToEnd(firstPara);

	CHArray<xml_node> sections(50);
	GetNodesByName(textNode,"section",sections);
    for(int i=0;i<sections.Count();i++)
    {
        xml_node temp = sections[i].child("secContent");
        MoveImagesToEnd(temp);
    }
}

//moves template param names into the xml parameter, <param n="caption">blah</param>
void CWikipediaParser::ParametrizeTemplate(xml_node& templateNode)
{
	//iterate over params
	xml_node curParam;
	for(curParam=templateNode.child("param"); curParam; curParam=curParam.next_sibling("param"))
	{
		xml_node stringNode=curParam.first_child();		//the leading string of the parameter body
		if(!stringNode || stringNode.type()!=node_pcdata) continue;

		BString string=stringNode.value();
		string.TrimLeft();

		int pos1=string.Find("=",0);
		if(pos1<=0) continue;

		CWordTrace word(string.Left(pos1).Trim().MakeLower());
		if(word.IsValidParameterName())		//If it qualifies, add an xml parameter value for parameter "n"
		{
			curParam.append_attribute("pn").set_value(word);
			string=string.Right(string.GetLength()-pos1-1);
			string.TrimLeft();
			stringNode.set_value(string);
		}
	}
}

//Called on the "content" node of a section
//Moves each image to the end of the content section
//into its own paragraph
void CWikipediaParser::MoveImagesToEnd(xml_node& contentNode)
{
	if(!contentNode) return;

	CHArray<xml_node> files(30);	//images

	xml_node curChild=contentNode.first_child();
	xml_node lastChild=contentNode.last_child();
	while(curChild)
	{
		BString name=curChild.name();
		if(name=="par" || name=="list")
		{
			GetNodesByNameExceptIn(curChild,"file","template",files);
			for(int i=0;i<files.Count();i++)
			{
				//Append a paragraph and place the image in it
				contentNode.append_child("par").append_copy(files[i]);
			}

			int numRemoved;
			//Remove the images from the current child node - if they were moved to the end of section
			if(files.Count()>0) RemoveNodesByNameIfPresent(curChild,"file",files,numRemoved);
		}

		if(curChild==lastChild) break;
		curChild=curChild.next_sibling();
	}

	RemoveEmptyParChildren(contentNode,false);
}

//Whether the page is a list page - list, set index, date, year, etc.
bool CWikipediaParser::IsListPage(xml_document& doc)
{
	//Test the title with regexes
	BString title=doc.child("page").child("title").first_child().value();

	//Regex to check the title for list pages
	RE2 re(
	"(^(List of|Lists of|Outline of|Glossary of|Timeline of|Timeline for|Index of) .*)|"
	"(^National Register of .*)|"
	"(^\\d{2}th century in .*)|"
	"(^\\d{4} New Year Honours$)|"
	"(^\\d{4} Birthday Honours$)|"
	"(^\\d{4} in .*)|"			//Check for "2001 in sports", etc.
	"(^\\d{3}0s in .*)|"	//1960s in music
	"(^\\d{3}0s$)|"		//1960s
	"(^(January|February|March|April|May|June|July|August|September|October|November|December) \\d{4} in .*)|" //Check for "June 2011 in sports", etc.
	"(^\\d{3,4}$)|"	//Check for 1945, etc.
	"(.* at the \\d{4} (Summer|Winter) Olympics$)"	//Check for things like "Boxing at the 1998 Summer Olympics", etc.
	);
	if(RE2::FullMatch((const char*)title,re)) return true;

	//Check the templates
	CHArray<xml_node> templates(200);
	GetNodesByName(doc,"template",templates);

	for(int i=0;i<templates.Count();i++)
	{
		BString name=templates[i].child("target").first_child().value();
		name.MakeLower();
		if(name=="set index" || name=="sia" || name=="set index article") return true;
		if(name=="months" || name=="yearbox" || name=="events by month links") return true;

		if(name.Left(9)=="years in ") return true;
	}

	return false;
}

//Extracts namespace from the string with the page data
//0 - article, disambig, redirect, 10-template, etc.
int CWikipediaParser::GetNamespace(const BString& text)
{
	int pos1=text.Find("<ns>");
	int pos2=-1;
	if(pos1>=0) pos2=text.Find("</ns>",pos1);

	if(pos1==-1 || pos2==-1 || pos2<(pos1+5))
	{
		AddError("Critical page error: could not extract namespace.");
		return -10;
	}

	BString ns=text.Mid(pos1+4,pos2-pos1-4);
	return atoi(ns);
}

//Removes nodes for, interwiki links and categories
//Removes templates, except those listed in retainedTemplates
void CWikipediaParser::ConditionalRemoveNodes1(xml_node& node)
{
	xml_node child=node.first_child();
	while(child)
	{
		xml_node curChild=child;
		child=child.next_sibling();

		if(curChild.type()==node_element)
		{
			BString name=curChild.name();

			bool fRemove=false;

			//remove nodes based on name
			if(name=="interwiki" || name=="category" || name=="wTable" || name=="div")	fRemove=true;

			//Remove all templates, except those specified in retainedTemplates
			//and infobox templates
			if(name=="template")
			{
				BString target=curChild.child("target").first_child().value();
				target.MakeLower();
				if( ! retainedTemplates.IsPresent(target) && target.Left(7)!="infobox") fRemove=true;
			}

			//remove nodes based on section name
			if(name=="section")
			{
				BString secTitle=curChild.child("secTitle").first_child().value();

				if(secTitle=="References" || secTitle=="External links" || secTitle=="Bibliography" || secTitle=="Footnotes" ||
					secTitle=="Further reading" || secTitle=="Notes") fRemove=true;
			}

			if(fRemove) node.remove_child(curChild);	//Remove the child if necessary
			else ConditionalRemoveNodes1(curChild);		//or place a recursive call if not
		}
	}
}

//Replaces all CR-LF with LF
//And limits all LF runs to a max of 2
void CWikipediaParser::HandleCRLF(BString& text)
{
	text.Replace("\x0D\x0A","\x0A");

	//Replace all remaining CR with LF - there should be none
	text.Replace('\x0D','\x0A');

	//Remove runs greater than 2 LFs
	int length=text.GetLength();

	CHArray<char> buffer(length+1);
	int crs=0;
	for(int i=0;i<length;i++)
	{
		char symbol=text[i];

		if(symbol=='\x0A')
		{
			if(crs<2) buffer.AddPoint(symbol);
			crs++;
		}
		else
		{
			crs=0;
			buffer.AddPoint(symbol);
		}
	}
	buffer.AddPoint(0);
	text=buffer.arr;

	return;
}



void CWikipediaParser::ParseLinks(xml_node& node)
{
	//if it is an external link
	if(BString(node.name())=="extLink")
	{
		//First, add element for the target
		xml_node curNodeToExpand=node.append_child("target");
		bool fSpaceFound=false;
	
		xml_node child=node.first_child();
		while(child && BString(child.name())!="target")
		{
			if(child.type()!=node_pcdata && !fSpaceFound) //it is not text
			{
				curNodeToExpand.append_copy(child);
			}
			else		//it is text
			{
				BString value(child.value());
								
				int pos1=value.Find(' ');
				if(pos1==-1) //No space is found
				{
					curNodeToExpand.append_copy(child);
				}
				else	//space found - everything else is going to be anchor text
				{
					fSpaceFound=true;
					xml_node newNode=curNodeToExpand.append_child(node_pcdata);
					newNode.set_value(value.Mid(0,pos1));
					curNodeToExpand=node.append_child("anchor");
					newNode=curNodeToExpand.append_child(node_pcdata);
					newNode.set_value(value.Mid(pos1+1,value.GetLength()-pos1));
				}
			}
			child=child.next_sibling();

		}//end iterate over children of extLink

		//If there is no anchor, create the anchor by copying the target
		xml_node target=node.child("target");
		xml_node anchor=node.child("anchor");
		if(!anchor)	//no anchor node
		{
			anchor=node.append_copy(target);
			anchor.set_name("anchor");
		}

		child=child.next_sibling();

		//remove all child nodes except those added
		RemoveUpToTarget(node);
	}//end if extLink

	//if it is an internal link
	if(BString(node.name())=="link")
	{
		xml_node curNodeToExpand=node.append_child("target");
		int numPipesSeen=0;
	
		//iterate over children
		xml_node child=node.first_child();
		while(child && BString(child.name())!="target")
		{
			if(child.type()!=node_pcdata) //it is not text
			{
				curNodeToExpand.append_copy(child);
			}
			else		//it is text
			{
				BString value(child.value());
								
				int pos1=0;
				int pos2=0;
				while((pos2=value.Find('|',pos1))!=-1)
				{
					BString curText=value.Mid(pos1,pos2-pos1);
					pos1=pos2+1;
					numPipesSeen++;
			
					if(curText!="")
					{
						xml_node curTextNode=curNodeToExpand.append_child(node_pcdata);
						curTextNode.set_value(curText);
					}
					curNodeToExpand=node.append_child("param");
				}
				BString curText=value.Mid(pos1,value.GetLength()-pos1);
				if(curText!="")
				{
					xml_node curTextNode=curNodeToExpand.append_child(node_pcdata);
					curTextNode.set_value(curText);
				}
			}
			child=child.next_sibling();

		}//end iterate over children of link

		//Remove everything that isn't in the newly created elements
		RemoveUpToTarget(node);

		//Trim target and capitalize the first letter
		xml_node targetText=node.child("target").first_child();
		BString temp=targetText.value();
		temp.Trim();
		BString targetWithoutCap=temp;			//Tjis is uncapitalized target - we may need it for the anchor!
		CapitalizeFirstLetter(temp);
		targetText.set_value(temp);
		
		//Check whether this is a File, Image, Media, Category, or an interwiki link
		bool fTypeFound=false;
		bool fPrecColon=false;	//whether the target text is preceded by a colon
		if(targetText.type()==node_pcdata)
		{
			BString string=targetText.value();
			string.Trim();
			if(!string.IsEmpty() && string[0]==':')
			{
				string=string.Right(string.GetLength()-1);
				targetText.set_value(string);
				fPrecColon=true;
			}
			if(!fTypeFound && (string.Left(5).MakeLower()=="file:" || string.Left(6).MakeLower()=="image:"))
			{
				node.set_name("file");
				fTypeFound=true;

				SetFileTarget(node,string);

				//Rename the last param to "caption", or add one
				//Captions to pre-existing file links are added here
				//Captions for <file> created from templates and galleries are added by SetFileCaption()
				xml_node last=node.last_child();
				if(last && BString(last.name())=="param" && IsProperCaption(last)) last.set_name("caption");
				else node.append_child("caption");
			}
			if(!fTypeFound && string.Left(6)=="Media:")
			{
				node.set_name("media");
				fTypeFound=true;
			}
			if(!fTypeFound && string.Left(9)=="Category:")
			{
				node.set_name("category");
				fTypeFound=true;
			}
			if(!fTypeFound && IsInterwiki(string))
			{
				if(!fPrecColon) node.set_name("interwiki");
				fTypeFound=true;
			}
		}

		//if it is still an internal link, change the name of the first parameter to "anchor" (and add it if it does not exist)
		if(BString(node.name())=="link")
		{
			xml_node targetNode=node.child("target");
			xml_node firstParam=targetNode.next_sibling();
			if(!firstParam)	//there is only the target, no anchor
			{
				firstParam=node.append_copy(node.child("target"));
				firstParam.first_child().set_value(targetWithoutCap);
			}

			//check whether the text after the link should be added to the anchor text
			xml_node afterTheLink=node.next_sibling();
			if(afterTheLink.type()==node_pcdata)
			{
				BString addedText="";
				BString value=afterTheLink.value();
				int valueLength=value.GetLength();

				if(valueLength > 0 && IsLetter(value[0]))
				{
					int i=0;
					while ( i<valueLength && IsLetter(value[i]) )
					{
						addedText+=value[i];
						i++;
					}

					xml_node anchor=firstParam.last_child();
					if(anchor.type()==node_pcdata)
					{
						BString newText=BString(anchor.value())+addedText;
						anchor.set_value(newText);
					}

					if(i>0) afterTheLink.set_value(value.Right(valueLength-i));
				}
			}

			firstParam.set_name("anchor");

			//Fix target text and
			//If there is a # in the target, split out the <section>
			xml_node targetTextNode=targetNode.first_child();
			BString targetText=targetTextNode.value();
			BString tPage, tSection;

			FixAndSplitTarget(targetText,tPage,tSection);
			targetTextNode.set_value(tPage);

			if(tSection!="")
			{
				xml_node sectionNode=node.insert_child_after("tSection",targetNode);
				sectionNode.append_child(node_pcdata).set_value(tSection);
			}
		}
	}//end if internal link

	xml_node child=node.first_child();
	while(child)
	{
		ParseLinks(child);
		child=child.next_sibling();
	}
}

bool CWikipediaParser::IsProperCaption(xml_node& paramNode)
{
	BString caption=paramNode.first_child().value();

	if(caption.Left(4)=="alt=" ||
		caption.Left(5)=="link=" ||
		caption=="left" ||
		caption=="right" ||
		caption=="center" ||
		caption=="centre" ||
		caption=="upright" ||
		caption=="thumb") return false;

	//Check if it ends in #px e.g. 23px
	int length=caption.GetLength();
	if(length>2)
	{
		if(caption.Right(2)=="px" && common.IsNumber(caption[length-3])) return false;
	}

	return true;
}

//Replace everything in the <par> and <listEl> nodes with their printed contents
//And remove and empty <par> and <listEl>
//Used in creating simplified XML structure for DizzySearcher
void CWikipediaParser::ShortenXml(xml_node& node)
{
	ApplyToElementOrDocTree(node,boost::bind(&CWikipediaParser::ShortenXmlHelper,this,_1));

	RemoveEmptyParAndListEl(node,true);
}

void CWikipediaParser::ShortenXmlHelper(xml_node& node)
{
	BString name=node.name();

	if(name=="par" || name=="listEl" || name=="secTitle")
	{
		ReplaceChildrenWithContent(node,false);		//Not including image captions
	}
}

//Removes empty paragraphs and list elements from XML
void CWikipediaParser::RemoveEmptyParAndListEl(xml_node& node, bool fAssumePcdataOnly)
{
	ApplyToElementOrDocTree(node,boost::bind(&CWikipediaParser::RemoveEmptyParChildren,this,_1,fAssumePcdataOnly));
}

//Removes the children that are empty <par> and <listEl> nodes
void CWikipediaParser::RemoveEmptyParChildren(xml_node& node, bool fAssumePcdataOnly)
{
	xml_node child=node.first_child();

	while(child)
	{
		xml_node curNode=child;
		child=child.next_sibling();

		BString curName=curNode.name();
		if(curName=="par" || curName=="listEl")
		{
			xml_node fChild=curNode.first_child();
			if(!fChild)
			{
				node.remove_child(curNode);
				continue;
			}

			if(fAssumePcdataOnly && fChild.type()==node_pcdata)
			{
				BString val=fChild.value();
				val.Trim();
				if(val=="") node.remove_child(curNode);
			}
		}
	}
}

//Prints the content of the node,
//Deletes all children, and places the content in the node as node_pcdata
void CWikipediaParser::ReplaceChildrenWithContent(xml_node& node, bool fIncludeImCaptions/*=true*/)
{
	BString content;
	WriteContentToString(node,content,fIncludeImCaptions);
	RemoveAllChildren(node);
	node.append_child(node_pcdata).set_value(content);
}

//Fixes the first letter of a link target,
//Replaces "_" with spaces
//And separates the section part after the "#" into tSection
void CWikipediaParser::FixAndSplitTarget(BString& target, BString& tPage, BString& tSection)
{
	target.Replace('_',' ');
	CapitalizeFirstLetter(target);
	
	int pos=target.Find("#",0);
	if(pos==-1)
	{
		tPage=target;
		tSection="";
	}
	else
	{
		int length=target.GetLength();
		tPage=target.Left(pos);
		tSection=target.Right(length-pos-1);
	}
	return;
}

void CWikipediaParser::CapitalizeFirstLetter(BString& string)
{
	int length = string.GetLength();
	if (length == 0) return;

	BString firstLetter=string.Left(1);
	firstLetter.MakeUpper();
	string=firstLetter+string.Right(length-1);
	return;
}

bool CWikipediaParser::IsLetter(char symbol)
{
	return ((symbol>96 && symbol<123) || (symbol>64 && symbol<91));
}

bool CWikipediaParser::IsInterwiki(BString& target)
{
	int pos=target.Find(':',0);
	if(pos==-1) return false;
	else
	{
		BString prefix=target.Left(pos);
		if(languageMap.IsPresent(prefix)) return true;
		else return false;
	}
}

//Recursive function that transforms some templates into readable form
//Processes "convert", "lang" templates, etc.
//Calls specialized non-recursive functions that begin with Template
void CWikipediaParser::ProcessSpecialTemplates(xml_node& node)
{
	xml_node child=node.first_child();
	while(child)
	{
		xml_node curNode=child;
		child=child.next_sibling();
		if(curNode.type()!=node_element) continue;

		bool fTemplFound=false;
		if(BString(curNode.name())=="template")		//if the current element is a template
		{
			BString target=curNode.child("target").first_child().value();
			target.MakeLower();

			//if it is a convert template
			if(!fTemplFound && target.Left(4)=="conv" && 
					(target=="convert" || target=="convert/2" || target=="convert/3" || target=="convert/4"))
			{fTemplFound=true;TemplateConvert(curNode);	}

			if(!fTemplFound && target.Left(5)=="lang-")
			{fTemplFound=true;TemplateLang(curNode);}

			if(!fTemplFound && target=="nihongo")
			{fTemplFound=true;TemplateNihongo(curNode);	}

			if(!fTemplFound && target=="double image")
			{fTemplFound=true;TemplateDoubleImage(curNode);}

			if(!fTemplFound && target=="triple image")
			{fTemplFound=true;TemplateTripleImage(curNode);}

			if(!fTemplFound && target=="multiple image")
			{fTemplFound=true;TemplateMultipleImage(curNode);}

			if(!fTemplFound && target=="gallery")
			{fTemplFound=true;TemplateGallery(curNode);	}

			if(!fTemplFound && target.Left(7).MakeLower()=="infobox")
			{fTemplFound=true;TemplateInfobox(curNode);}

			if(!fTemplFound && target=="quote")
			{fTemplFound=true;TemplateQuote(curNode);}

			if(!fTemplFound && target=="quotation")
			{fTemplFound=true;TemplateQuotation(curNode);}

			if(!fTemplFound && target=="bq")
			{fTemplFound=true;TemplateBq(curNode);}

			if(!fTemplFound && target=="centered pull quote")
			{fTemplFound=true;TemplateCenteredPullQuote(curNode);}

			if(!fTemplFound && target=="quote box")
			{fTemplFound=true;TemplateQuoteBox(curNode);}

			if(!fTemplFound && target=="quote box")
			{fTemplFound=true;TemplateQuoteBox(curNode);}

			if(!fTemplFound && target=="rquote")
			{fTemplFound=true;TemplateRQuote(curNode);}

			if(!fTemplFound && target=="nowrap")
			{fTemplFound=true;TemplateNowrap(curNode);}
		}

		if(!fTemplFound) ProcessSpecialTemplates(curNode);
	}

	return;
}

//Writes a quote from a quote template
//Inserts it before the insertBeforeNode
void CWikipediaParser::AddQuote(xml_node& quoteParam, xml_node& sourceParam,
								xml_node& insertBeforeNode)
{
	if(!quoteParam) return;

	//Insert a space before the quote
	insertBeforeNode.parent().insert_child_before(node_pcdata,insertBeforeNode).set_value(" ");
	CopyChildrenBefore(quoteParam,insertBeforeNode);	//Copy the quote
	//Insert a space after
	insertBeforeNode.parent().insert_child_before(node_pcdata,insertBeforeNode).set_value(" ");

	if(!sourceParam) return;

	CopyChildrenBefore(sourceParam,insertBeforeNode);	//Copy source
	//Another space
	insertBeforeNode.parent().insert_child_before(node_pcdata,insertBeforeNode).set_value(" ");
}

void CWikipediaParser::TemplateNowrap(xml_node& templateNode)
{
	ParametrizeTemplate(templateNode);

	xml_node param=templateNode.find_child_by_attribute("pn","1");
	if(!param) param=templateNode.child("param");
	if(!param) return;

	InsertChildrenBefore(param,templateNode);
}


void CWikipediaParser::TemplateQuote(xml_node& templateNode)
{
	ParametrizeTemplate(templateNode);

	xml_node quoteParam=templateNode.find_child_by_attribute("pn","text");
	if(!quoteParam) quoteParam=templateNode.find_child_by_attribute("pn","1");
	if(!quoteParam) quoteParam=templateNode.child("param");

	xml_node sourceParam=templateNode.find_child_by_attribute("pn","sign");
	if(!sourceParam) sourceParam=templateNode.find_child_by_attribute("pn","2");
	if(!sourceParam) sourceParam=quoteParam.next_sibling("param");

	AddQuote(quoteParam,sourceParam,templateNode);
}

void CWikipediaParser::TemplateQuotation(xml_node& templateNode)
{
	ParametrizeTemplate(templateNode);

	//Let's try to find positional names first
	xml_node quoteParam=templateNode.find_child_by_attribute("pn","1");
	xml_node sourceParam=templateNode.find_child_by_attribute("pn","2");

	if(!quoteParam) quoteParam=templateNode.child("param");
	if(!sourceParam) sourceParam=quoteParam.next_sibling("param");
	AddQuote(quoteParam,sourceParam,templateNode);
}

void CWikipediaParser::TemplateRQuote(xml_node& templateNode)
{
	//No parametrizing necessary, the quote and the source are second and third params
	xml_node quoteParam=templateNode.child("param").next_sibling("param");
	xml_node sourceParam=quoteParam.next_sibling("param");

	AddQuote(quoteParam,sourceParam,templateNode);
}

void CWikipediaParser::TemplateBq(xml_node& templateNode)
{
	ParametrizeTemplate(templateNode);

	//Three different quote markings
	xml_node quoteParam=templateNode.find_child_by_attribute("pn","text");
	if(!quoteParam) quoteParam=templateNode.find_child_by_attribute("pn","quote");
	if(!quoteParam) quoteParam=templateNode.find_child_by_attribute("pn","1");
	if(!quoteParam) return;

	//A lot of different source markings
	xml_node sourceParam=templateNode.find_child_by_attribute("pn","2");
	if(!sourceParam) sourceParam=templateNode.find_child_by_attribute("pn","sign");
	if(!sourceParam) sourceParam=templateNode.find_child_by_attribute("pn","cite");
	if(!sourceParam) sourceParam=templateNode.find_child_by_attribute("pn","author");
	if(!sourceParam) sourceParam=templateNode.find_child_by_attribute("pn","by");

	AddQuote(quoteParam,sourceParam,templateNode);
}

void CWikipediaParser::TemplateCenteredPullQuote(xml_node& templateNode)
{
	ParametrizeTemplate(templateNode);

	xml_node quoteParam=templateNode.find_child_by_attribute("pn","1");
	if(!quoteParam) quoteParam=templateNode.child("param");		//Text is the first param, unless it's numbered

	xml_node sourceParam=templateNode.find_child_by_attribute("pn","author");

	AddQuote(quoteParam,sourceParam,templateNode);
}

void CWikipediaParser::TemplateQuoteBox(xml_node& templateNode)
{
	ParametrizeTemplate(templateNode);

	xml_node quoteParam=templateNode.find_child_by_attribute("pn","quote");
	xml_node sourceParam=templateNode.find_child_by_attribute("pn","source");
	AddQuote(quoteParam,sourceParam,templateNode);
}

//Creates a <file> from template parameters
//Such as from {{multiple image}}
//captionParam can be a null node
void CWikipediaParser::CreateFileFromParams(xml_node& imageParam, xml_node& captionParam,
                                            xml_node& insertBeforeNode)
{
	if(!imageParam || !insertBeforeNode) return;

	//if there is a file param, we can copy it in front of the insertBeforeNode
	xml_node fileNode=imageParam.child("file");
	if(fileNode)
	{
		fileNode=insertBeforeNode.parent().insert_copy_before(fileNode,insertBeforeNode);
		//Delete the imageParam, so that the file isn't found twice by the image search function
		imageParam.parent().remove_child(imageParam);
	}
	else
	{
		fileNode=insertBeforeNode.parent().insert_child_before("file",insertBeforeNode);
		BString target=imageParam.first_child().value();
		SetFileTarget(fileNode,target);
	}

	SetFileCaption(fileNode,captionParam);	//captionParam can be a null node
}

//Sets target on the <file> node
void CWikipediaParser::SetFileTarget(xml_node& fileNode, const BString& target)
{
	BString theTarget=target;
	theTarget.Trim();
	if(theTarget.Left(5).MakeLower()=="file:") theTarget=theTarget.Right(theTarget.GetLength()-5);
	if(theTarget.Left(6).MakeLower()=="image:") theTarget=theTarget.Right(theTarget.GetLength()-6);
	theTarget.Trim();
	CapitalizeFirstLetter(theTarget);
	theTarget="File:"+theTarget;

	//Add target node, if needed
	xml_node targetNode=fileNode.child("target");
	if(targetNode) RemoveAllChildren(targetNode);
	else targetNode=fileNode.prepend_child("target");

	targetNode.append_child(node_pcdata).set_value(theTarget);
}

//Sets a file caption on a <file> node
//From node contains the caption that needs to be placed into the fileNode
//If the from node is empty, it will leave the caption unchanged
void CWikipediaParser::SetFileCaption(xml_node& fileNode, xml_node& fromNode)
{
	xml_node captionNode;

	if(!fromNode)			//If there is no fromNode, we just check that <caption> exists and return
	{
		captionNode=fileNode.child("caption");
		if(!captionNode) captionNode=fileNode.append_child("caption");
		return;
	}

	captionNode=fileNode.child("caption");
	
	if(captionNode) RemoveAllChildren(captionNode);
	else captionNode=fileNode.append_child("caption");
		
	CopyChildrenToNode(fromNode,captionNode);
}

void CWikipediaParser::TemplateInfobox(xml_node& templateNode)
{
	//For now, only images are extracted from the infoboxes
	//And all <file> nodes in them are deleted
	//Infoboxes without params are ignored, e.g. {{Infobox tantalum}}
	//They refer to standalone templates and there are only ~1,200 of them
	if(!templateNode.child("param")) return;

	ParametrizeTemplate(templateNode);

	//Only one (first) image and caption each are extracted from the infobox 
	xml_node imageParam;
	xml_node captionParam;
	for(xml_node curParam=templateNode.child("param"); curParam; curParam=curParam.next_sibling("param"))
	{
		if(imageParam && captionParam) break;

		xml_attribute attrib=curParam.attribute("pn");	//parameter name
		if(!attrib) continue;
		
		BString string=attrib.value();

		if(!imageParam && infoImageMarkers.IsPresent(string)) {imageParam=curParam;continue;};
		if(!captionParam && infoCaptionMarkers.IsPresent(string)) {captionParam=curParam;continue;};
	}

	if(imageParam) CreateFileFromParams(imageParam, captionParam, templateNode);

	//Delete all <file> nodes remaining in the infobox
	int numRemoved=0;
	RemoveChildrenByName(templateNode,"file",numRemoved);
}

//Image template processing
void CWikipediaParser::TemplateDoubleImage(xml_node& templateNode)
{
	//{{double image | blah | Left image.jpg | Left image size | Right image.jpg | Right image size |
	//  Left caption | Right caption | Left alt text | Right alt text}}

	xml_node leftImNode=templateNode.child("param").next_sibling();
	xml_node rightImNode=leftImNode.next_sibling().next_sibling();
	xml_node leftCaptionNode=rightImNode.next_sibling().next_sibling();
	xml_node rightCaptionNode=leftCaptionNode.next_sibling();

	if(!leftImNode || !rightImNode)
	{
		AddError("Error in a \"Double image\" template");
		return;
	}

	CreateFileFromParams(leftImNode,leftCaptionNode,templateNode);
	CreateFileFromParams(rightImNode,rightCaptionNode,templateNode);
}

void CWikipediaParser::TemplateTripleImage(xml_node& templateNode)
{
	//{{triple image | blah | Left image.jpg | Left image size | 
	// Center image.jpg | Center image size |
	// Right image.jpg | Right image size |
	//  Left caption | Center caption |Right caption | Left alt text | Right alt text}}

	xml_node leftImNode=templateNode.child("param").next_sibling();
	xml_node centerImNode=leftImNode.next_sibling().next_sibling();
	xml_node rightImNode=centerImNode.next_sibling().next_sibling();
	xml_node leftCaptionNode=rightImNode.next_sibling().next_sibling();
	xml_node centerCaptionNode=leftCaptionNode.next_sibling();
	xml_node rightCaptionNode=centerCaptionNode.next_sibling();

	if(!leftImNode || !centerImNode || !rightImNode)
	{
		AddError("Error in a \"Triple image\" template");
		return;
	}

	CreateFileFromParams(leftImNode,leftCaptionNode,templateNode);
	CreateFileFromParams(centerImNode,centerCaptionNode,templateNode);
	CreateFileFromParams(rightImNode,rightCaptionNode,templateNode);
}

//Extracts images from the multiple image template
void CWikipediaParser::TemplateMultipleImage(xml_node& templateNode)
{
	//in arbitrary order, {{multiple image|image1=blah|image2=blah|caption1=blah|caption2=blah}}

	ParametrizeTemplate(templateNode);

	for(int i=1;true;i++)
	{
		BString imString, captionString;
		imString.Format("image%i",i);
		
		xml_node imParam=templateNode.find_child_by_attribute("pn",imString);
		if(!imParam) break;	// we break when there is no image string with number i

		captionString.Format("caption%i",i);
		xml_node capParam=templateNode.find_child_by_attribute("pn",captionString);
		CreateFileFromParams(imParam,capParam,templateNode);
	}
}

//Extracts images from Template:Gallery
//Also used to extract images from Template:Gallery created by ProcessGalleryTags()
//from <gallery> tags
void CWikipediaParser::TemplateGallery(xml_node& templateNode)
{
	//Lots of named params, but file names and captions are unnamed
	//File names begin with a File: or Image:, and the next unnamed
	//parameter is the caption

	ParametrizeTemplate(templateNode);

	//Delete all named params
	xml_node curChild=templateNode.child("param");
	while(curChild)
	{
		xml_node cur=curChild;
		curChild=curChild.next_sibling("param");
		if(cur.attribute("pn")) templateNode.remove_child(cur);
	}

	curChild=templateNode.child("param");
	while(curChild)
	{
		xml_node capNode;
		bool fContainsFile=ParamContainsFileString(curChild);
		bool fNextContainsFile=ParamContainsFileString(curChild.next_sibling("param"));

		if(fContainsFile)
		{
			if(curChild.next_sibling("param") && !fNextContainsFile)
			{
				//These two are a pair of params for a single <file> node
                xml_node temp = curChild.next_sibling("param");
                CreateFileFromParams(curChild,temp,templateNode);
				curChild=curChild.next_sibling("param").next_sibling("param");
			}
			else
			{
				CreateFileFromParams(curChild,capNode,templateNode);
				curChild=curChild.next_sibling("param");
			}
		}
		else curChild=curChild.next_sibling("param");
	}
}

//Whether the <param> node contains a string with File: or Image: in it
bool CWikipediaParser::ParamContainsFileString(const xml_node& paramNode)
{
	if(!paramNode) return false;
	BString string=paramNode.first_child().value();
	string.Trim();
	if(string=="") return false;

	string.MakeLower();
	if(string.Left(5)=="file:" || string.Left(6)=="image:") return true;
	else return false;
}

//Function to process Template:Nihongo
void CWikipediaParser::TemplateNihongo(xml_node& templateNode)
{
    xml_node paramChild = templateNode.child("param");
    InsertChildrenBefore(paramChild,templateNode);
	templateNode.parent().insert_child_before(node_pcdata,templateNode).set_value(" (");

    xml_node lastChild = templateNode.last_child();
    InsertChildrenBefore(lastChild,templateNode);
	templateNode.parent().insert_child_before(node_pcdata,templateNode).set_value(")");
}

//Non-recursive specialized function called by ProcessSpecialTemplates()
void CWikipediaParser::TemplateLang(xml_node& templateNode)
{
	xml_node contentNode=templateNode.first_child().next_sibling().first_child();

	if(!contentNode)
	{
		AddError("Non-critical parse error: \"lang-\" template could not be parsed.");
		return;
	}

	//Copy the contents out of the lang template
	while(contentNode)
	{
		templateNode.parent().insert_copy_before(contentNode,templateNode);
		contentNode=contentNode.next_sibling();
	}
}

//Non-recursive specialized function called by ProcessSpecialTemplates()
void CWikipediaParser::TemplateConvert(xml_node& templateNode)
{
	//iterate over all params of the template until one matches units
	BString newString="";

	xml_node param=templateNode.first_child().next_sibling();
	bool fUnitsFound=false;
	while(param)
	{
		BString paramText=param.first_child().value();
		paramText.Trim();
		if(convertMap.IsPresent(paramText))		//units are found
		{
			int index=convertMap.GetIndex(paramText);
			newString+=convertTable(1,index);
			fUnitsFound=true;
			break;
		}
		else	//these are not units
		{
			newString+=paramText;
			newString+=" ";
		}
		param=param.next_sibling();
	}

	if(fUnitsFound)	//convert template seems to have been parsed fully - replace the template with the newString
	{
		newString="<a>"+newString;
		newString+="</a>";

		xml_document insDoc;
		if(!StringToXml(insDoc,newString))
		{
			AddError("Non-critical error: XML parsing error while replacing a convert template.");
			return;
		}

		xml_node insChild=insDoc.first_child().first_child();
		while(insChild)
		{
			templateNode.parent().insert_copy_before(insChild,templateNode);
			insChild=insChild.next_sibling();
		}
	}
	else	//convert template could not be parsed
	{
		AddError("Non-critical error: convert template could not be parsed.");
	}
}

void CWikipediaParser::ParseTemplates(xml_node& node)
{
	//Recursive function called on all nodes to process <template> elements
	//to extract <templateName> and <parameter>s

	if(BString(node.name())=="template")
	{
		//First, add elements for template target and params
		xml_node curNodeToExpand=node.append_child("target");
	
		xml_node child=node.first_child();
		while(child && BString(child.name())!="target")
		{
			if(child.type()!=node_pcdata) //it is not text
			{
				curNodeToExpand.append_copy(child);
			}
			else		//it is text
			{
				BString value(child.value());
								
				int pos1=0;
				int pos2=0;
				while((pos2=value.Find('|',pos1))!=-1)
				{
					BString curText=value.Mid(pos1,pos2-pos1);
					curText.Trim();
					pos1=pos2+1;
			
					if(curText!="")
					{
						xml_node curTextNode=curNodeToExpand.append_child(node_pcdata);
						curTextNode.set_value(curText);
					}
					curNodeToExpand=node.append_child("param");
				}
				BString curText=value.Mid(pos1,value.GetLength()-pos1);
				if(curText!="")
				{
					xml_node curTextNode=curNodeToExpand.append_child(node_pcdata);
					curTextNode.set_value(curText);
				}
			}
			child=child.next_sibling();

		}//end iterate over children of template

		//Remove everything that isn't in the newly created elements
		RemoveUpToTarget(node);
		
	}//end if template*/

	xml_node child=node.first_child();
	while(child)
	{
		ParseTemplates(child);
		child=child.next_sibling();
	}
}

void CWikipediaParser::RemoveUpToTarget(xml_node& node)
{
	//removes all nodes until it sees the node named "target"
	//used by template and link-parsing functions

	xml_node nextSibling;
	xml_node child=node.first_child();
	while(child && BString(child.name())!="target")
	{
		nextSibling=child.next_sibling();
		node.remove_child(child);
		child=nextSibling;
	}
}

