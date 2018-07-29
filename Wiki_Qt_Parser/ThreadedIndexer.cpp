#ifdef _MFC_VER
	#include "StdAfx.h"
#endif
#include "ThreadedIndexer.h"
#include "DizzyUtility.h"
#include "ImageInfo.h"

using namespace SimpleXml;
using namespace pugi;
using namespace DizzyUtility;

ThreadedIndexer::ThreadedIndexer(PageIndex& thePageIndex, WordIndex& theWordIndex, ImageIndex& theImageIndex,
									CImmStringTree& theNounTree,int theNumThreads, CBidirectionalMap<BString>& theFreqWordMap):
pageIndex(thePageIndex),
wordIndex(theWordIndex),
imageIndex(theImageIndex),
nounTree(theNounTree),
numThreads(theNumThreads),
freqWordMap(theFreqWordMap),
wordSeparators(" .,;:\"?'!-()/&+=|"),
nounStartSep(" \"-(/&+=|"),
nounEndSep(" .,;:\"?'!-)/&+=|"),
maxNumPages(-1)
{
}

ThreadedIndexer::~ThreadedIndexer(void)
{
}

//Threaded function to add every word in wiki to a map
void ThreadedIndexer::Index()
{
	//Index words from page strings
	SetCurADPage(0);

	//Arrays to store word numbers and the page numbers where they come from
	//wordMap is already appropriately sized
	int maxWords=2000000000;	//2 billion
	CHArray<int,int64> pageNumArr(maxWords);
	CHArray<int,int64> wordNumArr(maxWords);

	//Index simple words in a single thread - no locking needed
	ApplyToStringsInADXml(boost::bind(&ThreadedIndexer::IndexWordsFromString,this,
								_1,
								_2,
								boost::ref(pageNumArr),
								boost::ref(wordNumArr)));

	//How many words have been indexed
	simpleWordsInXml=pageNumArr.Count();

	//Index simple words from captions
	//We can use the same indexing function - IndexWordsFromString()
	CHArray<int,int64> imageNumArr(maxWords/10);
	CHArray<int,int64> wordNumArrForImages(maxWords/10);

	//Single thread indexing - no locking needed
	ApplyToCaptions(boost::bind(&ThreadedIndexer::IndexWordsFromString,this,
								_1,
								_2,
								boost::ref(imageNumArr),
								boost::ref(wordNumArrForImages)));

	//How many words have been indexed
	simpleWordsInImages=imageNumArr.Count();

	//Index nouns from xml and captions - use multiple threads
	SetCurADPage(0);

	boost::thread_group threads;
	for(int i=0;i<numThreads;i++)
	{
		threads.create_thread(boost::bind(&ThreadedIndexer::IndexNounsThread,this,
											boost::ref(pageNumArr),
											boost::ref(wordNumArr),
											boost::ref(imageNumArr),
											boost::ref(wordNumArrForImages)));
	}
	threads.join_all();
	
	//How many nouns have been indexed
	nounsInXml=pageNumArr.Count()-simpleWordsInXml;
	nounsInImages=imageNumArr.Count()-simpleWordsInImages;

	//Create inverse index - pages for words
	int numWords=wordIndex.wordMap.Count();
	wordIndex.pagesForWords.BuildIndex(wordIndex.numTimesSeenOnPage,(int64)numWords,wordNumArr,pageNumArr);

	//Create inverse index - images for words
	wordIndex.imagesForWords.BuildIndex(wordIndex.numTimesSeenInImage,(int64)numWords,wordNumArrForImages,imageNumArr);
}

void ThreadedIndexer::IndexNounsThread(	CHArray<int,int64>& pageNumArr,
										CHArray<int,int64>& wordNumArr,
										CHArray<int,int64>& imageNumArr,
										CHArray<int,int64>& wordNumArrForImages)
{
	ApplyToPages(boost::bind(&ThreadedIndexer::IndexNounsOnPage,this,
							_1,
							_2,
							boost::ref(pageNumArr),
							boost::ref(wordNumArr),
							boost::ref(imageNumArr),
							boost::ref(wordNumArrForImages)));
}

void ThreadedIndexer::IndexNounsOnPage(xml_node& docNode, int pageNum,
										CHArray<int,int64>& pageNumArr,
										CHArray<int,int64>& wordNumArr,
										CHArray<int,int64>& imageNumArr,
										CHArray<int,int64>& wordNumArrForImages)
{
	//Compose map holding the outgoing link page numbers for this page
	StdMap<int,int> linksToMap;
	FillLinksToMap(docNode,pageNum,linksToMap);
	linksToMap.Insert(pageNum);		//Add the page number of the current page to allowed noun interpretations

	//Index nouns from XML
	ApplyToStringsInNode(docNode,pageNum,boost::bind(&ThreadedIndexer::IndexNounsFromString,this,
														_1,
														_2,
														boost::ref(pageNumArr),
														boost::ref(wordNumArr),
														boost::ref(linksToMap)));

	//Index nouns from captions on the page - in the context of this page
	CHArray<int> imagesForPage((int*)NULL,0,true);		//images present on this page
	pageIndex.imagesForADpages.GetVirtualElement(pageNum,imagesForPage);
	BString pageUrl=pageIndex.artDisambigUrls[pageNum];

	for(int i=0;i<imagesForPage.Count();i++)
	{
		ImageInfo curImage;
		imageIndex.GetImage(curImage,imagesForPage[i]);
		int numCaptions=curImage.captions.Count();

		for(int j=0;j<numCaptions;j++)
		{
			if(curImage.pageUrls[j]!=pageUrl) continue;		//Only for captions from the current page
			if(curImage.captions[j]=="") continue;

			//Index nouns from this caption - it's on the current page
			IndexNounsFromString(curImage.captions[j],imagesForPage[i],
									imageNumArr,wordNumArrForImages,linksToMap);
		}
	}
}

//Fills links from a page into a map
//Regular links and "Main page" links are included
//Helper for IndexNounsOnPage()
void ThreadedIndexer::FillLinksToMap(xml_node& docNode, int pageNum, StdMap<int,int>& linksToMap)
{
	CHArray<int> linksOnPage((int*)NULL,0,true);	//virtual array
	pageIndex.linksToForADpages.GetVirtualElement(pageNum,linksOnPage);

	//Insert regular links
	linksToMap.InsertFromArray(linksOnPage);

	//Find the targets of all "Main" templates on the page
	CHArray<BString> mainTemplateTargets;
	ExtractMainTemplateTargets(docNode,mainTemplateTargets);

	//Add the destination pages from the main templates to the map
	for(int i=0;i<mainTemplateTargets.Count();i++)
	{
		int ADindex=pageIndex.artDisambigMap.GetIndex(mainTemplateTargets[i]);
		if(ADindex==-1 || pageIndex.IsDisambig(ADindex)) continue;

		linksToMap.Insert(ADindex);
	}
}

//Indexes nouns from a string
//Has locking - can be used with multiple threads
void ThreadedIndexer::IndexNounsFromString(BString& string, int pageNum,
											CHArray<int,int64>& pageNumArr,
											CHArray<int,int64>& wordNumArr,
											StdMap<int,int>& linksToMap)
{
	string.MakeLower();
	int stringLength=string.GetLength();

	CHArray<int> nounIndices(16);
	CHArray<int> virtArray((int*)NULL,0,true);	//virtual array for extracting data from CAIStrings
	CHArray<int> convArray(100);				//"convert" array for when we need to change data in virtArray - virtArray is read-only!

	for(int pos=0;pos<stringLength;pos++)
	{
		//guard against reading from the middle of the word - nouns start with a noun start separator
		if(pos!=0 && nounStartSep.Find(string[pos-1])==-1) continue;

		//get matches from the tree
		nounTree.GetAllMatches(string,pos,nounIndices);
		int numMatches=nounIndices.Count();
		if(numMatches==0) continue;

		//check each match and if it cannot be included, change the index in nounIndices to -1
		int numAllowed=0;
		for(int i=0;i<numMatches;i++)
		{
			BString& word=wordIndex.wordMap.wordArr[nounIndices[i]];
			WordInfo& wordInfo=wordIndex.wordInfo[nounIndices[i]];

			//Nouns must end in a noun end separator
			int wordLength=word.GetLength();
			if((pos+wordLength)!=stringLength && nounEndSep.Find(string[pos+wordLength])==-1)
			{
				nounIndices.arr[i]=-1;
				continue;
			}

			//If a noun is also a simple word, discard it - it's already indexed as a simple word
			if(wordInfo.isSimpleWord)
			{
				nounIndices.arr[i]=-1;
				continue;
			}

			//Decide noun eligibility based on whether the link is cited on the page
			bool nounAllowed=false;

			//check whether it should be allowed as a title
			if(!nounAllowed && wordInfo.isTitleADWP)
			{
				int lowerADWPtitleIndex=pageIndex.lowerADWPtitleMap.GetIndex(word);
				pageIndex.pagesForLowerADWPtitles.GetVirtualElement(lowerADWPtitleIndex,virtArray);

				if(linksToMap.IsPresentOneOf(virtArray)) nounAllowed=true;
			}

			//check whether the noun should be allowed as a redirect
			if(!nounAllowed && wordInfo.isRed)
			{
				int lowerRedIndex=pageIndex.lowerRedMap.GetIndex(word);
				pageIndex.redsForLowerReds.GetVirtualElement(lowerRedIndex,virtArray);

				//Convert redirect indices to page indices
				convArray.ResizeIfSmaller(virtArray.Count(),true);
				for(int j=0;j<virtArray.Count();j++) convArray.arr[j]=pageIndex.redToADpageIndex[virtArray[j]];

				if(linksToMap.IsPresentOneOf(convArray)) nounAllowed=true;
			}

			//check whether the noun should be allowed as a bolded synonym
			if(!nounAllowed && wordInfo.isBsyn)
			{
				int index=pageIndex.bSynLowerMap.GetIndex(word);
				pageIndex.adPagesForBSyn.GetVirtualElement(index,virtArray);

				if(linksToMap.IsPresentOneOf(virtArray)) nounAllowed=true;
			}

			if(!nounAllowed)
			{
				nounIndices.arr[i]=-1;
				continue;
			}

			numAllowed++;
		}
		if(numAllowed==0) continue;

		//Add the allowed nouns
		//locked scope
		{
			boost::recursive_mutex::scoped_lock lock(indexingMutex);

			for(int i=0;i<numMatches;i++)
			{
				//If the noun cannot be included, do nothing
				if(nounIndices.arr[i]==-1) continue;

				pageNumArr.AddAndExtend(pageNum);
				wordNumArr.AddAndExtend(nounIndices.arr[i]);
			}
		}
	}
}

/*for(int pos=0;pos<stringLength;pos++)
	{
		//guard against reading from the middle of the word - nouns start with a noun start separator
		if(pos!=0 && nounStartSep.Find(string[pos-1])==-1) continue;

		//get matches from the tree
		nounTree.GetAllMatches(string,pos,nounIndices);
		int numMatches=nounIndices.Count();
		if(numMatches==0) continue;

		//check each match and if it cannot be included, change the index in nounIndices to -1
		int numAllowed=0;
		for(int i=0;i<numMatches;i++)
		{
			//Nouns must end in a noun end separator
			BString& word=wordIndex.wordMap.wordArr[nounIndices[i]];
			WordInfo& wordInfo=wordIndex.wordInfo[nounIndices[i]];
			int wordLength=word.GetLength();
			if((pos+wordLength)!=stringLength && nounEndSep.Find(string[pos+wordLength])==-1)
			{
				nounIndices.arr[i]=-1;
				continue;
			}

			//If a noun is also a simple word, discard it - it's already indexed as a simple word
			if(wordInfo.isSimpleWord)
			{
				nounIndices.arr[i]=-1;
				continue;
			}

			//Noun eligibility based on whether the link is cited on the page
			bool nounAllowed=false;

			//check whether the noun should be allowed as a title
			if(!nounAllowed && wordInfo.isTitleADWP)
			{
				int lowerADWPtitleIndex=pageIndex.lowerADWPtitleMap.GetIndex(word);
				pageIndex.pagesForLowerADWPtitles.GetVirtualElement(lowerADWPtitleIndex,virtArray);

				if(linksToMap.IsPresentOneOf(virtArray)) nounAllowed=true;
			}

			//check whether the noun should be allowed as a redirect
			if(!nounAllowed && wordInfo.isRed)
			{
				int lowerRedIndex=pageIndex.lowerRedMap.GetIndex(word);
				pageIndex.redsForLowerReds.GetVirtualElement(lowerRedIndex,virtArray);

				//Convert redirect indices to page indices
				for(int j=0;j<virtArray.Count();j++) virtArray[j]=pageIndex.redToADpageIndex[virtArray[j]];

				if(linksToMap.IsPresentOneOf(virtArray)) nounAllowed=true;
			}

			//check whether the noun should be allowed as a bolded synonym
			if(!nounAllowed && wordInfo.isBsyn)
			{
				int index=pageIndex.bSynLowerMap.GetIndex(word);
				pageIndex.adPagesForBSyn.GetVirtualElement(index,virtArray);

				if(linksToMap.IsPresentOneOf(virtArray)) nounAllowed=true;
			}

			if(!nounAllowed)
			{
				nounIndices.arr[i]=-1;
				continue;
			}

			numAllowed++;
		}
		if(numAllowed==0) continue;

		//Add the allowed nouns
		//locked scope
		{
			boost::recursive_mutex::scoped_lock lock(indexingMutex);

			for(int i=0;i<numMatches;i++)
			{
				//If the noun cannot be included, do nothing
				if(nounIndices.arr[i]==-1) continue;

				pageNumArr.AddAndExtend(pageNum);
				wordNumArr.AddAndExtend(nounIndices.arr[i]);
			}
		}
	*/

//Indexes words from a string into the word map
//Does not have locking - single thread only (it's quick anyway)
void ThreadedIndexer::IndexWordsFromString(BString& string, int pageNum, CHArray<int,int64>& pageNumArr, CHArray<int,int64>& wordNumArr)
{
	string.MakeLower();

	int pos=0;
	BString curWord=string.Tokenize(wordSeparators,pos);
	while(curWord!="")
	{
		if(!freqWordMap.IsPresent(curWord))	//if not a frequent word
		{
			int index=wordIndex.wordMap.AddWordGetIndex(curWord);
			wordIndex.wordInfo[index].isSimpleWord=true;
			wordNumArr.AddAndExtend(index);
			pageNumArr.AddAndExtend(pageNum);
		}
		curWord=string.Tokenize(wordSeparators,pos);
	}
}

//AD page access
void ThreadedIndexer::SetCurADPage(int pageNum)
{
	boost::recursive_mutex::scoped_lock lock(pageMutex);
	curADpage=pageNum;
}

//Sets the number of articles to process
//Set to -1 for all pages to be processed
void ThreadedIndexer::SetMaxPagesToProcess(int theMaxNumPages)
{
	boost::recursive_mutex::scoped_lock lock(pageMutex);
	maxNumPages=theMaxNumPages;
}

//Will return an empty string when there are no more pages
void ThreadedIndexer::GetNextADpage(BString& page, int& i)
{
	boost::recursive_mutex::scoped_lock lock(pageMutex);

	int maxPageCount;
	if(maxNumPages<0 || maxNumPages>=(int)pageIndex.artDisambigXml.Count()) maxPageCount=(int)pageIndex.artDisambigXml.Count();
	else maxPageCount=maxNumPages;

	i=curADpage;
	if(curADpage>=maxPageCount) {page="";return;}	//No more pages

	pageIndex.artDisambigXml.GetCharStringAt(curADpage,page);
	curADpage++;
}



