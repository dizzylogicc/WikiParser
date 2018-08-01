## Wiki Parser: a high-performance data extractor for English Wikipedia

Wiki Parser is a high-performance parser designed to make Wikipedia more accessible to data mining and text analysis. Wikipedia is written in a fairly tricky formatting language called [MediaWiki](https://www.mediawiki.org/wiki/Help:Formatting); Wiki Parser converts the MediaWiki-formatted Wikipedia pages into regular human-readable text and standard XML.

In the XML output, each Wikipedia page is represented as an Abstract Syntax Tree (AST) that preserves the page structure (title, first paragraph, sections and their headings) and elements such as links, image references, infobox templates, and so on.

While many Wikipedia parsers [exist](https://www.mediawiki.org/wiki/Alternative_parsers), Wiki Parser is in a class of its own performance-wise. It can parse a complete dump of English Wikipedia (66 GB uncomressed as of July 2018) into plain text and XML in about 2-3 hours on a modern machine, which is is 10-100 times faster than other parsers. The speed advantage is largely due to multithreaded C++ code with only sparse use of regexes.

64-bit Windows installer is available from the [author's page](https://dizzylogic.com/wiki-parser), which also has some additional information on using Wiki Parser:

**[Wiki Parser installer for Windows (64-bit)](https://dizzylogic.com/wiki-parser)**


## Getting Started
To build this source code on your machine, clone this repository:

`git clone https://github.com/dizzylogicc/WikiParser`

Wiki Parser source code has five dependencies on outside libraries, of which two need to be downloaded separately. Here's the full list of its dependencies:
* [Qt5](https://www.qt.io/download): Wiki Parser employs Qt for user interface (Core, Gui and Widgets modules). Needs to be [downloaded separately](https://www.qt.io/download).
* [Boost C++ libraries](https://www.boost.org/): filtering streambuffer for bz2 decompression, threads and mutexes. Needs to be [downloaded separately](https://www.boost.org/users/download/). Boost version 1.58.0 is recommeded for Visual Studio 2013 since that compiler version chokes on newer Boost releases. 
* [Re2](https://github.com/google/re2) - a fast regex library used in the parser routines. Included with source code.
* [HTML Tidy](http://www.html-tidy.org/) - a library that fixes errors in HTML and XML formatting. Included with source code.
* [pugixml](https://pugixml.org/) - fast, lightweight XML processing library. Included with source code.

If you don't have them already, go ahead and download the Qt5 and Boost binaries appropriate for your machine and operating system.


## License

This project is licensed under the MIT Open Source License. See LICENSE.txt file for details.
