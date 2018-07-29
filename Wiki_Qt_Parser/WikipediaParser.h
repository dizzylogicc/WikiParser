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

#include "SimpleXml.h"
#include "Common.h"
#include "Timer.h"
#include "Array.h"
#include "BidirectionalMap.h"
#include "CAIStrings.h"
#include "WikipediaParser.h"
#include "Savable.h"
#include "Matrix.h"

class CWikipediaParser : public Savable
{
public:
	CWikipediaParser(const BString& parserFolder, bool fEncodedFile = false);
	~CWikipediaParser(void){}

public:
	CCommon common;
	CTimer timer;

public:
	CBidirectionalMap<BString> languageMap;	//language prefixes - for interwiki links
	CMatrix<BString> convertTable;			//table for the "convert" template
	CBidirectionalMap<BString> convertMap;	//map made from the above table
	CBidirectionalMap<BString> imExtensionMap;	//Map of image extensions

	CBidirectionalMap<BString> retainedTemplates;	//Names of templates that are not removed after page parse - Infobox, Main, etc.
	CHArray<BString> tagNamesForCleanup;		//Names of tags to remove in cleanup - ref, math, code
	CHArray<BString> disambigTargets;			//Targets for various disambiguation templates
	CHArray<BString> skipInNodeCleanup;		//Element names that are skipped when nodes are cleaned up with Tidy individually
	CBidirectionalMap<BString> infoImageMarkers;	//Infobox image markers - "image", "skyline_image", etc.
	CBidirectionalMap<BString> infoCaptionMarkers;	//Infobox caption markers - "caption", "image_caption", etc.

public:
	//Error output - error maps for all pages, redirects, templates, and articles/disambigs
	CBidirectionalMap<BString>* curErrorMap;		//Pointer which indicates the map to which messages are currently directed
	CBidirectionalMap<BString> errorMapGeneral;
	CBidirectionalMap<BString> errorMapRedirects;
	CBidirectionalMap<BString> errorMapTemplates;
	CBidirectionalMap<BString> errorMapArtDisambigs;

    void AddError(const BString& error, int numTimes=1){BString temp(error);curErrorMap->AddWord(temp,numTimes);};
	void ClearErrorMaps(){errorMapGeneral.Clear();errorMapRedirects.Clear();errorMapTemplates.Clear();errorMapArtDisambigs.Clear();};
	void AppendErrorMaps(CWikipediaParser& otherParser)
	{
		errorMapGeneral.AddFromMap(otherParser.errorMapGeneral);
		errorMapRedirects.AddFromMap(otherParser.errorMapRedirects);
		errorMapTemplates.AddFromMap(otherParser.errorMapTemplates);
		errorMapArtDisambigs.AddFromMap(otherParser.errorMapArtDisambigs);
	};
	void WriteReport(std::ostream& report);

private:
	void WriteErrorMap(std::ostream& report, CBidirectionalMap<BString>& theErrorMap);

//Reading serialized and plain parser data
public:
	void Serialize(BArchive& archive);
	void ReadPlainParserData(const BString& folder);

public:
	//parses a page
	//receives a mediawiki-formatted string that starts with <page> and ends with </page>
	//the string is the XML for a page from a wikipedia dump
	//Splits the article into sections and tries to process each section separately
	//If there is an error in the section, that section is discarded
	bool ParseArticle(BString& page, xml_document& output);

	//Replace everything in the <par> and <listEl> nodes with their printed contents
	//And remove unprintable nodes and empty <par>
	//Used in creating simplified XML structure for DizzySearcher
	void ShortenXml(xml_node& node);

	//Prints the content of the node,
	//Deletes all children, and places the content in the node as node_pcdata
	void ReplaceChildrenWithContent(xml_node& node, bool fIncludeImCaptions=true);

	//Removes empty paragraphs from the XML
	void RemoveEmptyParAndListEl(xml_node& node, bool fAssumePcdataOnly=false);

	//moves template param names into the xml parameter, <param n="caption">blah</param>
	void ParametrizeTemplate(xml_node& templateNode);

private:
	//processes each section from an article
	//if it encounters an error, it quits and returns false
	//Called by ParseArticle on each section (separately on section title and section text)
	//fAlreadyCleaned specifies whether the page (and section) has already been cleaned with tidy
	//and references were removed
	//If not, cleaning will be performed
	bool ParseSection(const BString& theSection, xml_document& output, bool fAlreadyCleaned);

	//Parses braces in the provided string (text), such as left='{' and right='}'
	//Called by ParseSection
	//Stores opening position for each brace in openArr and closing position in closeArr
	//Stores the start position of each brace set in setStart, and its length (i.g., 3 for "{{{...}}}") in setLength
	//Markup is an array of the same length as text that replaces all opening braces with a positive integer
	//all closing braces with a negative integer, where integer is the number of braces in a set,
	//and all non-brace characters with zeros
	bool ParseBraces(const BString& text, char left, char right, CHArray<int>& openArr, CHArray<int>& closeArr,
							CHArray<int>& setStart, CHArray<int>& setLength,CHArray<char>& markup);

	//After the <template> tags are added, parse the pipes (|) to extract <templateName> and <parameter>s
	//Recursive function
	//Called by ParseSection
	void ParseTemplates(xml_node& node);

	//Recursive function that transforms some templates into readable form
	//Processes "convert", "lang" templates, etc.
	//Calls specialized non-recursive functions that begin with Template
	void ProcessSpecialTemplates(xml_node& node);

	//Inserts <listEl> tags into paragraphs - called by ParseSection()
	bool InsertListElInParagraph(xml_node& parNode);
	
	//Specialized non-recursive functions called by ProcessSpecialTemplates()
	void TemplateConvert(xml_node& templateNode);
	void TemplateNowrap(xml_node& templateNode);
	void TemplateLang(xml_node& templateNode);
	void TemplateNihongo(xml_node& templateNode);
	void TemplateDoubleImage(xml_node& templateNode);
	void TemplateTripleImage(xml_node& templateNode);
	void TemplateMultipleImage(xml_node& templateNode);
	void TemplateGallery(xml_node& templateNode);
	void TemplateInfobox(xml_node& templateNode);

	void TemplateQuote(xml_node& templateNode);
	void TemplateQuotation(xml_node& templateNode);
	void TemplateBq(xml_node& templateNode);
	void TemplateCenteredPullQuote(xml_node& templateNode);
	void TemplateQuoteBox(xml_node& templateNode);
	void TemplateRQuote(xml_node& templateNode);

	//After the <link> and <extLink> tags are added, parse the text in them
	//Called by ParseSection
	void ParseLinks(xml_node& node);

	//removes all nodes until it sees the node named "target"
	//used by ParseTemplates and ParseLinks
	void RemoveUpToTarget(xml_node& node);

	//Checks whether the target has an interwiki prefix
	bool IsInterwiki(BString& target);

	//checks whether the symbol is a letter
	bool IsLetter(char symbol);

	//checks whether the symbol is a list symbol (*#;:)
	bool IsListSymbol(char symbol);

	//Replaces all CR-LF with LF (10)
	//And limits all LF runs to a max of 2
	void HandleCRLF(BString& text);

	//Handles '', ''', ''''' markers
	void ProcessBoldItalic(BString& text);

	//Removes nodes for templates, interwiki links and categories
	void ConditionalRemoveNodes1(xml_node& node);

	//Looks for one of the disambiguation templates
	//In the tree structure and returns true if found
	//otherwise false
	bool IsDisambiguationPage(xml_node& node);

	//Determines whether the parsed page is a list page - 
	//List of, Index of, Outline of, Set index, date, year, etc.
	bool IsListPage(xml_document& doc);

	//Check whether the last param of a file node really is a caption
	bool IsProperCaption(xml_node& paramNode);

	//Fixes the first letter of a link target,
	//Replaces "_" with spaces
	//And separates the section part after the "#" into tSection
	void FixAndSplitTarget(BString& target, BString& tPage, BString& tSection);

	//Capitalizes the first letter of a string
	void CapitalizeFirstLetter(BString& string);

	//Clean individual section nodes with HTML Tidy - when the page cleanup and section cleanup has failed
    void TidyAndCleanNode(pugi::xml_node &node);

	//For a section, ensure that lists are in section-level <list> elements
	//Renames paragraphs with listEls into <list> elements
	//Non-recursive, operates on top-level paragraphs only (all paragraphs are top-level in the section)
	void MoveListElToLists(xml_document& section);

	//Extracts namespace from the string with the page data
	//0 - article, disambig, redirect, 10-template, -10 - error, anything else - "other"
	int GetNamespace(const BString& text);

	//Attempts to use HTML Tidy and remove <code>, <math> and <ref> tags
	//Returns whether it was successful
	//Leaves text unchanged if not successful
	//Error prefix is used in reporting the errors - can be anything
	bool TidyAndClean(BString& text, const BString& errorPrefix);

	//Creates the markup that shows whether the character is inside or outside of any elements
	//(excluding the head element)
	//1-outside, 0-inside
	//head node - <firstPara>, or <secTitle>, or <secContent> - must not have attributes
	void CreateElementOutsideMarkup(xml_document& doc, CHArray<char>& markup, int textLength);

	//Removes LF from the type-element children of the node 
    void RemoveLFfromChildElements(pugi::xml_node &node);

	//Helper for ShortenXml
	void ShortenXmlHelper(xml_node& node);

	//Removes the children that are empty <par> and <listEl> nodes
    void RemoveEmptyParChildren(pugi::xml_node &node, bool fAssumePcdataOnly);

	//Removes leading <list> elements from <firstPara>
	//Until a printable <par> is found
	void RemoveLeadingLists(xml_document& doc);

	//Places all images in each section after the last text paragraph or list
	//Does nothing with the text in subsections
	void MoveImagesToEndOfSections(xml_document& doc);

	//Called on the "content" node of a section
	//Moves each image to the end of the content section
	//into its own paragraph
    void MoveImagesToEnd(pugi::xml_node &contentNode);

	//Sets a file caption on a <file> node
	//From node contains the caption that needs to be placed into the fileNode
	//captionParam can be a null node
    void SetFileCaption(pugi::xml_node &fileNode, pugi::xml_node &fromNode);

	//Sets file tartget on a <file> node
	void SetFileTarget(xml_node& node, const BString& target);

	//Creates a <file> from template parameters
	//Such as from {{multiple image}}
    void CreateFileFromParams(pugi::xml_node &imageParam, pugi::xml_node &captionParam,
                                                pugi::xml_node &insertBeforeNode);

	//Whether the <param> node contains string with File: or Image: in it
    bool ParamContainsFileString(const pugi::xml_node &paramNode);

	//Extracts images from all <gallery> tags
	void ProcessGalleryTags(xml_node& node);

	//Extracts images from a single gallery
	void GalleryTagToTemplate(xml_node& galleryNode);

	//Writes a quote from a quote template
	//Inserts it before the insertBeforeNode
	void AddQuote(xml_node& quoteParam, xml_node& sourceParam,xml_node& insertBeforeNode);
};
