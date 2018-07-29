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

#include "ThreadedParser.h"
#include "boost/bind.hpp"
#include "CommonUtility.h"

#include <iostream>
#include <fstream>

#include <boost/iostreams/operations.hpp>

ThreadedParser::ThreadedParser(const BString& parserConfigFile):
pageBegin("<page>",6),
pageEnd("</page>",7),
configFile(parserConfigFile),
dummyParser(parserConfigFile,true)
{
	//File names - set defaults
	xmlFileName = "xml_of_articles_and_disambigs.xml";
	iiaFileName = "ADiia.ari64";			//Article and disambig initial index array in xmlFileName
	pIndexFileName = "page_index.cust";

	//Options
	fShortReport				= false;
	fDiscardLists				= false;
	fDiscardDisambigs			= false;
	fWritePageIndex				= true;

	//Other initializations
	fRunning = false;

	readSize=500000000;			//Reads in chunks of 500 MB
	buffer.ResizeArray(readSize);

	pageIndex.artUrls.ResizeArray(6000000);
	pageIndex.artDisambigUrls.ResizeArray(7000000);
	pageIndex.redirectFrom.ResizeArray(8000000);
	pageIndex.redirectTo.ResizeArray(8000000);
	pageIndex.disambigUrls.ResizeArray(500000);
	pageIndex.templateUrls.ResizeArray(100000);
	pageIndex.isListAD.ResizeArray(8000000);
	pageIndex.templateXml.ResizeIfSmaller(50000000,100000);

	ClearData();
}

void ThreadedParser::ClearData()
{
	numActiveThreads = 0;
	stopFlag = false;
	totalBytesRead = 0;
	totalPagesRead = 0;
	fReadingData = false;

	numPagesParsed=0;
	numArticles=0;
	numListAD=0;
	numRedirects=0;
	numTemplates=0;
	numSavedTemplates=0;
	numDisambigs=0;
	numOtherPages=0;
	numFailed=0;

	pageIndex.Clear();

	xmlADsplitWriter.Clear();		//Not really necessary, gets cleared on Open()
	
	dummyParser.ClearErrorMaps();
}

void ThreadedParser::Parse(boost_istreambuf* theFile,int numThreads,
								const BString& saveFolder, std::ostream& report,
								int maxNumPages, bool fSynchronous)
{
	//Set all init params
	timer.SetTimerZero(0);

	file=theFile;
	curBufferOffset=0;
	buffer.SetNumPoints(0);
	eofReached = false;
	fRunning = true;
	ClearData();
	maxPagesToParse=maxNumPages;

	//Save some params for reporting later
	startTimeString = CommonUtility::CurDateTimeString("%Y-%m-%d %H:%M:%S");
	threadsUsed = numThreads;
	outputDir = saveFolder;

	//If prependToXML is not "", we'll prepend that string to the storage file
	//Without including it in the CAIS
	xmlADsplitWriter.Open(saveFolder + xmlFileName,prependToXML);

	//Launch workers through a wrapper function
	if(fSynchronous)	//Synchronous operation - launch wrapper function from current thread
	{
		WrapperThread(numThreads, saveFolder, report);
	}
	else	//Asynchronous - we will launch a wrapper thread and return immediately
	{
		boost::thread wrapper(boost::bind(	&ThreadedParser::WrapperThread,
											this,
											numThreads,
											saveFolder,
											boost::ref(report)));
		wrapper.detach();
	}
}

//Wrapper that launches worker threads
void ThreadedParser::WrapperThread(int numThreads, BString saveFolder, std::ostream& report)
{
	boost::thread_group threads;
	for(int i=0;i<numThreads;i++)
	{
		threads.create_thread(boost::bind(&ThreadedParser::ParsingThread,this));
	}
	threads.join_all();

	SaveData(saveFolder, report);

	fRunning = false;
}

//Worker thread
void ThreadedParser::ParsingThread()
{
	using namespace SimpleXml;

	//Thread is starting
	IncrementThreads();

	//Locking around parser creation - is it necessary?
	mutex.lock();
	CWikipediaParser parser(configFile,true);		//Each thread has its own parser
	mutex.unlock();

	BString page;
	BString curPageText;
	BString redirectTarget;

	while(1)
	{
		GetNextPage(page);
		if(page=="") break;

		pugi::xml_document xmlDoc;
		if(!parser.ParseArticle(page,xmlDoc))		//Page parse failure
		{
			boost::recursive_mutex::scoped_lock lock(mutex);
			numFailed++;
			continue;
		}

		BString type=xmlDoc.child("page").attribute("type").value();
		BString list=xmlDoc.child("page").attribute("list").value();
		char fList=0;
		if(list=="yes") {fList=1;numListAD++;}

		if(type=="other")		//If it's not a useful page type, increment counts and continue
		{
			boost::recursive_mutex::scoped_lock lock(mutex);
			numPagesParsed++;
			numOtherPages++;
			continue;
		}

		//If article or disambig, write XML to string
		if(type=="article" || type=="disambig")
		{
			SimplestXml::XmlToString(xmlDoc,curPageText,true);
		}

		//If this is the right kind of template, save its XML too
		bool fUsefulTemplate=false;
		BString url=xmlDoc.child("page").child("url").first_child().value();
		if(type=="template")
		{
			//Check whether this is "Template:Infobox..."
			BString tempTarget=url.Left(16);
			tempTarget.MakeLower();
			if(tempTarget=="template:infobox")
			{
				fUsefulTemplate=true;
				SimplestXml::XmlToString(xmlDoc,curPageText,true);
			}
		}

		//Extract redirect target for redirects
		if(type=="redirect")
		{
			redirectTarget=xmlDoc.child("page").attribute("target").value();
		}

		//Now we update global data while holding the lock
		{
			boost::recursive_mutex::scoped_lock lock(mutex);
			numPagesParsed++;

			if(type == "article")
			{
				if(!fList) numArticles++;		//We don't count list articles into the total number of articles

				//For articles that aren't lists, and for lists when not discarding them
				if(!fList || !fDiscardLists)
				{
					xmlADsplitWriter.AddCharString(curPageText,false);
					pageIndex.artDisambigUrls.AddAndExtend(url);
					pageIndex.isListAD.AddAndExtend(fList);
					lastArticleTitle = url;

					pageIndex.artUrls.AddAndExtend(url);
				}
			}

			if(type=="disambig")
			{
				numDisambigs++;

				//If not discarding disambiguations
				if(!fDiscardDisambigs)
				{
					xmlADsplitWriter.AddCharString(curPageText,false);
					pageIndex.artDisambigUrls.AddAndExtend(url);
					pageIndex.isListAD.AddAndExtend(fList);
					lastArticleTitle = url;

					pageIndex.disambigUrls.AddAndExtend(url);
				}
			}

			if(type=="redirect")
			{
				numRedirects++;
				pageIndex.redirectFrom.AddAndExtend(url);
				pageIndex.redirectTo.AddAndExtend(redirectTarget);
			}

			if(type=="template")
			{
				numTemplates++;
				if(fUsefulTemplate)
				{
					numSavedTemplates++;
					pageIndex.templateUrls.AddAndExtend(url);
					pageIndex.templateXml.AddCharString(curPageText);
				}
			}
		}	//End locked scope
	}

	//There are no more pages in the data file
	//Copy errors from the thread-local parser into the error maps in the dummy parser
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		dummyParser.AppendErrorMaps(parser);
	}

	//Thread is exiting
	DecrementThreads();
}

bool ThreadedParser::IsRunning()
{
	return fRunning;
}

void ThreadedParser::Stop()
{	
	boost::recursive_mutex::scoped_lock lock(mutex);
	stopFlag = true;
}

void ThreadedParser::IncrementThreads()
{
	boost::recursive_mutex::scoped_lock lock(mutex);
	numActiveThreads ++;
}

void ThreadedParser::DecrementThreads()
{
	boost::recursive_mutex::scoped_lock lock(mutex);
	numActiveThreads --;
}

void ThreadedParser::SaveData(const BString& saveFolder, std::ostream& report)
{
	//Write parse report file
	report << "***Wiki Parser: Multithreaded Parse of Wikipedia Database to XML***\n\n";

	if(inputFileForReport != "") report << "Wikipedia database file to parse: " << inputFileForReport << ".\n";
	report << "Saving parsed data to directory: " << outputDir << "\n\n";

	report << "Parse started: " << startTimeString << "\n";
	report << "Parse ended: " << CommonUtility::CurDateTimeString("%Y-%m-%d %H:%M:%S") << "\n";
	int h, m, s;
	CommonUtility::SecondsToHMS((int)timer.GetCurTime(0),h,m,s);
	report << "Parsing took a total of: " << h << " hours, " << m << " minutes, " << s << " seconds.\n";
	report << "Number of parsing threads used: " << threadsUsed << " threads.\n\n";
	
	//Indicate whether we are discarding lists of disambiguations
	if(fDiscardLists) report << "List-like articles were discarded during the parse.\n";
	if(fDiscardDisambigs) report << "Disambiguation pages were discarded during the parse.\n";
	if(fDiscardLists || fDiscardDisambigs) report << "\n";

	report << "Total number of pages that were successfully parsed: " << numPagesParsed << ".\n";
	report << "Number of pages that failed to parse: " << numFailed <<".\n\n";
	report << "Number of articles among the parsed pages (exclusing lists): " << numArticles << ".\n";

	report << "Numer of list articles among the parsed pages: "<< numListAD;
	if(fDiscardLists) report << " (discarded)";

	report << "\nNumber of disambiguations among the parsed pages: " << numDisambigs;
	if(fDiscardDisambigs) report << " (discarded)";

	report << "\nNumber of redirects among the parsed pages: " << numRedirects << ".\n";
	report << "Number of other pages - Wikipedia, File, Category, Template, etc. (discarded): " << numOtherPages << ".\n\n";

	report << "Types of pages saved to the XML file: \n";
	report << "\t\tNon-list articles\n";
	if(!fDiscardLists) report << "\t\tList articles\n";
	if(!fDiscardDisambigs) report << "\t\tDisambiguations\n";
	report << "Number of qualifying parsed pages saved to the XML file: " << xmlADsplitWriter.Count() << ".\n\n";

	if(!fShortReport)
	{
		report<<"Number of infobox templates: " << numTemplates << "\n";
		report<<"Size of XML text for articles + disambiguations: " << xmlADsplitWriter.StorageSize() << ".\n";
		report<<"Size of XML text for templates: " << pageIndex.templateXml.storageArr.Count() << ".\n";

		report<<"\nOutputs:\n";
		report<<"page_index.cust\n";
		report<<"xml_of_articles_and_disambigs.isc64\n";

		//Parser output from the dummy parser, which accumulated all error output:
		dummyParser.WriteReport(report);
	}
	
	//Write all data
	if(fWritePageIndex) pageIndex.Save(saveFolder + pIndexFileName);
	
	//Save init index for articles and disambigs
	xmlADsplitWriter.Close();
	xmlADsplitWriter.SaveInitIndex(saveFolder + iiaFileName);
}

//page will have the next page in it, with <page> and </page> markers
//or will be empty if there are no pages left in the file
void ThreadedParser::GetNextPage(BString& page)
{
	boost::recursive_mutex::scoped_lock lock(mutex);

	if(numPagesParsed > maxPagesToParse || stopFlag) {page="";return;};

	int64 beginPos=buffer.FindSequence(pageBegin,curBufferOffset);
	int64 endPos=-1;
	if(beginPos!=-1) endPos=buffer.FindSequence(pageEnd,beginPos);

	if(beginPos==-1 || endPos==-1)
	{
		if(eofReached) {page="";return;}

		else
		{
			ReadNextChunk();							//curBufferOffset will be set to 0
			beginPos=buffer.FindSequence(pageBegin,curBufferOffset);
			if(beginPos!=-1) endPos=buffer.FindSequence(pageEnd,beginPos);

			if(beginPos==-1 || endPos==-1) {page="";return;}	//Still could not find page markers - return;
		}
	}

	copyArray.ResizeIfSmaller(endPos-beginPos+pageEnd.Count()+1);
	buffer.ExportPart(copyArray,beginPos,endPos-beginPos+pageEnd.Count());

	//Increment read statistics
	totalBytesRead += copyArray.Count();
	totalPagesRead ++;

	//Add a NULL and copy to string
	copyArray.AddPoint(NULL);
	page=copyArray.arr;

	curBufferOffset=beginPos+1;
}

void ThreadedParser::ReadNextChunk()
{
	boost::recursive_mutex::scoped_lock lock(mutex);
	fReadingData = true;

	buffer.TrimLeft(buffer.Count()-curBufferOffset);

	int64 readRequest = readSize - buffer.Count();

	int64 countRead = boost::iostreams::read(*file, buffer.arr + buffer.Count(), readRequest);

	if(countRead < readRequest) eofReached=true;

	curBufferOffset=0;
	buffer.SetNumPoints(buffer.Count() + countRead);

	fReadingData = false;
}

//Returns the current statistics of the working parser
void ThreadedParser::GetCurStats(ThreadedParserStats& stats)
{
	//If we are reading a data chunk, don't wait for the lock
	//And just inform the caller that we are reading data
	if(fReadingData)
	{
		stats.fSpecialStatus = true;
		stats.specialStatus = "Reading the next data chunk into memory";
		return;
	}

	//If not reading data, get the lock and read all stats
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		stats.lastArticle = lastArticleTitle;
		stats.numPagesParsed = numPagesParsed;
		stats.totalBytesRead = totalBytesRead;
	}
}

int ThreadedParser::NumPagesParsed()
{
	boost::recursive_mutex::scoped_lock lock(mutex);
	return numPagesParsed;
}

int ThreadedParser::NumADPagesSaved()
{
	boost::recursive_mutex::scoped_lock lock(mutex);
	return xmlADsplitWriter.Count();
}
