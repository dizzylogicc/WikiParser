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

#include "DizzyUtility.h"
#include "SimpleXml.h"
#include "Array.h"
#include "Common.h"

//Some regexes global to the namespace... Avoiding recompiling them on every use.
RE2 DizzyUtility::wordRegex("(\\w+)");
RE2 DizzyUtility::urlRegex(	"((https?):\\/\\/)?[\\w\\-_]+(\\.[\\w\\-_]+)+"
							"([\\w\\-\\.,@?^=%&:/~\\+#]*[\\w\\-\\@?^=%&/~\\+#])?");


//Extracts the targets from all "Main" templates in a node
void DizzyUtility::ExtractMainTemplateTargets(xml_node& node, CHArray<BString>& results)
{
	using namespace SimpleXml;
	CHArray<xml_node> mainTemplateNodes(32);

	GetNodesByNameChildValue(node,"template", "target", "main", true, mainTemplateNodes);

	for(int i=0;i<mainTemplateNodes.Count();i++)
	{
		BString pageTarget;
		xml_node curParam=mainTemplateNodes[i].child("param");

		while(curParam)
		{
			pageTarget=curParam.first_child().value();
			pageTarget.Trim();

			results.AddAndExtend(pageTarget);

			curParam=curParam.next_sibling("param");
		}
	}
}

//Luhn check - for example, for credit card numbers
//Returns true if Luhn check is correct
bool DizzyUtility::LuhnCheck(const BString& digitString)
{
	int length=digitString.GetLength();
	
	int sum=0;
	bool fDouble=false;
	for(int i=length-1;i>=0;i--)
	{
		int curDigit=digitString[i]-48;
		
		if(fDouble)
		{
			curDigit*=2;
			if(curDigit>9) curDigit-=9;
		}

		sum+=curDigit;
		fDouble=!fDouble;
	}

	return sum % 10 == 0;
}

//Extract numbers from string and return them in a separate string
BString DizzyUtility::ExtractDigits(const BString& string)
{
	BString result;
	int length=string.GetLength();
	for(int i=0;i<length;i++)
	{
		char cur=string[i];
		if(cur>='0' && cur<='9') result+=cur;
	}

	return result;
}

//Converts Wikipedia section names to URL-compatible notation
BString DizzyUtility::UrlCovertWikiSection(const BString& secName)
{
	BString result;

	int length=secName.GetLength();
	for(int i=0;i<length;i++)
	{
		char cur=secName[i];
		if(cur==' ') {result+='_';continue;}
		if(cur=='!') {result+=".21";continue;}
		if(cur=='\"') {result+=".22";continue;}
		if(cur=='#') {result+=".23";continue;}
		if(cur=='$') {result+=".24";continue;}
		if(cur=='%') {result+=".25";continue;}
		if(cur=='&') {result+=".26";continue;}
		if(cur=='\'') {result+=".27";continue;}
		if(cur=='(') {result+=".28";continue;}
		if(cur==')') {result+=".29";continue;}
		if(cur=='*') {result+=".2A";continue;}
		if(cur=='+') {result+=".2B";continue;}
		if(cur==',') {result+=".2C";continue;}
		if(cur=='/') {result+=".2F";continue;}
		if(cur==':') {result+=".3A";continue;}
		if(cur==';') {result+=".3B";continue;}
		if(cur=='=') {result+=".3D";continue;}
		if(cur=='?') {result+=".3F";continue;}
		if(cur=='@') {result+=".40";continue;}
		if(cur=='[') {result+=".5B";continue;}
		if(cur==']') {result+=".5D";continue;}

		result+=cur;
	}

	return UrlTransformTitle(result);
}

//Returns file extension
BString DizzyUtility::GetExtension(const BString& fileName)
{
	BString extension="";
	BString copy=fileName;
	copy.Trim();
	int length=copy.GetLength();

	int i;
	for(i=length-1;i>=0 && copy[i]!='.';i--)
	{
		extension=copy[i]+extension;
	}

	if(i<0) return "";
	else return extension;
}

//Transforms article title for URL
BString DizzyUtility::UrlTransformTitle(const BString& title)
{
	BString result=title;
	result.Trim();
	result.Replace(" ","_");
	result=CapitalizeFirstLetter(result);
	return result;
}

BString DizzyUtility::CapitalizeFirstLetter(const BString& string)
{
	BString result=string;
	BString firstLetter=result.Left(1);
	firstLetter.MakeUpper();
	int length=result.GetLength();
	result=firstLetter+result.Right(length-1);
	return result;
}

//Find wordbreak position in string after the specified position
//returns the index next after wordbreak - iterate to but not including this index!
int DizzyUtility::FindWordbreakAfter(const BString& string, int pos)
{
	int length=string.GetLength();
	if(pos>=length) return length;

	const char* stringStart=(const char*)string;
	re2::StringPiece result;

	if(RE2::PartialMatch(stringStart+pos,wordRegex,&result))
	{
		return (int)(result.data()-stringStart) + result.length();
	}
	else return string.GetLength();
}

//Marks the parts of the string that match the regex in markup
//1 - matching, 0 - not matching
//Non-overlapping matching
//Regex must match strings with length > 0
//Returns number of matches
int DizzyUtility::MarkWithRegex(const BString& string, const RE2& regex, CHArray<char>& markup)
{
	int length=string.GetLength();
	markup.ResizeIfSmaller(length);
	markup.SetNumPoints(length);
	markup=0;

	re2::StringPiece result;
	re2::StringPiece input((const char*)string);
	const char* inputStart=input.data();
	
	int numMatches=0;
	while(RE2::FindAndConsume(&input,regex,&result))
	{
		numMatches++;
		int resStart=(int)(result.data()-inputStart);
		int resEnd=resStart+result.length();

		for(int i=resStart;i<resEnd;i++) markup.arr[i]=1;
	}
	return numMatches;
}

//Mark terms with <b></b> in the string
void DizzyUtility::MarkTermsWithBold(const CHArray<BString>& terms, BString& string)
{
	CHArray<char> markup;
	for(int i=0;i<terms.Count();i++)
	{
		re2::StringPiece strPiece((const char*)terms[i]);
		std::string regexString=RE2::QuoteMeta(strPiece);

		//If the search term terminates in a word boundary, add word boundary to the regex
		//Otherwise omit the word boundary at the end
		if(RE2::FullMatch(regexString,".*\\b$")) regexString+="\\b)";
		else regexString+=")";
		RE2 regex((std::string("((?i)\\b")+regexString).c_str());

		MarkWithRegex(string,regex,markup);
		InsertUsingMarkup(string,markup,"<b>","</b>");
	}

	//Replace & - WT's xml parser will choke on it otherwise
	string.Replace("&","&amp;");
}

//Truncates a long string after a wordbreak after a certain number of symbols
//And adds a <b>...</b> if truncation has been performed
void DizzyUtility::TruncateAtWordbreak(int maxSymbols, BString& string)
{
	//Truncate the content at the word boundary after length limit, if needed
	if(string.GetLength()>maxSymbols)
	{
		string=string.Left(FindWordbreakAfter(string,maxSymbols));
		string+=" <b>...</b>";
	}
}

//Inserts insertAtStart and insertAtEnd pieces into target
//Using markup - a char array initially sized the same as target with 1 and 0 elements
//Whenever 0 changes to 1 (or if 1 begins the string), insertAtStart is inserted
//When 1 changes to 0 (or if 1 terminates the string), insertAtEnd is inserted
//Returns the number of start-end pairs inserted
//Useful for XML tag insertion into a marked-up string
int DizzyUtility::InsertUsingMarkup(BString& target, const CHArray<char>& markup,
						const BString& insertAtStart, const BString& insertAtEnd)
{
	//Do nothing on mismatched sizes or zero length
	if(markup.Count()!=target.GetLength() || markup.Count()==0) return 0;

	int length=target.GetLength();
	int startLength=insertAtStart.GetLength();
	int endLength=insertAtEnd.GetLength();

	//First pass - count the number of inserts
	//Count the number of 0-1 or 1-0 changes in the markup
	int numChanges=0;
	for(int i=1;i<markup.Count();i++) if(markup[i]!=markup[i-1]) numChanges++;
	if(markup[0]==1) numChanges++;					//Starting with 1 is also a change
	if(markup.Last()==1) numChanges++;	//Ending with 1 is also a change

	int numPairs=numChanges/2;
	int newSize = numPairs * (startLength + endLength) + 1;	//+1 for NULL termination
	CHArray<char> newString(newSize);

	if(markup[0]==1) newString.AddExFromString(insertAtStart);
	newString.AddAndExtend(target[0]);
	for(int i=1;i<length;i++)
	{
		if(markup[i]!=markup[i-1])
		{
			if(markup[i]==1) newString.AddExFromString(insertAtStart);
			else newString.AddExFromString(insertAtEnd);
		}
		newString.AddAndExtend(target[i]);
	}
	if(markup.Last()==1) newString.AddExFromString(insertAtEnd);
	newString.AddAndExtend(0);

	target=newString.arr;

	return numPairs;
}

//Get image mime type from extension
BString DizzyUtility::GetImageMimeType(const BString& target)
{
	BString ext=GetExtension(target);	//file extension
	
	if(ext=="jpg" || ext=="jpeg") return "image/jpeg";
	if(ext=="png" || ext=="svg") return "image/png";
	if(ext=="gif") return "image/gif";
	if(ext=="tif" || ext=="tiff") return "image/tiff";
	if(ext=="bmp") return "image/bmp";

	return "";
}

//A simple validation for the URL
bool DizzyUtility::IsURLvalid(const BString& url)
{
	return RE2::FullMatch((const char*)url, urlRegex);
}


//extracts the "server" part from the URL
BString DizzyUtility::ExtractUrlRoot(const BString& url)
{
	BString root=url;
	int urlLength=url.GetLength();

	if(url.Left(7).MakeLower()=="http://") root=url.Right(urlLength-7);
	if(url.Left(8).MakeLower()=="https://") root=url.Right(urlLength-8);

	int pos=root.FindOneOf("#?/");

	if(pos==-1) return root;

	root=root.Left(pos);
	return root;
}

//Get URL for a wiki page link
BString DizzyUtility::URLforWikiPage(const BString& title)
{
	return "https://en.wikipedia.org/wiki/"+UrlTransformTitle(title);
}

//Extracts article text from parsed mediaWiki in XML
//Replaces html entities and "strange dash"
void DizzyUtility::WriteContentToString(xml_node& node, BString& string, bool fIncludeImCaptions/*=true*/)
{
	//Uses InternalWriteContent, a double-pass XML writer function
	int numSymbols=0;
	InternalWriteContent(node,NULL,numSymbols,fIncludeImCaptions);
	CHArray<char> buffer(numSymbols+1,true);
	numSymbols=0;
	InternalWriteContent(node,buffer.arr,numSymbols,fIncludeImCaptions);
	buffer[numSymbols]=0;
	string=buffer.arr;

	ConvertHtmlEntities(string);
	ReplaceStrangeDash(string);
}

//Extracts article text from parsed mediaWiki in XML
void DizzyUtility::InternalWriteContent(const xml_node& node, char* buffer, int& counter, bool fIncludeImCaptions/*=true*/)
{
	using namespace pugi;

	//node_pcdata is printable
	if(node.type()==node_pcdata)
	{
		BString value=node.value();
		int length=value.GetLength();
		if(buffer!=NULL) memcpy(buffer+counter,value,length);
		counter+=length;
		return;
	}

	//if it is not an element node at this point, it is not printable
	if(node.type()!=node_element && node.type()!=node_document) return;
	BString name=node.name();

	//some element nodes are ignored completely
	if(name=="template" || name=="interwiki" || name=="wTable" || name=="media" || name=="category") return;
	if(name=="style" || name=="url") return;

	//if it is a link, print the anchor text
	if(name=="link" || name=="extLink") {InternalWriteContent(node.child("anchor"), buffer, counter, fIncludeImCaptions);return;}

	//if it is an image, print caption
	if(name=="file")
	{
		if(!fIncludeImCaptions) return;
		InternalWriteContent(node.child("caption"),buffer,counter, fIncludeImCaptions);
		if(counter!=0) InsertIntoWriteBuffer(' ', buffer, counter);
		return;
	}

	//all other types of nodes, iterate over the children
	xml_node child=node.first_child();
	while(child)
	{
		InternalWriteContent(child, buffer, counter, fIncludeImCaptions);
		child=child.next_sibling();
	}

	//If it was a par or a listEl, add a space at the end, unless the buffer is empty
	if(name=="par" || name=="listEl" || name=="title" || name=="secTitle")
	{
		if(counter!=0) InsertIntoWriteBuffer(' ', buffer, counter);
	}
}

//Private helper for content writer
void DizzyUtility::InsertIntoWriteBuffer(char symbol, char* buffer, int& counter)
{
	if(buffer!=NULL) {buffer[counter]=symbol;}
	counter++;
}

//converts some frequent html entities
void DizzyUtility::ConvertHtmlEntities(BString& text)
{
	RemoveAmp(text);
	text.Replace("&gt;",">");
	text.Replace("&lt;","<");

	text.Replace("&nbsp;"," ");
	text.Replace("&quot;","\"");
	text.Replace("&mdash;"," - ");
	text.Replace("&ndash;"," - ");
}

//Replaces &amp; with & once
void DizzyUtility::RemoveAmpOnce(BString& string)
{
	string.Replace("&amp;","&");
}

//Replaces &amp; with & as many times as necessary
void DizzyUtility::RemoveAmp(BString& text)
{
	while(text.Find("&amp;")!=-1) text.Replace("&amp;","&");
}

//Replaces strange dash
void DizzyUtility::ReplaceStrangeDash(BString& string)
{
	string.Replace("\xE2\x80\x93","-");
}
