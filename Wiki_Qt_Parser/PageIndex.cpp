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

#include "PageIndex.h"

PageIndex::PageIndex(void)
{
}

PageIndex::~PageIndex(void)
{
}

void PageIndex::Serialize(BArchive& ar)
{
	artDisambigUrls.Serialize(ar);
	artUrls.Serialize(ar);
	disambigUrls.Serialize(ar);
	redirectFrom.Serialize(ar);
	redirectTo.Serialize(ar);
	redToADpageIndex.Serialize(ar);
	templateUrls.Serialize(ar);

	isListAD.Serialize(ar);

	renumFromADtoArt.Serialize(ar);
	renumFromArtToAD.Serialize(ar);
	
	numArtCited.Serialize(ar);
	numArtDisambigCited.Serialize(ar);
	numRedirectCited.Serialize(ar);

	similarArticles.Serialize(ar);

	linksFromForADpages.Serialize(ar);
	linksToForADpages.Serialize(ar);

	anchorsLowerMap.Serialize(ar);
	anchorsForADpages.Serialize(ar);
	freqAnchorsForADpages.Serialize(ar);
	adPagesForAnchors.Serialize(ar);
	freqADpagesForAnchors.Serialize(ar);

	lowerADWPtitleMap.Serialize(ar);
	pagesForLowerADWPtitles.Serialize(ar);
	lowerRedMap.Serialize(ar);
	redsForLowerReds.Serialize(ar);

	artDisambigUrlsWP.Serialize(ar);
	parenthAD.Serialize(ar);
	isADparenth.Serialize(ar);

	bSynLowerMap.Serialize(ar);
	bSynForADpages.Serialize(ar);
	adPagesForBSyn.Serialize(ar);

	imagesForADpages.Serialize(ar);

	templateXml.Serialize(ar);
}

bool PageIndex::Load(const BString& fileName)
{
	bool ret=Savable::Load(fileName);	//will call serialize
	if(!ret) {return false;}

	//Fill all maps
	artDisambigMap.AddFromArray(artDisambigUrls);
	artMap.AddFromArray(artUrls);
	disambigMap.AddFromArray(disambigUrls);
	redirectFromMap.AddFromArray(redirectFrom);
	templateMap.AddFromArray(templateUrls);

	return true;
}
