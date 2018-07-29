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
#include "Array.h"
#include "boost/thread.hpp"
#include "Common.h"
#include "Timer.h"
#include "CAIStrings.h"
#include "CAISSplitWriter.h"
#include "BidirectionalMap.h"
#include "WikipediaParser.h"
#include "PageIndex.h"
#include <iostream>
#include <fstream>

#include <boost/iostreams/filtering_streambuf.hpp>
typedef boost::iostreams::filtering_streambuf<boost::iostreams::input> boost_istreambuf;

class ThreadedParserStats
{
public:
	ThreadedParserStats():
			fSpecialStatus(false),
			numPagesParsed(0),
			totalBytesRead(0)
			{};

public:
	bool fSpecialStatus;
	BString specialStatus;
	BString lastArticle;
	int numPagesParsed;
	int64 totalBytesRead;
};

//Multithreaded wrapper for the CWikipediaParser class
class ThreadedParser
{
public:
	ThreadedParser(const BString& parserConfigFile);
	~ThreadedParser(void){};

public:
	//Parse synchronously or asynchronously
	void Parse(boost_istreambuf* theFile, int numThreads,
					const BString& saveFolder, std::ostream& report,
					int maxNumPages=1000000000,
					bool fSynchronous = true);
	bool IsRunning();
	void Stop();
	void GetCurStats(ThreadedParserStats& stats);
	int NumPagesParsed();	//Number of pages successfully parsed
	int NumADPagesSaved();	//Number of articles and disambiguations saved to XML file

	PageIndex& GetPageIndex(){return pageIndex;};

	//Setting options
	void SetShortReport(bool val)		{fShortReport = val;};
	void SetDiscardLists(bool val)		{fDiscardLists = val;};
	void SetDiscardDisambigs(bool val)	{fDiscardDisambigs = val;};
	void SetInputFileForReport(const BString& file) {inputFileForReport = file;};
	void SetXmlFileName(const BString& file) {xmlFileName = file;};
	void SetIiaFileName(const BString& file) {iiaFileName = file;};
	void SetPageIndexFileName(const BString& file) {pIndexFileName = file;};
	void SetWritePageIndex(bool val) {fWritePageIndex = val;};
	void SetPrependToXML(const BString& string) {prependToXML = string;};

private:
	//Worker threads
	void ParsingThread();

	//Wrapper thread for workers in asynch operation
	void WrapperThread(int numThreads, BString saveFolder, std::ostream& report);

	//Watching thread count
	void IncrementThreads();
	void DecrementThreads();
	
private:
	boost::recursive_mutex mutex;
	int numActiveThreads;			//number of currently active worker threads
	bool stopFlag;					//Flag that signals the threads to stop as if the end of file has been reached
	volatile bool fRunning;
	volatile bool fReadingData;		//Flag that indicates that the parser is currently reading data from file

	//File names - these are set in the constructor, and can be changed through setters
	BString xmlFileName;
	BString iiaFileName;
	BString pIndexFileName;

	//Options
	bool fShortReport;
	bool fDiscardLists;
	bool fDiscardDisambigs;
	bool fWritePageIndex;		//Whether the page index file is written at the end of the parse
	BString prependToXML;		//The string that can be prepended to the XML storage file, but not included in the CAIS

	CTimer timer;
	CCommon common;
	BString configFile;
	CWikipediaParser dummyParser;		//A parser just to store the error output from the actual working parsers

	//Params to save for reporting at the end of the parse
	BString startTimeString;			//A string with start time
	int threadsUsed;					//Number of threads to use
	BString inputFileForReport;			//For reporting purposes, can be set with SetInputFileForReport()
	BString outputDir;					//For reporting purposes

	//page will have the next page in it, with <page> and </page> markers
	//or will be empty if there are no pages left in the file
	void GetNextPage(BString& page);
	void ReadNextChunk();	//read next chunk from the file, leaving in the buffer whatever hasn't been processed
	
	boost_istreambuf* file;	//the file buffer on which we can call "read" - may be reading from bz2 or plain file, we don't care
	bool eofReached;
	int64 curBufferOffset;	//the position just past the last successfully read <page> in memory buffer
	int64 readSize;			//chunk size
	int maxPagesToParse;
	int64 totalBytesRead;	//The number of bytes read from the input stream
	int totalPagesRead;		//The number of pages read from the input stream
	BString lastArticleTitle;	//For display purposes, the title of the last article parsed
	CHArray<char,int64> buffer;		//buffer with a size of readSize, that holds the current chunk from the file
	CHArray<char,int64> copyArray;	//auxilliary array for copying the page into a string

	CHArray<char,int64> pageBegin;	//Marker sequences for page start and end
	CHArray<char,int64> pageEnd;

	//Data saved during processing
	void ClearData();
	void SaveData(const BString& saveFolder, std::ostream& report);

	int numPagesParsed;
	int numArticles;
	int numListAD;
	int numRedirects;
	int numDisambigs;
	int numTemplates;
	int numSavedTemplates;
	int numOtherPages;
	int numFailed;			//Number of pages for which ParseArticle() returned false
		
	PageIndex pageIndex;

	CAISSplitWriter<char,int64> xmlADsplitWriter;			//XML text for all articles and disambiguations stored as separate data and iia
};

