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

#include "Array.h"

//Just a collection of common functions without any object data
//Should be a namespace...
class CCommon
{
public:
	CCommon(void){};
	~CCommon(void){};

public:
	int ToInt(BString& string);
	BString ToString(int val);

	long long atoll(const char* string);	//converts a string to long long int (like atoi())

	unsigned int IPStringToUINT(const BString& IP);		//converts a BString containing an IP address into UINT

	BString GetExtension(const BString& fileName);	//Extracts file extension from the file name
	BString ByteArrayToHexString(const CHArray<char>& theArr);	//Converts an array of bytes to a hex string

	void ShowInt(long long val, BString comment="");
	void ShowFloat(double val, BString comment="");
	void ShowString(BString string, BString comment="");

	bool WriteString(const BString& string, const BString& fileName);
	bool ReadString(BString& string, const BString& fileName);

	void ReplaceInString(const char* oldString, char* newString, const char* toReplace, const char* replacement);
	void Replace(BString& text, const BString& toReplace, const BString& replacement);
	int ProcessBracketedByStrings(char* oldString, char* newString, char* left, char* right, char* altRight, bool fKeepOrRemove);
	int KeepBracketed(char* oldString, char* newString, char* left, char* right);
	void RemoveBracketed(char* string, char left, char right);
	int AddStringToCharPointer(char* pointer, int startFrom, BString string);
	void RemoveBracketed(BString& string, char left, char right);
	void RemoveBracketedByStrings(BString& string, BString left, BString right, BString altRight="");
	BString GetBracketed(BString& string, BString leftStr, BString rightStr);
	void RemoveUsingMarkup(BString& string, CHArray<char>& markup);
	BString RemoveAfterAndIncluding(BString label, BString string);
	bool BoundedStringCompare(char* str1, char* str2);

	int64 FindInString(const char* string, const char* snippet, int64 startPos, int64 stringSize);

	BString MakeNounPlural(BString noun);
	BString MakeAdverbFromAdj(BString adj);
	BString MakeErAdj(BString adj);
	BString MakeEstAdj(BString adj);
	BString MakeLessAdj(BString word);
	BString MakeNessNoun(BString word);
	bool IsNumber(unsigned char symbol) {return (symbol>='0' && symbol<='9');};
	int NumVowelsInWord(BString word);

	bool IsVowel(char a);
	bool IsHardConsonant(char a);
	bool IsConsonant(char a) {return !IsVowel(a);};
	bool EndsInSibilant(BString word);
	bool WriteSparseMatrix(const BString& fileName, CHArray<int> from, CHArray<int> to, CHArray<double> val);
};
