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

#include "WordTrace.h"

CWordTrace::CWordTrace(void)
{
	string="";
}

CWordTrace::CWordTrace(const BString& theTrace)
{
	string=theTrace;
}

CWordTrace::CWordTrace(const char* theTrace)
{
	string=theTrace;
}

CWordTrace::~CWordTrace(void)
{
}

bool CWordTrace::IsAllLetters() const
{
	BString lower=string;
	lower.MakeLower();
	int length=string.GetLength();

	for(int i=0;i<length;i++)
	{
		if(lower[i]<97 || lower[i]>122) return false;
	}

	return true;
}

bool CWordTrace::IsAllDigits() const
{
	int length=string.GetLength();

	for(int i=0;i<length;i++)
	{
		if(string[i]<48 || string[i]>57) return false;
	}

	return true;
}

//Whether the word is composed of digits, numbers, underscores, minuses and spaces
bool CWordTrace::IsLetDigUndMinSpace() const
{
	int length=string.GetLength();

	for(int i=0;i<length;i++)
	{
		const char cur=string[i];

		if( (cur>=97 && cur<=122) ||
			(cur>=65 && cur<=90) ||
			(cur>=48 && cur<=57) ||
			cur==95 ||
			cur==45 ||
			cur==32
			) continue;
		else return false;
	}

	return true;
}

//Whether it can be a parameter name in wikipedia templates (excludes all punctuation, only - and _ are allowed)
//All lowercase letters and digits
bool CWordTrace::IsValidParameterName() const
{
	if(GetLength()>0 && IsLowercase() && IsLetDigUndMinSpace()) return true;
	else return false;
}

bool CWordTrace::IsLowercase() const
{
	int length=string.GetLength();

	for(int i=0;i<length;i++)
	{
		const char cur=string[i];
		if( cur>=65 && cur<=90 ) return false;
	}

	return true;
}

bool CWordTrace::StartsWithLetter() const
{
	if(GetLength()==0) return false;

	char first=string[0];
	if( (first>=65 && first<=90) ||
		(first>=97 && first<=122)
		) return true;
	else return false;
}
