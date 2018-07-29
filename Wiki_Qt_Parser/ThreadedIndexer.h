#pragma once
#include "PageIndex.h"
#include "WordIndex.h"
#include "ImageIndex.h"
#include "boost/thread.hpp"
#include "Common.h"
#include "SimpleXml.h"
#include "pugixml.hpp"
#include "StringTree.h"
#include "StdMap.h"

//Multithreaded text indexer
class ThreadedIndexer
{
public:
	ThreadedIndexer(PageIndex& thePageIndex, WordIndex& theWordIndex, ImageIndex& theImageIndex,
					CImmStringTree& theNounTree,int theNumThreads, CBidirectionalMap<BString>& theFreqWordMap);
	~ThreadedIndexer(void);

public:
	void Index();					//Threaded function to index XML content and store data in wordIndex
	void SetMaxPagesToProcess(int theMaxNumPages);	//Set to -1 (default) for all pages to be processed
		
private:
	//Applies the function to every AD page
	//F receives two arguments - the document reference for the page and the page number
	//Call SetCurADPage() first
	template<typename F> void ApplyToPages(F&& FunctionToCall);

	//Applies the function to every string in the artDisambigXml - the funtion to call is responsible for locking
	//F receives two arguments - the string reference and the page number
	//Call SetCurADPage() first
	template<typename F> void ApplyToStringsInADXml(F&& FunctionToCall);

	//Applies the function to every caption from ImageIndex
	//FunctionToCall should take as params the caption (BString&) and imageNum
	template<typename F> void ApplyToCaptions(F&& FunctionToCall);

	//Applies the function to every string in an XML node - the funtion to call is responsible for locking
	//F receives two arguments - the string reference and the page number
	//Call SetCurADPage() first
	template<typename F> void ApplyToStringsInNode(xml_node& node, int pageNum, F&& FunctionToCall);
	

	//AD page access
	void SetCurADPage(int pageNum);
	void GetNextADpage(BString& page,	int& i);	//Returns the page and its number, returns empty page when out of pages

	//Indexing helper functions
	void IndexWordsFromString(BString& string, int pageNum, CHArray<int,int64>& pageNumArr, CHArray<int,int64>& wordNumArr);	//called by Index()

	//Chained calls
	void IndexNounsThread(CHArray<int,int64>& pageNumArr, CHArray<int,int64>& wordNumArr,
						CHArray<int,int64>& imageNumArr,CHArray<int,int64>& wordNumArrForImages);		//Called by Index()
	void IndexNounsOnPage(xml_node& docNode, int pageNum, CHArray<int,int64>& pageNumArr,CHArray<int,int64>& wordNumArr,
							CHArray<int,int64>& imageNumArr,CHArray<int,int64>& wordNumArrForImages);
	void IndexNounsFromString(BString& string, int pageNum, CHArray<int,int64>& pageNumArr,
								CHArray<int,int64>& wordNumArr, StdMap<int,int>& linksToMap);	//helper

	//Fills links from a page into a map
	//Regular links and "Main page" links are included
	void FillLinksToMap(xml_node& docNode, int pageNum, StdMap<int,int>& linksToMap);

public:
	//The number of indexed nouns in text - available after indexing
	int64 simpleWordsInXml;
	int64 nounsInXml;
	int64 simpleWordsInImages;
	int64 nounsInImages;
	
private:
	PageIndex& pageIndex;
	WordIndex& wordIndex;
	ImageIndex& imageIndex;
	CImmStringTree& nounTree;
	CBidirectionalMap<BString>& freqWordMap;
	int numThreads;

	boost::recursive_mutex pageMutex;
	boost::recursive_mutex indexingMutex;

	//maximum number of pages to process - for testing purposes
	//Set to -1 (default) for all pages
	int maxNumPages;

	int curADpage;

	BString wordSeparators;		//Separators for simple word indexing
	BString nounStartSep;			//Separators in front of the noun
	BString nounEndSep;			//Separators after the noun

	CCommon common;
};

//Applies the function to all image captions from ImageIndex
//Function to call shall accept the caption as a BString& and the imageNum
template<typename F>
void ThreadedIndexer::ApplyToCaptions(F&& FunctionToCall)
{
	for(int i=0;i<imageIndex.Count();i++)
	{
		ImageInfo curImage;
		imageIndex.GetImage(curImage,i);
		int numCaptions=curImage.captions.Count();

		for(int j=0;j<numCaptions;j++) FunctionToCall(curImage.captions[j],i);
	}
}

//Applies the function to every page in artDisambigXml
template<typename F>
void ThreadedIndexer::ApplyToPages(F&& FunctionToCall)
{
	BString page;
	int pageNum;
	GetNextADpage(page,pageNum);
	
	while(page!="")
	{
		//Parse it to XML
		xml_document doc;
		StringToXml(doc,page);
		//Call the function on the document
		FunctionToCall(doc,pageNum);

		GetNextADpage(page,pageNum);
	}
}

//Applies the function to every string in the artDisambigXml
template<typename F>
void ThreadedIndexer::ApplyToStringsInADXml(F&& FunctionToCall)
{
	BString page;
	int pageNum;
	GetNextADpage(page,pageNum);
	
	while(page!="")
	{
		//Parse it to XML
		xml_document doc;
		StringToXml(doc,page);

		//Call the function on the document
		ApplyToStringsInNode(doc,pageNum,FunctionToCall);

		GetNextADpage(page,pageNum);
	}	
}

//Applies the function to every string in an XML node - the funtion to call is responsible for locking
//Recursive
template<typename F>
void ThreadedIndexer::ApplyToStringsInNode(xml_node& node, int pageNum, F&& FunctionToCall)
{
	if(node.type()!=node_element && node.type()!=node_document) return;

	BString nodeName=node.name();
	if(nodeName=="par" || nodeName=="listEl" || nodeName=="title" || nodeName=="secTitle")		//This is a "printable" node
	{
		BString string;
		WriteContentToString(node,string);
		//Call the function 
		FunctionToCall(string,pageNum);
		
	}
	else	//it is not a bottom-level node; run recursively on all children
	{
		xml_node child=node.first_child();
		while(child)
		{
			ApplyToStringsInNode(child, pageNum, FunctionToCall);
			child=child.next_sibling();
		}
	}
}

