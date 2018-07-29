The following files have been generated during the parse:

article_titles.txt: the list of titles for the articles that were extracted from the Wikipedia database file and saved into XML and plain text.

articles_in_plain_text.txt: the plain text of the parsed articles. The number of articles in this file, and the ordering of the articles is the same as in "article_titles.txt"

articles_in_xml.xml: the parsed XML of the articles from the Wikipedia database file. This file is intended to be parsed by XML processors and isn't intended to be human-readable. The number of articles and their ordering is the same as in "article_titles.txt". The "xml_schema.txt" file contains a brief description of the XML schema used.

parse_report.txt: a short report put together by the parser. You can look through it to verify that the parse went as expected.

redirects.txt: in addition to parsing articles, the parser also extracts redirect information from the Wikipedia database file. The redirects are saved as "Redirect from title" --> "Redirect to title" pairs.

xml_schema.txt: shows the XML schema for the parsed pages in "articles_in_xml.xml"