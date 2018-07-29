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
#include "Savable.h"
#include "Array.h"
#include "CAIStrings.h"
#include "BidirectionalMap.h"

class PageIndex : public Savable
{
public:
	PageIndex(const BString& fileName){Load(fileName);};
	PageIndex(void);
	~PageIndex(void);

public:
	//All URL strings in CHArray<BString>
	CHArray<BString> artDisambigUrls;
	CHArray<BString> artUrls;
	CHArray<BString> disambigUrls;
	CHArray<BString> redirectFrom;
	CHArray<BString> redirectTo;
	CHArray<int> redToADpageIndex;	//indices of AD pages to which the redirect redirects
	CHArray<BString> templateUrls;

	//For each article in artDisambigUrls, whether this is a list article
	//Disambiguations are not considered list articles
	//0 - not a list, 1 - list
	CHArray<char> isListAD;

	//Renumbering from artDisambigUrls to artUrls and back
	CHArray<int> renumFromADtoArt;
	CHArray<int> renumFromArtToAD;

	//Number of citations for each ARTICLE and redirect from ARTICLES
	CHArray<int> numArtCited;
	CHArray<int> numArtDisambigCited;		//For convenience. Same as numArtCited, but with 0s for disambigs. Follows index in artDisambigUrls.
	CHArray<int> numRedirectCited;

	//For each article in artUrls, a list of related articles (indices in artUrls)
	CAIStrings<int> similarArticles;

	//Links to and from AD
	CAIStrings<int,int> linksFromForADpages;
	CAIStrings<int,int> linksToForADpages;

	//Anchors - direct index
	CBidirectionalMap<BString> anchorsLowerMap;
	CAIStrings<int,int64> anchorsForADpages;
	CAIStrings<int,int64> freqAnchorsForADpages;
	//Anchors - inverse index
	CAIStrings<int,int64> adPagesForAnchors;
	CAIStrings<int,int64> freqADpagesForAnchors;

	//Bolded synonyms at the start of the article - direct index
	CBidirectionalMap<BString> bSynLowerMap;
	CAIStrings<int,int64> bSynForADpages;
	//Bolded synonyms - inverse index
	CAIStrings<int,int64> adPagesForBSyn;

	//Lowercase indices for page and redirect titles without parenths (WP)
	CBidirectionalMap<BString> lowerADWPtitleMap;
	CAIStrings<int,int64> pagesForLowerADWPtitles;
	CBidirectionalMap<BString> lowerRedMap;
	CAIStrings<int,int64> redsForLowerReds;		//Indices of redirect in redirectFrom for each lowercase redirect
	
	//Removing parenthecation from AD titles
	CHArray<BString> artDisambigUrlsWP;		//Title without parenthecation
	CHArray<BString> parenthAD;				//Parenthicated part, for every page in the AD subset
	CHArray<bool> isADparenth;					//Is article or disambig parenthecated

	//Images for pages
	CAIStrings<int,int64> imagesForADpages;

	//xml of pages in CAIStrings
	//Only loaded when the corresponding Load function is called on each
	CAIStrings<char,int64> artDisambigXml;
	CAIStrings<char,int64> templateXml;

	//These maps are not stored in the file
	//They are only filled on Load()
	CBidirectionalMap<BString> artDisambigMap;
	CBidirectionalMap<BString> artMap;
	CBidirectionalMap<BString> disambigMap;
	CBidirectionalMap<BString> redirectFromMap;
	CBidirectionalMap<BString> templateMap;

	//Some convenience functions
	bool IsDisambig(int ADindex){return renumFromADtoArt[ADindex]==-1;};		//Whether a page in AD is a disambiguation

	//Serialization
	void LoadArtDisambigXml(const BString& fileName){artDisambigXml.Load(fileName);};
	void SaveArtDisambigXml(const BString& fileName){artDisambigXml.Save(fileName);};
	bool Load(const BString& fileName);
	void Serialize(BArchive& ar);

	void Clear()
	{
		artUrls.EraseArray();
		artDisambigUrls.EraseArray();
		isListAD.EraseArray();
		redirectFrom.EraseArray();
		redirectTo.EraseArray();
		disambigUrls.EraseArray();
		templateUrls.EraseArray();
		templateXml.Clear();
	}
};

