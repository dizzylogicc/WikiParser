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

#include "wiki_qt_parser.h"
#include <QMessageBox>
#include <QThread>
#include <QDesktopServices>
#include <QUrl>
#include "QtUtils.h"

#include "aboutdialog.h"
#include "howtousedialog.h"
#include "licensedialog.h"

#include "CommonUtility.h"

Wiki_Qt_Parser::Wiki_Qt_Parser(QWidget *parent)
	: QMainWindow(parent),
	exeDir( (qApp->applicationDirPath() + "/").toStdString() ),
	parser(exeDir + "pdata.cfg")
{
	ui.setupUi(this);

	//The file where the settings are saved from last time
	savableFile		= exeDir + "wps.cfg";

	//Set up file names used during parsing - except input file and directory, which are stored in "savable"
	xmlFile			= "articles_in_xml.xml";
	iiaFile			= "ADiia.ari64";
	pageIndexFile	= "pindex.cust";
	plainTextFile	= "articles_in_plain_text.txt";
	reportFile		= "parse_report.txt";
	redirectFile	= "redirects.txt";
	artTitleFile	= "article_titles.txt";

	//Pass file names to parser and set some parser options
	parser.SetXmlFileName(xmlFile);
	parser.SetIiaFileName(iiaFile);
	parser.SetPageIndexFileName(pageIndexFile);

	//Prepended to XML to make it legal XML
	//We'll need to append </pages> at the end after the parsing is done
	parser.SetPrependToXML("<?xml version=\"1.0\" encoding=\"UTF-8\" ?><pages>");
	
	//QTimer and other signal-slot connections
	timer = new QTimer(this);
	connect(timer,SIGNAL(timeout()),this,SLOT(OnTimer()));

	connect(ui.bnSelectInputFile,SIGNAL(clicked()),this,SLOT(BnSelectInputClicked()));
	connect(ui.bnSelectOutputDir,SIGNAL(clicked()),this,SLOT(BnSelectOutputDirClicked()));
	connect(ui.bnStart,SIGNAL(clicked()),this,SLOT(BnStartClicked()));
	connect(ui.checkTestRun,SIGNAL(clicked()),this,SLOT(SetDiskSpaceText()));
	connect(ui.checkSkipImageCaptions,SIGNAL(clicked()),this,SLOT(SkipImageCaptionsClicked()));

	connect(ui.actionAbout,SIGNAL(triggered()),this,SLOT(AboutMenuItemClicked()));
	connect(ui.actionHowToUse,SIGNAL(triggered()),this,SLOT(HowToUseMenuItemClicked()));
	connect(ui.actionLicense,SIGNAL(triggered()),this,SLOT(LicenseMenuItemClicked()));

	connect(ui.bnGoToFolder,SIGNAL(clicked()),this,SLOT(GoToFolderClicked()));

	//Number of articles for test runs and the number to show in the progress bar
	numArtsInTest = 100;
	numArtsInProgBar = 20000;

	//Interface styles and initial interface values
	ui.progressBar->setValue(0);
	ui.progressBar->setFormat("0 pages parsed");
	qApp->setStyleSheet(	"QProgressBar {text-align: center; border: 2px solid grey; border-radius: 5px;}"
							"QProgressBar::chunk {background-color: #05B8CC; width: 5px;}"
							"QLineEdit {border: 2px solid grey; border-radius: 5px; padding-left: 5px; padding-right: 5px;}"
							"QPushButton {	border: 2px solid #8f8f91; border-radius: 3px;"
											"background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
											"stop: 0 #B7C7E8, stop: 1 #A0B7EB);"
											"min-width: 70px; min-height: 24px;}"
							"QPushButton:pressed {background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
													"stop: 0 #dadbde, stop: 1 #f6f7fa);}");

	ui.checkTestRun->setText(QString("This is a test run (only the first ") +
							QLocale(QLocale::English).toString(numArtsInTest)+
							" articles will be parsed).");

	//Hide the GoToFolder button
	ui.bnGoToFolder->setHidden(true);

	//Number of cores to use
	numCores = QThread::idealThreadCount();
	if(numCores <= 0) numCores = 2;

	numCoresMinOne = numCores - 1;
	if(numCoresMinOne <= 0) numCoresMinOne = 1;

	numOtherCoresDefault = 2;
	if(numCores < 2) numOtherCoresDefault = 1;

	SetNumCoreTexts();
	
	//The text for disk space
	testDiskSpaceText = "You are in test mode (see options above). Less than 1 GB is needed to store the test output.";
	SetDiskSpaceText();

	//Aux variable to show loading of the next data chunk into memory
	numDotsInProg = 1;

	//Parser options
	parser.SetShortReport(true);

	//Attempt to load all savable dialog data and show it in the dialog
	Load();

	//State machine
	parserRunning = false;
	writerRunning = false;
	stopFlag = false;
}

Wiki_Qt_Parser::~Wiki_Qt_Parser()
{
	//Save the options on exit
	Save();
}

void Wiki_Qt_Parser::HowToUseMenuItemClicked()
{
	auto dlg = new HowToUseDialog(this);
    dlg->exec();
}

void Wiki_Qt_Parser::LicenseMenuItemClicked()
{
	auto dlg = new LicenseDialog(this);
    dlg->exec();
}

//User wants to see the parsed files in the save directory
//Launch file explorer
void Wiki_Qt_Parser::GoToFolderClicked()
{
	if(directory == "") return;

	QString path = QDir::toNativeSeparators(directory.c_str());
	QDesktopServices::openUrl(QUrl("file:///" + path));
}

//User clicked on the SkipImageCaptions checkbox, enable or disable the MarkImageCaptions checkbox
void Wiki_Qt_Parser::SkipImageCaptionsClicked()
{
	ui.checkMarkCaptions->setEnabled( !ui.checkSkipImageCaptions->isChecked() );
}

void Wiki_Qt_Parser::AboutMenuItemClicked()
{
	auto about = new AboutDialog(this);
    about->exec();
}

//Load all savable data from the dialog
void Wiki_Qt_Parser::Load()
{
	bool res = savable.Load(savableFile);
	if(!res)		//Load unsuccessful - probably no such file, yet
	{
		//Load default options
		savable.inputFile				= "";
		savable.outputDir				= "";
		savable.checkTest				= false;
		savable.checkDiscardLists		= true;
		savable.checkDiscardDisambigs	= true;
		savable.checkDiscardCaptions	= false;
		savable.checkMarkArticles		= true;
		savable.checkMarkSections		= true;
		savable.checkMarkCaptions		= true;

		savable.radioCoresMinus1		= true;
		savable.radioAllCores			= false;
		savable.radioOtherCores			= false;
		savable.numOtherCores			= 2;
	}

	//Show all loaded options in the dialog
	ShowInputFile();
	ShowOutputDir();

	ui.checkTestRun->setChecked(savable.checkTest);
	ui.checkDiscardListPages->setChecked(savable.checkDiscardLists);
	ui.checkDiscardDisambigs->setChecked(savable.checkDiscardDisambigs);
	ui.checkSkipImageCaptions->setChecked(savable.checkDiscardCaptions);
	ui.checkMarkArticles->setChecked(savable.checkMarkArticles);
	ui.checkMarkSections->setChecked(savable.checkMarkSections);
	ui.checkMarkCaptions->setChecked(savable.checkMarkCaptions);

	ui.radioAllCoresMinOne->setChecked(savable.radioCoresMinus1);
	ui.radioAllCores->setChecked(savable.radioAllCores);
	ui.radioOtherCores->setChecked(savable.radioOtherCores);
	ui.editOtherCores->setText(QString::number(savable.numOtherCores));

	SkipImageCaptionsClicked();
}

//Save all savable data from the dialog
void Wiki_Qt_Parser::Save()
{
	//No need to worry about input file and output directory, they are already in savable
	savable.checkTest				= ui.checkTestRun->isChecked();
	savable.checkDiscardLists		= ui.checkDiscardListPages->isChecked();
	savable.checkDiscardDisambigs	= ui.checkDiscardDisambigs->isChecked();
	savable.checkDiscardCaptions	= ui.checkSkipImageCaptions->isChecked();
	savable.checkMarkArticles		= ui.checkMarkArticles->isChecked();
	savable.checkMarkSections		= ui.checkMarkSections->isChecked();
	savable.checkMarkCaptions		= ui.checkMarkCaptions->isChecked();

	savable.radioCoresMinus1		= ui.radioAllCoresMinOne->isChecked();
	savable.radioAllCores			= ui.radioAllCores->isChecked();
	savable.radioOtherCores			= ui.radioOtherCores->isChecked();
	savable.numOtherCores			= ui.editOtherCores->text().toInt();

	savable.Save(savableFile);
}

void Wiki_Qt_Parser::ShowInputFile()
{
	if(savable.inputFile == "" || !QFile(savable.inputFile.c_str()).exists())
	{
		ui.editInputFile->setText("");
		return;
	}

	QString full = savable.inputFile.c_str();
	QString shortened = full.right(80);
	if(shortened.length() != full.length()) shortened = "..." + shortened;

	ui.editInputFile->setText(shortened);

	QFile file(savable.inputFile.c_str());
	quint64 size = file.size();

	int64 spaceNeeded;
	if(savable.inputFile.Right(8) == ".xml.bz2") spaceNeeded = size*40/11/1000000000;
	else spaceNeeded = size*40/55/1000000000;

	diskSpaceText = QString("You need around %1 GB to store the output files.").arg(spaceNeeded);
	SetDiskSpaceText();
}

void Wiki_Qt_Parser::ShowOutputDir()
{
	QString full = savable.outputDir.c_str();
	QString shortened = full.right(80);
	if(shortened.length() != full.length()) shortened = "..." + shortened;

	ui.editOutputDir->setText(shortened);
}

void Wiki_Qt_Parser::SetNumCoreTexts()
{
	ui.radioAllCoresMinOne->setText(QString("All cores except one (%1 cores, recommended)").arg(numCoresMinOne));
	ui.radioAllCores->setText(QString("All cores (%1 cores)").arg(numCores));
	ui.radioAllCoresMinOne->setChecked(true);

	ui.editOtherCores->setValidator( new QIntValidator(1, numCores, this) );
	ui.editOtherCores->setText(QString("%1").arg(numOtherCoresDefault));
	ui.editOtherCores->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

	connect(ui.radioOtherCores,SIGNAL(clicked()),this,SLOT(EnsureNonemptyOtherCores()));
}

void Wiki_Qt_Parser::EnsureNonemptyOtherCores()
{
	if(ui.editOtherCores->text() == "") ui.editOtherCores->setText(QString("%1").arg(numOtherCoresDefault));
}

void Wiki_Qt_Parser::BnSelectInputClicked()
{
	QString filters("Compressed XML(*.xml.bz2);;XML files (*.xml);;All files (*.*)");
    QString defaultFilter("Compressed (*.bz2)");

	QString response = QFileDialog::getOpenFileName(this, "Select a Wikipedia database file to parse",
							"", filters,&defaultFilter);

	if(response == "") return;

	savable.inputFile = response.toStdString();
	ShowInputFile();

	Save();

	ui.bnGoToFolder->setHidden(true);
}

void Wiki_Qt_Parser::SetDiskSpaceText()
{
	if(ui.checkTestRun->isChecked()) ui.editDiskSpace->setText(testDiskSpaceText);
	else ui.editDiskSpace->setText(diskSpaceText);
}

void Wiki_Qt_Parser::BnSelectOutputDirClicked()
{
	QString response = QFileDialog::getExistingDirectory(this, "Select a directory to save the parsed data files", "");

	if(response == "") return;

	savable.outputDir = response.toStdString();
	if(savable.outputDir != "" && savable.outputDir.Right(1) != "/" && savable.outputDir.Right(1) != "\\")
		savable.outputDir += "/";

	ShowOutputDir();
	Save();

	ui.bnGoToFolder->setHidden(true);
}

void Wiki_Qt_Parser::BnStartClicked()
{
	//If it's already running, this is the Stop button
	if(parserRunning || writerRunning)
	{
		stopFlag = true;
		ui.labelParseStatus->setText("Stopping parse...");
		return;
	}

	if(savable.inputFile == "")
	{
		QMessageBox::critical(this,"Input file not specified","Please select a Wikipedia database file to parse in Step 1.");
		return;
	}

	if(!QFile(savable.inputFile.c_str()).exists())
	{
		QMessageBox::critical(this,"Input file could not be found",
						"The Wikipedia database input file in Step 1 does not exist.");
		return;
	}

	//Check intput file extension
	BString fileString = savable.inputFile;

	if(fileString.Right(8) != ".xml.bz2" && fileString.Right(4) != ".xml")
	{
		QMessageBox::critical(this,"Unknown input file type",
						"The input file type in Step 1 must be either *.xml or *.xml.bz2");
		return;
	}

	directory = savable.outputDir;

	if(directory == "")
	{
		QMessageBox::critical(this,"Output directory not specified",
								"Please select a directory to save parsed data in Step 2.");
		return;
	}

	if(!QDir(directory.c_str()).exists())
	{
		QMessageBox::critical(this,"Output directory could not be found",
								"The output directory you specified in Step 2 does not exist.");
		return;
	}

	//By now, the parser is not running and we can start it
	EnsureNonemptyOtherCores();

	//State machine
	parserRunning = true;
	writerRunning = false;
	stopFlag = false;

	ui.bnStart->setText("Stop");
	ui.progressBar->setFormat("Preparing to parse Wikipedia database file...");
	ui.progressBar->setValue(0);

	fTestRun = ui.checkTestRun->isChecked();

	//Figure out the number of threads
	int numThreads = -1;
	if(ui.radioAllCoresMinOne->isChecked()) numThreads = numCoresMinOne;
	else if(ui.radioAllCores->isChecked()) numThreads = numCores;
	else if(ui.radioOtherCores->isChecked()) numThreads = ui.editOtherCores->text().toInt();
	
	if(numThreads == -1) numThreads = numCoresMinOne;

	//How many pages to parse
	int pagesToParse = 1000000000;	//1 billion
	if(fTestRun) pagesToParse = numArtsInTest;

	//Open the streams from the input file
	instream.close();
	streambuf.reset();

	instream.open(savable.inputFile.c_str(),std::ios::binary | std::ios::in);

	if(!instream)
	{
		QMessageBox::critical(this,"Error opening input file",
								"Wiki Parser was unable to open the input file specified in Step 1.");
		return;
	}
		
    if(fileString.Right(8) == ".xml.bz2") streambuf.push(boost::iostreams::bzip2_decompressor());
    streambuf.push(instream);

	//Set max value on the progress bar
	if(fTestRun) ui.progressBar->setMaximum(numArtsInTest);
	else ui.progressBar->setMaximum(numArtsInProgBar);

	//Clear output stream
	parserReport.clear();
	parserReport.str("");

	//Set parser options
	parser.SetDiscardLists(ui.checkDiscardListPages->isChecked());
	parser.SetDiscardDisambigs(ui.checkDiscardDisambigs->isChecked());
	parser.SetWritePageIndex(false);

	//Tell the parser the input file name for reporting purposes
	parser.SetInputFileForReport(savable.inputFile);

	//Set writer options
	writer.SetSkipImCaptions(ui.checkSkipImageCaptions->isChecked());
	writer.SetMarkArticles(ui.checkMarkArticles->isChecked());
	writer.SetMarkSections(ui.checkMarkSections->isChecked());
	writer.SetMarkCaptions(ui.checkMarkCaptions->isChecked());

	//Start parse asynchronously
	parser.Parse(	&streambuf,
					numThreads,
					directory,
					parserReport,
					pagesToParse,
					false);		//Asynchronous - will launch a wrapper thread and return immediately

	//Record start time
	stopwatch.SetTimerZero(0);

	//Hide the bnGoToFolder
	ui.bnGoToFolder->setHidden(true);

	//Start timer
	timer->start(370);
}

//Show stats of the running parser and handle postprocessing of the parsed data
void Wiki_Qt_Parser::OnTimer()
{
	//Use the state machine to figure out what to do
	if(parserRunning)
	{
		if(stopFlag) parser.Stop();

		OnTimerParserRunning();
		
		if(!parser.IsRunning())		//The parser has stopped running - data is all ready
		{
			//Process data from pageIndex - redirects and article titles
			ProcessPageIndex(parser.GetPageIndex());

			//The parser was running, but has now stopped - change machine state
			parserRunning = false;
			writerRunning = true;

			//Set the maximum on progBar
			pagesToWrite = parser.NumADPagesSaved();
			ui.progressBar->setMaximum(pagesToWrite);

			//Start plain text writer
			writer.Process(	directory + xmlFile,
							directory + iiaFile,
							directory + plainTextFile);
		}
	}

	if(writerRunning)
	{
		if(stopFlag) writer.Stop();

		OnTimerWriterRunning();
		if(!writer.IsRunning()) OnTimerFinalize();	//The writer has finished writing plain text
	}
}

//The parser and writer have both finished
//We need to finalize the parse
void Wiki_Qt_Parser::OnTimerFinalize()
{
	timer->stop();

	//String with time taken
	int sec = (int)stopwatch.GetCurTime(0);
	int h, m, s;
	CommonUtility::SecondsToHMS(sec,h,m,s);
	BString timeString;
	timeString.Format("%0.2ih : %0.2im : %0.2is",h,m,s);

	//End parse message
	QString message;
	if(stopFlag) message = QString("Parse interruped at ") + timeString.c_str() + ".";
	else message = QString("Parse completed in ") + timeString.c_str() + ".";
		
	ui.labelParseStatus->setText(message);
	ui.bnStart->setText("Start again");

	//Get writer report
	writer.Report(parserReport);

	//Save report
	std::ofstream reportStream(directory + reportFile, std::ios::binary);
	BString repString = parserReport.str();
	reportStream.write(repString,repString.GetLength());

	//Write readme.txt file
	QFile::copy( (exeDir + "readme.txt").c_str() ,(directory + "readme.txt").c_str() );

	//Copy the description of the XML schema
	QFile::copy( (exeDir + "xml_schema.txt").c_str() , (directory + "xml_schema.txt").c_str() );

	//Delete intermediate files
	QFile::remove((directory + iiaFile).c_str());
	QFile::remove((directory + pageIndexFile).c_str());

	//Finilize the XML file:
	//Append the </pages> tag at the end
	{
		std::ofstream  xmlStream(directory + xmlFile, std::ios::binary | std::ios::app);	//append
		xmlStream << "</pages>";
	}

	//Show the GoToFolder button
	if(!stopFlag) ui.bnGoToFolder->setHidden(false);

	//If the parse was interrupted, set the text on the progress dialog
	if(stopFlag)
	{
		ui.progressBar->setFormat("The parse was stopped before it could be completed.");
	}

	//State machine back to init state
	parserRunning = false;
	writerRunning = false;
	stopFlag = false;
}

//When the OnTimer() function has determined that the writer is running
//Hsow running writer stats
void Wiki_Qt_Parser::OnTimerWriterRunning()
{
	ThreadedWriterStats stats;
	writer.GetCurStats(stats);

	QString message = QString("Writing as plain text: \"") + stats.lastPageTitle + "\"";
	ui.labelParseStatus->setText(message);

	ui.progressBar->setValue(stats.numPagesWritten);

	ui.progressBar->setFormat(QLocale(QLocale::English).toString(stats.numPagesWritten) + 
								" pages written as plain text");
}

//When the state machine in the OnTimer() function indicates that the parser is still running
//Show running parser stats
void Wiki_Qt_Parser::OnTimerParserRunning()
{
	ThreadedParserStats stats;
	parser.GetCurStats(stats);

	if(stats.fSpecialStatus)
	{
		QString status = stats.specialStatus.c_str();
		status += ": ";
		status += QString(numDotsInProg,QChar('#'));

		numDotsInProg += 3;
		if(numDotsInProg > 20) numDotsInProg = 1;

		ui.labelParseStatus->setText(status);
		return;
	}
	
	QString message = QString("Currently parsing: \"") + stats.lastArticle + "\"";
	ui.labelParseStatus->setText(message);
	
	int numParsed = stats.numPagesParsed;
	
	//Show real number of parsed pages for a test run
	//And indicate progress for a full run
	int progValue;
	if(!fTestRun) progValue = numParsed % numArtsInProgBar;
	else progValue = numParsed;

	ui.progressBar->setValue(progValue);
	ui.progressBar->setFormat(QLocale(QLocale::English).toString(numParsed) + " pages parsed");
}

//After parser is done, we can extract the redirects and article titles from the page index
void Wiki_Qt_Parser::ProcessPageIndex(PageIndex& index)
{
	index.artDisambigUrls.WriteStrings(directory + artTitleFile);

	index.redirectFrom += "\t\t-->\t\t";
	index.redirectFrom += index.redirectTo;

	index.redirectFrom.WriteStrings(directory + redirectFile);
}



