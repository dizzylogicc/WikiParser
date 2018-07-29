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

#include "boost/thread.hpp"
#include "pugixml.hpp"
#include "Timer.h"

#include "CAISFileFetcher.h"

class ThreadedWriterStats
{
public:
	ThreadedWriterStats():
	  numPagesWritten(0){};

public:
	int numPagesWritten;
	BString lastPageTitle;
};

class ThreadedWriter
{
public:
	ThreadedWriter();
	~ThreadedWriter(){};

public:
	void Process(	const BString& storageFile,
					const BString& iiaFile,
					const BString& outputFile);
	bool IsRunning();
	void Stop();
	void GetCurStats(ThreadedWriterStats& stats);
	void Report(std::ostream& stream);

	//Options
	void SetSkipImCaptions(bool val){fSkipImCaptions = val;};
	void SetMarkArticles(bool val){fMarkArticles = val;};
	void SetMarkSections(bool val){fMarkSections = val;};
	void SetMarkCaptions(bool val){fMarkCaptions = val;};

private:
	void WorkerThread(	BString storageFile,
						BString iiaFile,
						BString outputFile);

private:
	//Extracts article text from parsed mediaWiki in XML
	//Replaces html entities and "strange dash"
	void WriteContentToString(const pugi::xml_node& node, BString& string);

	//Internal function for content writer
	void InternalWriteContent(const pugi::xml_node& node, char* buffer, int& counter);
	void InsertIntoWriteBuffer(char symbol, char* buffer, int& counter);	//Internal helper for content writer
	void InsertIntoWriteBuffer(const BString& string, char* buffer, int& counter);		//Internal helper

private:
	//Write options
	bool fSkipImCaptions;
	bool fMarkArticles;
	bool fMarkSections;
	bool fMarkCaptions;

private:
	boost::recursive_mutex statMutex;
	CTimer timer;
	volatile bool stopFlag;
	volatile bool fRunning;
	int numPagesWritten;
	BString lastPageTitle;

	BString startTimeString;
};