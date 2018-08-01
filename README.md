## Wiki Parser: a high-performance data extractor for English Wikipedia

Wiki Parser is a high-performance parser designed to make Wikipedia more accessible to data mining and text analysis. Wikipedia is written in a formatting language called [MediaWiki](https://www.mediawiki.org/wiki/Help:Formatting); Wiki Parser converts the MediaWiki-formatted Wikipedia pages into regular human-readable text and standard XML.

In the XML output, each Wikipedia page is represented as an Abstract Syntax Tree (AST) that preserves the page structure (title, first paragraph, sections and their headings) and elements such as links, image references, infobox templates, and so on.

While many Wikipedia parsers [exist](https://www.mediawiki.org/wiki/Alternative_parsers), Wiki Parser is in a class of its own performance-wise. It can parse a complete dump of English Wikipedia (66 GB uncomressed as of July 2018) into plain text and XML in about 2-3 hours on a modern machine, which is is 10-100 times faster than other parsers. The speed advantage is largely due to multithreaded C++ code with sparse use of regexes.

64-bit Windows installer is available from the [author's page](https://dizzylogic.com/wiki-parser), which also has some additional information on using Wiki Parser:

**[Wiki Parser installer for Windows (64-bit)](https://dizzylogic.com/wiki-parser)**

## Getting Started
Wiki Parser source code has five dependencies on third-party libraries:
* [Qt5](https://www.qt.io/download): Wiki Parser employs Qt for user interface (Core, Gui and Widgets modules). Needs to be [downloaded separately](https://www.qt.io/download).
* [Boost C++ libraries](https://www.boost.org/): filtering streambuffer for bz2 decompression, threads and mutexes. Needs to be [downloaded separately](https://www.boost.org/users/download/). Boost version 1.58.0 is recommeded for Visual Studio 2013 since that compiler version chokes on newer Boost releases. 
* [Re2](https://github.com/google/re2) - a fast regex library used in the parser routines. Included with source code.
* [HTML Tidy](http://www.html-tidy.org/) - a library that fixes errors in HTML and XML formatting. Included with source code.
* [pugixml](https://pugixml.org/) - fast, lightweight XML processing library. Included with source code.

Therefore the steps needed to get this project up and running on your local machine with Visual Studio are as follows:

* Clone this project: git clone https://github.com/dizzylogicc/PanoTwist
    Download Qt5, if you don't have it already. Downloading Qt5 binaries is much simpler than downloading the sources and compiling them. The binaries must match your Visual Studio version and the runtime library (multi-threaded, multi-threaded DLL, etc.).
    Download OpenCV and Exiv2. Here too, downloading the binary files is simpler than downloading and compiling the sources.
    Open the PanoTwist.sln file with Visual Studio 2013 (or later) with the Qt plugin installed.
    Point the project to where Qt5 is located on your system (Qt5 menu -> Qt Options and Qt5 menu -> Qt Project Settings).
    Specify the include directories for Qt5, OpenCV and Exiv2 (Project menu -> Properties -> Configuration Properties -> VC++ Directores -> Include directories).
    Add the following libraries for OpenCV and Exiv2 support to the project (Project -> Add existing item): libexiv2.lib, libexpat.lib, opencv_world310.lib, xmpsdk.lib, zlib1.lib. Note that library names may differ somewhat on your system. If these libraries are already among the project files, remove them from the project first.
    Build the project!

License

This project is licensed under the MIT Open Source License. See LICENSE.txt file for details.
