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

#include "CommonUtility.h"
#include "Array.h"
#include "Common.h"

#include <iostream>
#include <fstream>
#include <ctime>

//Reads a BString from file
bool CommonUtility::ReadStringFromFile(BString& string, const BString& fileName)
{
	std::ifstream instream(fileName, std::ifstream::in|std::ios::binary);
	if(!instream) return false;

    instream.seekg (0, instream.end);
    std::streampos length = instream.tellg();
    instream.seekg (0, instream.beg);

    char* buffer = new char[length + std::streampos(1)];
    instream.read(buffer,length);
	buffer[length]=0;

	string=buffer;
	delete[] buffer;
	return true;
}

//Write a BString to file
bool CommonUtility::WriteStringToFile(BString& string, const BString& fileName)
{
	std::ofstream outstream(fileName, std::ofstream::out|std::ios::binary);
	if (!outstream) return false;

	outstream.write(string,string.GetLength());
	return true;
}

//Removes from the string the runs of "symbol" greater than n in length
void CommonUtility::LimitRuns(BString& string, char symbol, int n)
{
	int length=string.GetLength();

	CHArray<char> buffer(length+1);
	int num=0;
	for(int i=0;i<length;i++)
	{
		char cur=string[i];

		if(cur==symbol)
		{
			if(num < n) buffer.AddPoint(cur);
			num++;
		}
		else
		{
			num=0;
			buffer.AddPoint(cur);
		}
	}
	buffer.AddPoint(0);
	string=buffer.arr;
}

//Returns current date-time string formatted per strftime() specifications
BString CommonUtility::CurDateTimeString(const BString& format)
{
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time (&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer,80,format,timeinfo);
	return buffer;
}

//Luhn check - for example, for credit card numbers
//Returns true if Luhn check is correct
bool CommonUtility::LuhnCheck(const BString& digitString)
{
	int length=digitString.GetLength();
	
	int sum=0;
	bool fDouble=false;
	for(int i=length-1;i>=0;i--)
	{
		int curDigit=digitString[i]-48;
		
		if(fDouble)
		{
			curDigit*=2;
			if(curDigit>9) curDigit-=9;
		}

		sum+=curDigit;
		fDouble=!fDouble;
	}

	return sum % 10 == 0;
}

//Convert seconds to hours, minutes and seconds
void CommonUtility::SecondsToHMS(int sec, int& h, int& m, int& s)
{
	h = sec / 3600;
	sec -= h*3600;

	m = sec / 60;
	sec -= m*60;

	s = sec;
}

//converts a decimal string to long long integer
long long CommonUtility::atoll(const char* string)
{
	long long retval;
	retval=0;
	long long sign=1;

	for (int i=0;string[i]!=NULL;i++)
	{
		char curChar=string[i];
		if(curChar<'0' || curChar>'9')
		{
			if(curChar=='-') {sign*=-1;}
			continue;
		}

		retval = 10*retval + (curChar - '0');
	}

	retval*=sign;
	return retval;
}

//Extract numbers from string and return them in a separate string
BString CommonUtility::ExtractDigits(const BString& string)
{
	BString result;
	int length=string.GetLength();
	for(int i=0;i<length;i++)
	{
		char cur=string[i];
		if(cur>='0' && cur<='9') result+=cur;
	}

	return result;
}
//Returns file extension
BString CommonUtility::GetExtension(const BString& fileName)
{
	BString extension="";
	BString copy=fileName;
	copy.Trim();
	int length=copy.GetLength();

	int i;
	for(i=length-1;i>=0 && copy[i]!='.';i--)
	{
		extension=copy[i]+extension;
	}

	if(i<0) return "";
	else return extension;
}

//Capitalizes the first letter of a string
BString CommonUtility::CapitalizeFirstLetter(const BString& string)
{
	BString result=string;
	BString firstLetter=result.Left(1);
	firstLetter.MakeUpper();
	int length=result.GetLength();
	result=firstLetter+result.Right(length-1);
	return result;
}

//extracts the "server" part from the URL
BString CommonUtility::ExtractUrlRoot(const BString& url)
{
	BString root=url;
	int urlLength=url.GetLength();

	if(url.Left(7).MakeLower()=="http://") root=url.Right(urlLength-7);
	if(url.Left(8).MakeLower()=="https://") root=url.Right(urlLength-8);

	int pos=root.FindOneOf("#?/");

	if(pos==-1) return root;

	root=root.Left(pos);
	return root;
}

//Converts number of cents to a dollar string
//3900 -> "$39.00"
BString CommonUtility::CentsToDollarString(int cents)
{
	BString result;
	result.Format("$%i.%02i",cents/100,cents%100);
	return result;
}
