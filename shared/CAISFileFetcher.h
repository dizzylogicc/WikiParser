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
#include "fseek_large.h"
#include "Array.h"
#include "boost/thread.hpp"			//Needs locking for multiple threads accessing the file and stringBuffer

//Class for rapid access to CAIS
//Without loading the CAIS storage array into memory

//When constructor is called with a single file name,
//It accesses a serialized CAIStrings<theType,intType> file on disk
//Relies on the following CAIS file serialization:
//storageArr first, initIndexArr second

//When with two file names, accesses a CAIS file stored separately:
//Storage data (plain data, not a serialized CHArray) and the initial index array (serialized)

//Can read about 4000 fragments per second on an SSD

template <class theType, class intType=int> class CAISFileFetcher
{
public:
	CAISFileFetcher(const BString& fileName);										//Access to a serialized CAIS file
	CAISFileFetcher(const BString& storageFile, const BString& initIndexFile);		//Access to CAIS stored separately - storage data and IIA
	~CAISFileFetcher(void);

//Get element functions
public:

	template <class argIntType> void GetElementAt(intType index, CHArray<theType,argIntType>& result);
	void GetCharStringAt(intType index, BString& result);		//retrieves a char string stored with a terminating zero or without
	intType Count() const;

private:
	FILE* fp;								//file is opened in constructor, closed in destructor
	int64 storageStart;						//offset in the file where the storage array starts
	CHArray<intType,intType> initIndexArr;	//initial index array is held in memory, storage is not
	CHArray<char,int64> stringBuffer;		//buffer to copy chars into when a BString is requested - avoid allocating each time
	boost::recursive_mutex mutex;
};

//Access to a serialized CAIS file
template <class theType, class intType>
CAISFileFetcher<theType,intType>::CAISFileFetcher(const BString& fileName)
{
	//Layout of the CAIStrings file on disk:

	//storage array count			(intType)
	//storage array elements		(theType * storageCount)
	//init index array count		(intType)
	//init index array elements		(intType * initCount)

	//Open the file
	fp=fopen(fileName,"rb");		//Open as binary
	if(fp==NULL)
	{
		BString message="Could not open: "+fileName+".";
		std::cerr << message << std::endl;
		return;
	}

	////Seek to the start of file
	fseek_large(fp,0,SEEK_SET);

	//Read the number of points in the storage array and define the storage start
	intType storageCount;
	fread(&storageCount,sizeof(intType),1,fp);
	storageStart=sizeof(intType);
	
	//Seek past the end of storage - this is where the initial index array is stored
	fseek_large(fp, storageStart + sizeof(theType)*storageCount, SEEK_SET);

	//Read the number of points in the initial index array and seek back to the end of storage
	intType initCount;
	fread(&initCount,sizeof(intType),1,fp);
	fseek_large(fp, storageStart + sizeof(theType)*storageCount, SEEK_SET);

	//Read the initial index array into a char buffer
	CHArray<char,intType> buffer(sizeof(intType)*(initCount+1));
	fread(buffer.arr,sizeof(intType),initCount+1,fp);

	//Fill initIndexArr from the char buffer
	char* bufferStart=buffer.arr;					//Deserialization will move the pointer, can't use buffer.arr
	initIndexArr.SerializeFromBuffer(bufferStart);
}

//Access to a CAIS file stored separately:
//Data file (not a serialized CHArray, just data)
//And the initial index array (serialized CHArray)
template <class theType, class intType>
CAISFileFetcher<theType,intType>::CAISFileFetcher(const BString& storageFile, const BString& initIndexFile)
{
	//Open the storage file
	fp=fopen(storageFile,"rb");		//Open as binary
	if(fp==NULL)
	{
		BString message="Could not open: "+storageFile+".";
		std::cerr << message << std::endl;
		return;
	}

	storageStart = 0;
	initIndexArr.Load(initIndexFile);
}

template <class theType, class intType>
CAISFileFetcher<theType,intType>::~CAISFileFetcher()
{
	boost::recursive_mutex::scoped_lock lock(mutex);
	fclose(fp);
}

template <class theType, class intType>
template <class argIntType>
void CAISFileFetcher<theType,intType>::GetElementAt(intType index, CHArray<theType,argIntType>& result)
{
	boost::recursive_mutex::scoped_lock lock(mutex);

	intType startPos=initIndexArr[index];
	intType endPos=initIndexArr[index+1];
	intType elemSize=endPos-startPos;

	if((intType)result.GetSize()<(elemSize+1)) result.ResizeArray((argIntType)(elemSize+1));

	result.SetNumPoints((argIntType)elemSize);

	//Seek to where the element starts
	fseek_large(fp, storageStart + sizeof(theType)*startPos, SEEK_SET);
	
	//Read into the array
	fread(result.arr,sizeof(theType),elemSize,fp);
}

template <class theType, class intType>
void CAISFileFetcher<theType,intType>::GetCharStringAt(intType index, BString& result)
{
	boost::recursive_mutex::scoped_lock lock(mutex);

	GetElementAt(index,stringBuffer);
	stringBuffer.AddAndExtend(0);
	result=stringBuffer.arr;
}

template <class theType, class intType>
intType CAISFileFetcher<theType,intType>::Count() const
{
	intType result = initIndexArr.Count() - 1;
	if(result == -1) result = 0;

	return result;
}
