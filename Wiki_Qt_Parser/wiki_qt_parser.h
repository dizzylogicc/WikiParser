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

#ifndef WIKI_QT_PARSER_H
#define WIKI_QT_PARSER_H

#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include <QTimer>
#include "Timer.h"
#include "ui_wiki_qt_parser.h"

#include "WpSavable.h"

#include "PageIndex.h"

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

#include "ThreadedParser.h"
#include "ThreadedWriter.h"
#include <sstream>

class Wiki_Qt_Parser : public QMainWindow
{
	Q_OBJECT

public:
	Wiki_Qt_Parser(QWidget *parent = 0);
	~Wiki_Qt_Parser();

public:
	

private:
	//This must be the first declaration  - no data before this declaration
	//The directory where the executable file is located - needed for access to files in that dir
	BString exeDir;

	//All modifiable data in the window - all options and file names
	WpSavable savable;

	//File names used in operation, except the input file and the output directory
	BString savableFile;
	BString xmlFile;
	BString iiaFile;
	BString pageIndexFile;
	BString plainTextFile;
	BString reportFile;
	BString redirectFile;
	BString artTitleFile;

private:
	//"Devices" used during the parse
	QTimer* timer;
	ThreadedParser parser;
	ThreadedWriter writer;
	std::ostringstream parserReport;
	std::ifstream instream;
	boost::iostreams::filtering_streambuf<boost::iostreams::input> streambuf;
	CTimer stopwatch;

	//State machine for the parse
	bool parserRunning;
	bool writerRunning;
	volatile bool stopFlag;
	
	//Run params
	BString directory;			//The directory is stored in case the user decides to change it while the parse is in progress
	int numArtsInTest;
	bool fTestRun;				//Flag to indicate that the current run is a test run
	int numArtsInProgBar;		//The number of pages 
	int pagesToWrite;			//Once the parse has completed, we know how many pages we have to write as plain text

	//Number of cores to use settings
	int numCores;
	int numCoresMinOne;
	int numOtherCoresDefault;

	//Auxilliary vars for parser stat output
	int numDotsInProg;

private:
	QString diskSpaceText;
	QString testDiskSpaceText;

private slots:
	void BnSelectInputClicked();
	void BnSelectOutputDirClicked();
	void BnStartClicked();
	void OnTimer();
	void SetDiskSpaceText();
	void SkipImageCaptionsClicked();
	void AboutMenuItemClicked();
	void HowToUseMenuItemClicked();
	void LicenseMenuItemClicked();
	void GoToFolderClicked();
	void EnsureNonemptyOtherCores();
	
private:
	void SetNumCoreTexts();
	void OnTimerParserRunning();
	void OnTimerWriterRunning();
	void OnTimerFinalize();
	void ProcessPageIndex(PageIndex& index);

private:
	//Load and save all savable data from the dialog
	void Load();
	void Save();
	void ShowInputFile();
	void ShowOutputDir();

private:
	Ui::Wiki_Qt_ParserClass ui;
};

#endif // WIKI_QT_PARSER_H
