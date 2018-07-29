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

#ifdef _MFC_VER
	#include "stdafx.h"
#endif

#include "pugixml.hpp"
#include "Array.h"
#include "re2/re2.h"

using pugi::xml_parse_result;
using pugi::xml_node;
using pugi::xml_document;

//Namespace with common functions used throughout Dizzy

namespace DizzyUtility
{
	//Luhn check - for example, for credit card numbers
	//Returns true if Luhn check is correct, false otherwise
	bool LuhnCheck(const BString& digitString);

	//Extract numbers from string and return them in a separate string
	BString ExtractDigits(const BString& string);

	//Extracts the targets from all "Main" templates in a node
	void ExtractMainTemplateTargets(xml_node& node, CHArray<BString>& results);

	//Converts Wikipedia section names to URL-compatible notation
	BString UrlCovertWikiSection(const BString& secName);

	//Returns file extension
	BString GetExtension(const BString& fileName);

	//Transforms the title for URL
	BString UrlTransformTitle(const BString& title);

	//Returns the string with capitalized first letter
	BString CapitalizeFirstLetter(const BString& string);

	//Returns image mime type
	BString GetImageMimeType(const BString& target);

	//A simple validation for the URL
	bool IsURLvalid(const BString& url);

	//extracts the "server" part from the URL
	BString ExtractUrlRoot(const BString& url);

	//Get URL for a wiki page link
	BString URLforWikiPage(const BString& title);

	//Find wordbreak in string after the specified position
	//returns the index next after wordbreak - iterate to but not including this index!
	int FindWordbreakAfter(const BString& string, int pos);

	//Marks the parts of the string that match the regex in markup
	//1 - matching, 0 - not matching
	//Non-overlapping matching
	//Regex must match strings with length > 0
	//Returns number of matches
	int MarkWithRegex(const BString& string, const RE2& regex, CHArray<char>& markup);

	//Inserts insertAtStart and insertAtEnd pieces into target
	//Using markup - a char array initially sized the same as target with 1 and 0 elements
	//Whenever 0 changes to 1 (or if 1 begins the string), insertAtStart is inserted
	//When 1 changes to 0 (or if 1 terminates the string), insertAtEnd is inserted
	//Returns the number of start-end pairs inserted
	//Useful for XML tag insertion into a marked-up string
	int InsertUsingMarkup(BString& target, const CHArray<char>& markup,
		const BString& insertAtStart, const BString& insertAtEnd);

	//Mark terms with <b></b> in the string
	void MarkTermsWithBold(const CHArray<BString>& terms, BString& string);

	//Truncates a long string after a wordbreak after a certain number of symbols
	//And adds a <b>...</b> if truncation has been performed
	void TruncateAtWordbreak(int maxSymbols, BString& string);

	//Extracts article text from parsed mediaWiki in XML
	//Replaces html entities and "strange dash"
	void WriteContentToString(xml_node& node, BString& string, bool fIncludeImCaptions=true);

	//Internal function for content writer
    void InternalWriteContent(const pugi::xml_node &node, char* buffer, int& counter, bool fIncludeImCaptions=true);
	void InsertIntoWriteBuffer(char symbol, char* buffer, int& counter);	//Internal helper for content writer

	//converts some frequent html entities
	void ConvertHtmlEntities(BString& text);

	void RemoveAmpOnce(BString& string);	//Replaces &amp; with & once
	void RemoveAmp(BString& text);			//Replaces &amp; with & as many times as necessary
	void ReplaceStrangeDash(BString& string);

	//Regexes global to the namespace - not ideal, but allows to avoid recompiling them on every call
	extern RE2 wordRegex;	// Is simply "(\\w+)"
	extern RE2 urlRegex;	//for simple URL validation
};
