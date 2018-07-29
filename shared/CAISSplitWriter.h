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

#include "boost/thread.hpp"			//Needs locking for multiple threads accessing the file and stringBuffer
#include <iostream>
#include <fstream>

//A class to write a CAIStrings to two separate files:
//a storage file (not a serialized array, just data)
//and an initial index array file (a serialized CHArray)

//By writing continuously to the data file, storage of large volumes in memory can be avoided

//Usage:
//Call Open(BString& file) to open a new storage file
//Call AddElement() as many times as needed
//Then call SaveInitIndex() when done
//And call Close() (or destroy the object)

template <class theType, class intType=int> class CAISSplitWriter
{
public:
	CAISSplitWriter(){};
	CAISSplitWriter(const BString& storageFile){Open(storageFile);};		//Will call open
	~CAISSplitWriter(void){};

public:
	//If prependToFile != "", the string is prepended to the CAIS storage file
	//And the iia is shifted by the corresponding amount
	//Correspondingly, the file and the iia can still be read with a CAISFileFetcher
	//The prepended string is not seen by the CAISSplitWriter and CAISFileFetcher at all after it has been added
	bool Open(const BString& storageFile, const BString& prependToFile = "");
	void Close();
	void Clear();

	intType Count()
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		intType res = initIndexArr.Count() - 1;
		if(res < 0) return 0;
		else return res;
	}

	bool SaveInitIndex(const BString& initIndexFile)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		return initIndexArr.Save(initIndexFile);
	};

public:
	intType StorageSize()
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		if(initIndexArr.IsEmpty()) return 0;
		else return initIndexArr.Last();
	};

//Adding elements
public:
	void AddElement(const theType* arr, intType num)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);

		storageStream.write((const char*)arr, sizeof(theType) * num);
		initIndexArr.AddAndExtend(initIndexArr.Last() + num);
	};

	template <class argIntType> void AddElement(const CHArray<theType,argIntType>& element)	//accepts different intType
	{
		boost::recursive_mutex::scoped_lock lock(mutex);

		AddElement(element.arr, (intType)element.Count());
	}

	void AddCharString(const BString& string, bool fAddZero = true)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);

		int length = string.GetLength();
		if(fAddZero) length += 1;
		AddElement((const char*)string, length);
	}
	
private:
	std::ofstream storageStream;					//storage data is dumped to disk into this stream
	CHArray<intType,intType> initIndexArr;	//initial index array is held in memory, storage is not
	boost::recursive_mutex mutex;
};

template <class theType, class intType>
void CAISSplitWriter<theType,intType>::Close()
{
	boost::recursive_mutex::scoped_lock lock(mutex);
	if(storageStream) storageStream.close();
}

template <class theType, class intType>
void CAISSplitWriter<theType,intType>::Clear()
{
	boost::recursive_mutex::scoped_lock lock(mutex);
	Close();
	initIndexArr.Clear();
}

template <class theType, class intType>
bool CAISSplitWriter<theType,intType>::Open(const BString& storageFile, const BString& prependToFile/*=""*/)
{
	boost::recursive_mutex::scoped_lock lock(mutex);

	if(storageStream) storageStream.close();
	storageStream.open(storageFile, std::ios_base::binary);

	if(!storageStream)
	{
		std::cerr << "Could not open file for writing: " << storageFile << std::endl;
		return false;
	}

	initIndexArr.Clear();

	if(prependToFile != "")
	{
		storageStream.write(prependToFile,prependToFile.GetLength());
		initIndexArr.AddAndExtend(prependToFile.GetLength());
	}
	else initIndexArr.AddAndExtend(0);

	return true;
}
