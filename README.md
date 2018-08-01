## Wiki Parser: a high-performance data extractor for English Wikipedia

Wiki Parser is a high-performance parser designed to make Wikipedia more accessible to data mining and text analysis. Wikipedia is written in a fairly tricky formatting language called [MediaWiki](https://www.mediawiki.org/wiki/Help:Formatting); Wiki Parser converts the MediaWiki-formatted Wikipedia pages into regular human-readable text and standard XML.

In the XML output, each Wikipedia page is represented as an Abstract Syntax Tree (AST) that preserves the page structure (title, first paragraph, sections and their headings) and elements such as links, image references, infobox templates, and so on.

While many Wikipedia parsers [exist](https://www.mediawiki.org/wiki/Alternative_parsers), Wiki Parser is in a class of its own performance-wise. It can parse a complete dump of English Wikipedia (66 GB uncomressed as of July 2018) into plain text and XML in about 2-3 hours on a modern machine, which is is 10-100 times faster than other parsers. The speed advantage is largely due to multithreaded C++ code with only sparse use of regexes.

64-bit Windows installer is available from the [author's page](https://dizzylogic.com/wiki-parser), which also has some additional information on using Wiki Parser:

**[Wiki Parser installer for Windows (64-bit)](https://dizzylogic.com/wiki-parser)**

![Wiki Parser main window](Wiki_Qt_Parser/Wiki_Parser_main_window.png?raw=true "Wiki Parser main window")


## Getting Started
To build this source code on your machine, clone the repository:

`git clone https://github.com/dizzylogicc/WikiParser`

Wiki Parser source code has five dependencies on outside libraries, of which two need to be downloaded separately. Here's the full list of its dependencies:
* [Qt5](https://www.qt.io/download): Wiki Parser employs Qt for user interface (Core, Gui and Widgets modules). Needs to be [downloaded separately](https://www.qt.io/download).
* [Boost C++ libraries](https://www.boost.org/): filtering streambuffer for bz2 decompression, threads and mutexes. Needs to be [downloaded separately](https://www.boost.org/users/download/). Boost version 1.58.0 is recommeded for Visual Studio 2013 since that compiler version chokes on newer Boost releases. 
* [Re2](https://github.com/google/re2) - a fast regex library used in the parser routines. Included with source code.
* [HTML Tidy](http://www.html-tidy.org/) - a library that fixes errors in HTML and XML formatting. Included with source code.
* [pugixml](https://pugixml.org/) - fast, lightweight XML processing library. Included with source code.

If you don't have them already, go ahead and download the Qt5 and Boost binaries appropriate for your machine and operating system.


### Building with Visual Studio (Windows)
This is the tried-and-tested build variant for Wiki Parser. You'll need Visual Studio 2013 or newer with the [Qt plugin](http://doc.qt.io/archives/vs-addin/index.html) installed. Follow these steps to build the project:

* Open the solution file `Wiki_Qt_Parser.sln` with Visual Studio.
* Point the project to where Qt5 is located on your system (`Qt5 menu -> Qt Options` and `Qt5 menu -> Qt Project Settings`).
* Set the build configuration to "Release" and "x64" (`Build -> Configuration manager`).
* Set the *include* and *library* directories for your Boost installation: select the `Wiki_Qt_Parser project`, go to `Project -> Wiki_Qt_Parser properties -> Configuration properties -> VC++ directories`. Edit the `Include Directories` and `Library Directories` lines to add paths to Boost.
* Build the `Re2` and `Tidy` projects. Then build the `Wiki_Qt_Parser` project.
* The executable produced by the build needs all the files from the `package_with_executable` directory to run. Place those files in the same directory as the executable. In addition, in order to start independently from the build environment, the executable will need the Qt DLLs and platform files packaged with it per usual Qt requirements. Have a look at the directory structure created by the [installer package](https://dizzylogic.com/wiki-parser) for exact file names.


### Building with Qt Creator (Linux/Mac OS/Windows)
You should be able to build this project with Qt Creator as well (for all three operating systems). This has been done under Mac OS for an earlier version of Wiki Parser, though not for the latest version. You'll need to create proper project files for Qt Creator if you are willing to try. There are auto-generated .pri and .pro files included with the source code that can serve as a starting point.


## How it works 
It's probably a good idea to give a high-level overview of the code. The actual parsing of the MediaWiki-formatted Wikipedia pages occurs in the `CWikipediaParser` class. The class named `ThreadedWrapper` is a multithreaded wrapper. It spawns as many threads as requested by the user, with each thread creating its own thread-local `CWikipediaParser`. The constructor for `CWikipediaParser` is given a file name for the file containing serialized parser parameters (basically, the serialized contents of the `Parser data files` directory). This is the `pdata.cfg` file in the repository.

Raw data is extracted from the bz2-compressed Wikipedia dump with a Boost filtering streambuffer. It is then split into pages (in the form of strings), which in turn are fed to the worker threads of the `ThreadedParser` where the `CWikipediaPaser::Parse` function is called. The parser then processes each page and outputs an XML document object (`pugixml::xml_document`). These XML documents are printed into strings again (with all XML tags) and continuously written to the XML file in a thread-safe manner.

Once all the pages from the Wikipedia dump have been parsed, a `ThreadedWriter` class starts its work. It only launches one worker thread, which is actually sufficient because there isn't much heavy lifting left to do at this point. `ThreadedWriter` reads the XML pages from the disk file written by the `ThreadedParser` and converts them into plain text. 


## License

This project is licensed under the MIT License. See the LICENSE file for details.
