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



//A partial clone for Microsoft's CStringA string class
//Derived from std::string

/*IMPLEMENTED FUNCTIONS:

Append
AppendChar
Compare
Delete
Find
FindOneOf
Format
GetAt
GetLength
GetString
Insert
IsEmpty
Left
MakeLower
MakeReverse
MakeUpper
Mid
Remove
Replace
ReverseFind
Right
SetAt
SetString
SpanExcluding
SpanIncluding
Tokenize
Trim
TrimLeft
TrimRight

//Operators:

operator const char*
operator[]
operator =
operator +;
operator +=;
operator ==
operator !=
operator <
operator >
operator <=
operator >=

*/

/*FUNCTIONS THAT ARE NOT IMPLEMENTED:

AllocSysString
AnsiToOem
AppendFormat
Collate
CollateNoCase
CompareNoCase
FormatMessage
FormatMessageV
FormatV
GetEnvironmentVariable
LoadString
OemToAnsi
SetSysString


//Inherited from CSimpleString:

CopyChars
CopyCharsOverlapped
Empty
FreeExtra
GetAllocLength
GetBuffer
GetBufferSetLength
GetManager
LockBuffer
Preallocate
ReleaseBuffer
ReleaseBufferSetLength
SetManager
StringLength
Truncate
UnlockBuffer

*/

/* EXTENSIONS AND MODIFICATIONS:

1) Most functions that are void() in CString return *this as BString& in the current implementation
2) Added the function Remove(const char* str). It removes all occurrences of a string. Only a Remove(char c)
   is present in CString.
3) Added convenient functions WriteToFile(fileName) and ReadFromFile(fileName).
4) Added void Serialize() function so that the string could be saved in BArchive

*/


#pragma once
#include "BArchive.h"
#include <string>
#include <cstdarg>
#include <algorithm>
#include <vector>
#include <fstream>

class BString : public std::string
{
public:
	//Constructors
	BString() {}
	BString(const char* str) : std::string(str) {}
	BString(const std::string& str) : std::string(str) {}
	//Copy and move contstructors written by the compiler are fine

	//Get const and non-const references to element
	//CStringT returns by value?
	char& GetAt(int pos) { return at(pos); }
	const char& GetAt(int pos) const { return at(pos); }
	char& operator[](int pos) { return GetAt(pos); }
	const char& operator[](int pos) const { return GetAt(pos); }

	//Automatic conversion to const char*
	operator const char*() const { return c_str(); }

	//Assignment operators
	BString& operator = (const char* rhs) { std::string::operator=(rhs); return *this; }
	BString& operator = (const std::string& rhs) { std::string::operator=(rhs); return *this; }
	//BString to BString assignment written by the compiler is fine
	//Move assignment written by the compiler is fine

	//Operator + and += : the ones inherited from std::string return std::string!
	//New ones need to be defined
	//We'll spell out all combination of friend operator+ explicitly:
	friend BString operator+(const BString& one, const BString& two)
	{
		BString result = static_cast<const std::string&>(one) + static_cast<const std::string&>(two);
		return result;
	}
	friend BString operator+(const BString& one, const std::string& two)
	{
		BString result = static_cast<const std::string&>(one) + two;
		return result;
	}
	friend BString operator+(const BString& one, const char* two)
	{
		BString result = static_cast<const std::string&>(one) + two;
		return result;
	}
	friend BString operator+(const std::string& one, const BString& two)
	{
		BString result = one + static_cast<const std::string&>(two);
		return result;
	}
	friend BString operator+(const char* one, const BString& two)
	{
		BString result = one + static_cast<const std::string&>(two);
		return result;
	}


	//Operator += redefined for char, BString, std::string and const char* to guarrantee BString& being returned
	BString& operator+=(const BString& other){ std::string::operator+=(other); return *this; }
	BString& operator+=(const std::string& other){ std::string::operator+=(other); return *this; }
	BString& operator+=(const char* other){ std::string::operator+=(other); return *this; }
	BString& operator+=(char c){ std::string::operator+=(c); return *this; }

	BString& SetAt(int pos, char c) { (*this)[pos] = c; return *this; }

	//Length of the string
	int GetLength() const { return (int)length(); }
	bool IsEmpty() const { return length() == 0; }

	//The CString version of c_str()
	const char* GetString() const { return c_str(); }

	//The CString version of assign()
	//Checks for nullptr
	BString& SetString(const char* str) { if(str!=nullptr) assign(str); return *this; }
	BString& SetString(const char* str, int nLength) { if(str!=nullptr) assign(str, size_t(nLength)); return *this; }

	//Extension: a serialization function so that the string can be saved to BArchive
	void Serialize(BArchive& ar)
	{
		int curLength = GetLength();

		if (ar.IsLoading())
		{
			int newLength;
			ar >> newLength;

			char* buffer = new char[newLength + 1];
			ar.RetrieveArray(buffer, newLength);
			buffer[newLength] = 0;

			operator=(buffer);
			delete[] buffer;
		}
		else
		{
			ar << curLength;
			ar.StoreArray((const char*)(*this), curLength);
		}
	}

	//Comparison: < 0 if this string compares as "less" to the provided string
	// >0 if it compares "greater"
	// 0 if equal
	//It's opposite to std::string comparison
	int Compare(const char* other) const {return -compare(other);}

	//Appending other strings and a character
	BString& Append(const BString& other) { append(other); return *this; }
	BString& Append(const char* str, int nLength) { append(str, size_t(nLength)); return *this; }
	BString& Append(const char* str) { append(str); return *this; }
	BString& AppendChar(char c){ *this += c; return *this; }

	//Converting to lower and upper case
	//Unlike the void CString functions, these ones return the referenct to itself
	BString& MakeUpper() { for (auto &c : *this) c = toupper((unsigned char)c); return *this; }
	BString& MakeLower() { for (auto &c : *this) c = tolower((unsigned char)c); return *this; }

	//Getting left, right and middle portions of the string
	BString Left(int nCount) const
	{
		if (nCount < 0) nCount = 0;
		return substr(0, nCount);
	}

	BString Right(int nCount) const
	{
		int startPos = length() - nCount;
		if (startPos < 0) startPos = 0;
		if (nCount < 0) nCount = 0;
		return substr(startPos, nCount);
	}

	BString Mid(int iFirst, int nCount) const
	{
		if (iFirst >= length() || iFirst < 0) return BString();
		if (nCount < 0) nCount = 0;
		return substr(iFirst, nCount);
	}

	BString Mid(int iFirst) const
	{
		if (iFirst >= length() || iFirst < 0) return BString();
		return substr(iFirst);
	}

	//String reversal
	//Returns a reference to itself
	BString& MakeReverse() { std::reverse(begin(), end()); return *this; }

	//Finds a character in the string
	//Returns the position, or -1 if not found
	int Find(char c, int iStart = 0) const
	{
		size_t res = find(c, iStart);
		if (res == std::string::npos) return -1;
		else return int(res);
	}

	//Finds the first occurrence of a substring in the string
	//Returns the position, or -1 if not found
	int Find(const char* str, int iStart = 0) const
	{
		size_t res = find(str, iStart);
		if (res == std::string::npos) return -1;
		else return int(res);
	}

	//Find the first occurrence of any symbol from str
	//Returns the position, or -1 if not found
	int FindOneOf(const char* symbols, int iStart = 0) const
	{
		size_t res = find_first_of(symbols, iStart);
		if (res == std::string::npos) return -1;
		else return int(res);
	}

	//Finds the last occurrence of a character in the string
	//Return the position, or -1 if not found
	int ReverseFind(char c)
	{
		size_t res = find_last_of(c);
		if (res == std::string::npos) return -1;
		else return int(res);
	}

	//Delete characters by range
	//Return new length
	int Delete(int iIndex, int nCount = 1) { erase(size_t(iIndex), size_t(nCount)); return length(); }

	//Remove all occurrences of a character
	//Return number of removed characters
	int Remove(char c)
	{
		int initLength = length();
		erase(std::remove(begin(), end(), c), end());
		return initLength - length();
	}

	//Remove all occurrences of a string
	//Return the number of removed ocurrences
	//This function is absent in CString, but should have been there
	int Remove(const char* str)
	{
		//No new allocation is needed because the new string is not larger than the old string

		//What is the size of the string being removed
		size_t strSize = CSize(str);

		//Is it empty?
		if (strSize == 0) return 0;

		//Test for a match between str and this string at every i
		size_t numRemoved = 0;
		size_t curPos = 0;		//current write position
		for (size_t i = 0; i < length(); i++)
		{
			//Do we have a match at i?
			size_t j;
			for (j = 0; str[j] != 0 && i+j < length(); j++) if (str[j] != at(i + j)) break;
			
			if (str[j] == 0)		//It's a match; just skip, don't copy
			{
				i += (strSize - 1);
				numRemoved++;
				continue;
			}
			else  //No match; copy if necessary
			{
				if (i != curPos) at(curPos) = at(i);
				curPos++;
			}
		}

		erase(begin() + curPos, end());
		return numRemoved;
	}

	//Replace all ocurrences of a char by another char
	//Return the number of replacements
	int Replace(char chOld, char chNew)
	{
		int numReplaced = 0;
		for (auto &cur : *this)
		{
			if (cur == chOld) {	cur = chNew; numReplaced++;	}
		}
		return numReplaced;
	}

	//Replace all occurrences of a string by another string
	//Return the number of replacements
	int Replace(const char* oldStr, const char* newStr)
	{
		//Low-level manipulation to avoid potential multiple reallocations
		size_t oldSize = CSize(oldStr);
		size_t newSize = CSize(newStr);

		//Prelim checks
		if (oldSize == 0) return 0;
		if (newSize == 0) return Remove(oldStr);		//No need for reallocations - quicker

		//Find where the matches (occurrences of oldStr) are
		std::vector<size_t> ocVec;

		size_t startPos = 0;
		size_t pos;
		while ( (pos = find(oldStr, startPos)) != std::string::npos)
		{
			ocVec.push_back(pos);
			startPos = pos + oldSize;
		}

		size_t numMatches = ocVec.size();
		if (numMatches == 0) return 0; //no replacements to be done

		//Size of the temporary array
		size_t tempStringSize = length() + (newSize - oldSize) * numMatches + 1;
		char* temp = new char[tempStringSize];

		size_t curPos = 0;	//current write position

		//Copy the beginning of the string
		for (size_t i = 0; i < ocVec[0]; i++) temp[curPos++] = at(i);

		//add an artificial match at the end so that the last match can be handled the same way as other ones
		ocVec.push_back(length()); 
		for (size_t i = 0; i < numMatches; i++)
		{
			//copy the new string
			for (size_t j = 0; j < newSize; j++) temp[curPos++] = newStr[j];
			//copy the string in between matches
			for (size_t k = ocVec[i] + oldSize; k < ocVec[i + 1]; k++) temp[curPos++] = at(k);
		}

		//Terminating zero
		temp[tempStringSize - 1] = 0;

		*this = temp;
		delete[] temp;

		return int(numMatches);
	}

	//Extract characters from the string that are NOT in the provided set
	//Return them as a new BString
	BString SpanExcluding(const char* set) const { return InternalSpanExtraction(set, false); }

	//Extract characters from the string that ARE in the provided set
	//Return them as a new BString
	BString SpanIncluding(const char* set) const { return InternalSpanExtraction(set, true); }

	//Insert a character or string by position
	int Insert(int iIndex, char c) { insert(size_t(iIndex), 1, c); return length(); }
	int Insert(int iIndex, char* str) { insert(size_t(iIndex), str); return length(); }

	//Tokenizer function
	//Returns next token in string, assuming tokens are separated by characters in delims
	//Order of delimiter characters is not important
	//Skips leading delimiters, then copies the token to return value
	//Returns empty string if no more tokens are present or end of string is reached
	//Starts searching at position iStart
	//Updates iStart to the position past the delimiter character that follows the token
	//Or sets the iStart to -1 if end of string is reached
	BString Tokenize(const char* delims, int& iStart) const
	{
		if (iStart == -1) return "";

		//Housekeeping vars to avoid type conversions, hopefully
		int intLength = GetLength();
		const char* buffer = c_str();

		//Skip leading delimiters
		while ( iStart < intLength && CCharPresent(delims, buffer[iStart]) ) ++iStart;

		//Have we hit the end of string?
		if (iStart >= intLength) { iStart = -1; return ""; }

		//The initial position and the length of the substring we plan to return
		int tokenStart, tokenLength;

		//We have found the token start by now
		tokenStart = iStart;
		++iStart;

		//Run through the token characters
		while (iStart < intLength && !CCharPresent(delims, buffer[iStart])) ++iStart;

		//We have either hit a delimiter or end of string - we know where the token ends
		tokenLength = iStart - tokenStart;

		//Move the iStart by one more symbol, check for end of string and return the token substring
		++iStart;
		if (iStart >= intLength) iStart = -1;

		return Mid(tokenStart, tokenLength);
	}

	//Trimming:
	//Trim(char c) - trim leading and trailing occurrences of a single character
	//Trim(const char* str) - leading and trailing, all chars in the str
	//Trim() - trim whitespace, " \n\r\t\v\f"

	BString& Trim(char c){ TrimLeft(c); TrimRight(c); return *this; }
	BString& Trim(const char* str = " \n\r\t\v\f") { TrimLeft(str); TrimRight(str); return *this; }

	BString& TrimLeft(char c)
	{
		BString str; str += c;
		TrimLeft(str.c_str());
		return *this;
	}

	BString& TrimRight(char c)
	{
		BString str; str += c;
		TrimRight(str.c_str());
		return *this;
	}

	BString& TrimLeft(const char* str = " \n\r\t\v\f")
	{
		int startPos = 0;
		const char* buffer = c_str();
		while (startPos < GetLength() && CCharPresent(str, buffer[startPos])) startPos++;

		if (startPos > 0) *this = Right(GetLength() - startPos);
		return *this;
	}

	BString& TrimRight(const char* str = " \n\r\t\v\f")
	{
		int endPos = GetLength() - 1;
		const char* buffer = c_str();
		while (endPos >= 0 && CCharPresent(str, buffer[endPos])) endPos--;

		if (endPos < (GetLength() - 1)) *this = Left(endPos + 1);
		return *this;
	}
	
	//Format function
	BString& Format(const char* format, ...)
	{
		//Double pass string formatting: first time to figure out buffer size, second time to write
		va_list args;
		va_start(args, format);

		size_t bufSize = FormatBufferSize(format, args);	//First pass - does not consume args
		char* buffer = new char[bufSize];

		vsnprintf(buffer, bufSize, format, args);
		va_end(args);

		*this = buffer;
		delete[] buffer;

		return *this;
	}

	//Extension - read string from file
	bool ReadFromFile(const BString& fileName)
	{
		std::ifstream instream(fileName, std::ifstream::in | std::ios::binary);
		if (!instream) return false;

		instream.seekg(0, instream.end);
		std::streampos length = instream.tellg();
		instream.seekg(0, instream.beg);

		char* buffer = new char[length + std::streampos(1)];
		instream.read(buffer, length);
		buffer[length] = 0;

		*this = buffer;
		delete[] buffer;
		return true;
	}

	//Extension - output string to file
	bool WriteToFile(const BString& fileName) const
	{
		std::ofstream outstream(fileName, std::ofstream::out | std::ios::binary);
		if (!outstream) return false;

		outstream.write(c_str(), length());
		return true;
	}

protected:
	//Figures out what size is needed for a format buffer
	//By running vsnprintf with a nullptr
	size_t FormatBufferSize(const char* format, va_list args) const
	{
		int res;
		va_list listCopy;
		va_copy(listCopy, args);

		//Retval will be the required number of characters not counting the terminating '\0'
		res = vsnprintf(nullptr, 0, format, listCopy);
		va_end(listCopy);
		
		//Account for the terminating '\0'
		if (res < 0) res = 0;
		return size_t(res + 1);
	}

	//Determine the c-type string size
	inline size_t CSize(const char* str) const
	{
		size_t res = 0;
		while (str[res] != 0) res++;
		return res;
	}

	//Whether a char is present in a C-type string 
	inline bool CCharPresent(const char* str, char c) const
	{
		for (size_t i = 0; str[i] != 0; ++i) if (str[i] == c) return true;
	
		return false;
	}

	//Span extraction implemented with remove_if
	BString InternalSpanExtraction(const char* set, bool fIncluding) const
	{
		BString res(*this);
		BString tempSet(set);

		auto newEnd = std::remove_if(res.begin(), res.end(),
			[&tempSet, fIncluding](char c)->bool
			{
				return (tempSet.Find(c) == -1) == fIncluding;
			}
		);

		res.erase(newEnd, res.end());
		return res;
	}
};