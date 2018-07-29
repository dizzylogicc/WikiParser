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

#include "BString.h"

//Class that encapsulates operations on a word projection
class CWordTrace
{
public:
	CWordTrace(void);
	CWordTrace(const BString& theTrace);
	CWordTrace(const char* theTrace);
	~CWordTrace(void);

public: operator const char*() {return (const char*)string;};

public:
	int GetLength() const {return string.GetLength();};
	void MakeLower() {string.MakeLower();};

public:
	bool IsAllLetters() const;
	bool IsAllDigits() const;
	bool IsLetDigUndMinSpace() const;		//Is letters, digits, underscore, or minus
	bool IsValidParameterName() const;		//Whether it can be a parameter name in templates (excludes all punctuation, only - and _ are allowed)
	bool IsLowercase() const;
	bool StartsWithLetter() const;		//Whether it starts with a letter

public:
	BString string;
};

