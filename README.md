## Wiki Parser: a high-performance data extractor for English Wikipedia

Wiki Parser is a high-performance parser designed to make Wikipedia more accessible to data mining and text analysis. Wikipedia is written in a formatting language called [MediaWiki](https://www.mediawiki.org/wiki/Help:Formatting); Wiki Parser converts the MediaWiki-formatted Wikipedia pages into regular human-readable text and standard XML.

In the XML output, each Wikipedia page is represented by an Abstract Syntax Tree (AST) that preserves the page structure (title, first paragraph, sections and their headings) and elements such as links, image references, infobox templates, and so on.

While many Wikipedia parsers [exist](https://www.mediawiki.org/wiki/Alternative_parsers), Wiki Parser is in a class of its own performance-wise. It can parse a complete dump of English Wikipedia (66 GB uncomressed as of July 2018) into plain text and XML in about 2-3 hours on a modern machine, which is is 10-100 times faster than other parsers. The speed advantage is largely due to multithreaded, mostly regex-free C++ code.

A 64-bit Windows installer is available from the [author's page](https://dizzylogic.com/wiki-parser), which also has some additional information on using Wiki Parser:

**[Wiki Parser installer for Windows (64-bit)](https://dizzylogic.com/wiki-parser)**

## Getting Started

