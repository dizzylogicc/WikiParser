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

#include "pugixml.hpp"
#include "Array.h"

using pugi::xml_parse_result;
using pugi::xml_node;
using pugi::xml_document;

namespace CommonUtility
{
	//Reads a BString from file
	bool ReadStringFromFile(BString& string, const BString& fileName);

	//Write a BString to file
	bool WriteStringToFile(BString& string, const BString& fileName);
	
	//Removes from the string the runs of "symbol" greater than n in length
	void LimitRuns(BString& string, char symbol, int n);

	//Convert seconds to hours, minutes and seconds
	void SecondsToHMS(int sec, int& h, int& m, int& s);

	//Returns current date-time string formatted per strftime() specifications
	BString CurDateTimeString(const BString& format);

	//Luhn check - for example, for credit card numbers
	//Returns true if Luhn check is correct, false otherwise
	bool LuhnCheck(const BString& digitString);

	//Converts a string to a long long integer
	long long atoll(const char* string);

	//Extract numbers from string and return them in a separate string
	BString ExtractDigits(const BString& string);

	//Returns file extension
	BString GetExtension(const BString& fileName);

	//Returns the string with capitalized first letter
	BString CapitalizeFirstLetter(const BString& string);

	//extracts the "server" part from the URL
	BString ExtractUrlRoot(const BString& url);

	//Converts number of cents to a dollar string
	//3900 -> "$39.00"
	BString CentsToDollarString(int cents);
};