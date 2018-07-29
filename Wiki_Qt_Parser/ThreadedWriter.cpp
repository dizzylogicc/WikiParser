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

#include "ThreadedWriter.h"

#include "DizzyUtility.h"
#include "CommonUtility.h"
#include "SimpleXml.h"
#include <fstream>
using namespace pugi;

ThreadedWriter::ThreadedWriter()
{
	fRunning = false;
	stopFlag = false;

	//Default options
	fSkipImCaptions = false;
	fMarkArticles = true;
	fMarkSections = true;
	fMarkCaptions = true;
}

//Processes a split CAIS into a text file
void ThreadedWriter::Process(	const BString& storageFile,
								const BString& iiaFile,
								const BString& outputFile)
{
	//Set all variables to init state
	stopFlag = false;
	fRunning = true;
	numPagesWritten = 0;
	lastPageTitle = "";

	startTimeString = CommonUtility::CurDateTimeString("%Y-%m-%d %H:%M:%S");
	timer.SetTimerZero(0);

	//Pass the request into a worker thread and return immediately
	boost::thread worker(	&ThreadedWriter::WorkerThread,this,
							storageFile,
							iiaFile,
							outputFile);
	worker.detach();
}

void ThreadedWriter::WorkerThread(	BString storageFile,
									BString iiaFile,
									BString outputFile)
{
	CAISFileFetcher<char,int64> cais(storageFile,iiaFile);
	std::ofstream outstream(outputFile, std::ios::binary);

	for(int64 i=0; i < cais.Count(); i++)
	{
		if(stopFlag) break;

		BString curPage;
		cais.GetCharStringAt(i,curPage);
		
		xml_document doc;
		SimplestXml::StringToXml(doc, curPage);

		BString text;	
		WriteContentToString(doc, text);

		outstream.write(text,text.GetLength());

		{
			boost::recursive_mutex::scoped_lock lock(statMutex);
			numPagesWritten ++;
			lastPageTitle = doc.child("page").child("title").first_child().value();
		}
	}

	fRunning = false;
}

bool ThreadedWriter::IsRunning()
{
	return fRunning;
}

void ThreadedWriter::GetCurStats(ThreadedWriterStats& stats)
{
	boost::recursive_mutex::scoped_lock lock(statMutex);
	stats.numPagesWritten = numPagesWritten;
	stats.lastPageTitle = lastPageTitle;
}

void ThreadedWriter::Stop()
{
	stopFlag = true;
}

//Extracts article text from parsed mediaWiki in XML
//Replaces html entities and "strange dash"
void ThreadedWriter::WriteContentToString(const xml_node& node, BString& string)
{
	//Uses InternalWriteContent, a double-pass XML writer function
	int numSymbols=0;
	InternalWriteContent(node,NULL,numSymbols);
	CHArray<char> buffer(numSymbols+1,true);

	numSymbols=0;
	InternalWriteContent(node,buffer.arr,numSymbols);
	buffer[numSymbols]=0;
	string=buffer.arr;

	DizzyUtility::ConvertHtmlEntities(string);
	DizzyUtility::ReplaceStrangeDash(string);
	string.Replace("\t\n","\n");
	CommonUtility::LimitRuns(string,'\x0A',2);	//Remove runs greater than 2 LFs in a row
}

//Extracts article text from parsed mediaWiki in XML
void ThreadedWriter::InternalWriteContent(const xml_node& node, char* buffer, int& counter)
{
	using namespace pugi;

	//node_pcdata is printable
	if(node.type()==node_pcdata)
	{
		InsertIntoWriteBuffer(node.value(),buffer,counter);
		return;
	}

	//if it is not an element node at this point, it is not printable
	if(node.type()!=node_element && node.type()!=node_document) return;
	BString name=node.name();

	//If it is a page, write the title, type, and whether it's a list, unless we aren't marking articles
	if(name=="page")
	{
		BString title = node.child("title").first_child().value();

		BString string;
		if(fMarkArticles)
		{
			BString type = node.attribute("type").value();
			if(type == "disambig") type = "disambiguation";
			if(type == "article") type = "regular article";

			BString list = node.attribute("list").value();
			if(list == "yes") type = "list article";

			string = BString("\n\n#Article: ") + title +
							"\n#Type: " + type + "\n\n";
		}
		else string = BString("\n\n") + title + "\n\n";

		InsertIntoWriteBuffer(string,buffer,counter);
	}

	if(name == "section")
	{
		BString subtitle;
		WriteContentToString(node.child("secTitle"),subtitle);

		BString string;
		if(fMarkSections)
		{
			BString level = node.attribute("level").value();
			string = BString("\n\n#Subtitle level ") + level + ": " + subtitle + "\n\n";
		}
		else string = BString("\n\n") + subtitle + "\n\n";

		InsertIntoWriteBuffer(string,buffer,counter);

		InternalWriteContent(node.child("secContent"),buffer,counter);
		return;
	}

	//some element nodes are ignored completely
	if(name=="template" || name=="interwiki" || name=="wTable" || name=="media" || name=="category") return;
	if(name=="style" || name=="url" || name=="title") return;

	//if it is a link, print the anchor text
	if(name=="link" || name=="extLink") {InternalWriteContent(node.child("anchor"), buffer, counter);return;}

	//if it is an image, print the caption if we are including them
	if(name=="file")
	{
		if(!fSkipImCaptions)
		{
			BString caption;
			WriteContentToString(node.child("caption"),caption);

			if(caption == "") return;

			BString string = "\n\n";
			if(fMarkCaptions) string += "#Caption: ";
			string += caption;
			string += "\n\n";
			InsertIntoWriteBuffer(string,buffer,counter);
		}
		return;
	}

	//If it is a listEl, insert a tabulation before, a double LF after, and do nothing if it's empty
	if(name=="listEl") InsertIntoWriteBuffer('\t',buffer,counter);

	//all other types of nodes, iterate over the children
	xml_node child=node.first_child();
	while(child)
	{
		InternalWriteContent(child, buffer, counter);
		child=child.next_sibling();
	}

	//If it was a par or a listEl, add a double LF, unless the buffer is empty
	if(name=="par" || name=="listEl")
	{
		if(counter!=0) InsertIntoWriteBuffer("\n\n", buffer, counter);
	}
}

//Private helper for content writer
void ThreadedWriter::InsertIntoWriteBuffer(char symbol, char* buffer, int& counter)
{
	if(buffer!=NULL) {buffer[counter]=symbol;}
	counter++;
}

//Private helper for content writer
void ThreadedWriter::InsertIntoWriteBuffer(const BString& string, char* buffer, int& counter)
{
	int length = string.GetLength();
	if(buffer!=NULL){memcpy(buffer+counter,string,length);}
	counter+=length;
}

void ThreadedWriter::Report(std::ostream& stream)
{
	stream << "***Wiki Parser: converting parsed XML into plain text***\n\n";

	stream << "Processing started: " << startTimeString << "\n";
	stream << "Processing ended: " << CommonUtility::CurDateTimeString("%Y-%m-%d %H:%M:%S") << "\n";
	
	int h, m, s;
	CommonUtility::SecondsToHMS((int)timer.GetCurTime(0),h,m,s);
	stream << "Processing took a total of: " << h << " hours, " << m << " minutes, " << s << " seconds.\n\n";


	stream << "The following settings were used during the conversion:\n";

	if(fMarkArticles) stream << "\t\tMarking articles with #Article tags.\n";
	else stream << "\t\tNOT marking articles with #Article tags.\n";

	if(fMarkSections) stream << "\t\tMarking sections with #Subtitle tags.\n";
	else stream << "\t\tNOT marking sections with #Subtitle tags.\n";

	if(fSkipImCaptions) stream << "\t\tSkipping image captions.\n";
	else stream << "\t\tIncluding image captions in plain text.\n";

	if(!fSkipImCaptions)
	{
		if(fMarkCaptions) stream << "\t\tMarking image captions with #Caption tags.\n";
		else stream << "\t\tNOT marking image captions with #Caption tags.\n";
	}

	stream << "\n";

	stream << "Number of pages from the parsed XML written as plain text: " << numPagesWritten << ".\n";
}