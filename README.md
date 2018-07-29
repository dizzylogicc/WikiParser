## Wiki Parser: a high-performance data extractor for English Wikipedia

Wiki Parser is a high-performance parser designed to make Wikipedia more accessible to data mining and text analysis. Wikipedia is written in a formatting language called [MediaWiki](https://www.mediawiki.org/wiki/Help:Formatting); Wiki Parser converts the MediaWiki-formatted Wikipedia pages into regular human-readable text and standard XML.

In the XML output, each Wikipedia page is represented by an Abstract Syntax Tree (AST) that preserves the page structure (title, first paragraph, sections and their headings) and elements such as links, image references, infobox templates, and so on.

While many Wikipedia parsers [exist](https://www.mediawiki.org/wiki/Alternative_parsers), Wiki Parser
