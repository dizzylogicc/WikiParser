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

//Contiguous array of immutable strings - efficient storage of strings of char, int, etc.

#pragma once
#include "Array.h"
#include "Savable.h"
#include "Common.h"

template <class theType, class intType=int> class CAIStrings : public Savable
{
public:
	CAIStrings(intType storageSize=0, intType maxElements=0);							//Create an empty array

	//Create CAIStrings from two arrays: one containing the data
	//And the other containing initial index array for the elements in the data array
	CAIStrings(const CHArray<intType,intType>& iia, const CHArray<theType,intType>& dataArr);	

	CAIStrings(BString fileName){Load(fileName);};										//Load from file

	CAIStrings(const CAIStrings<theType,intType>& rhs):		//Copy constructor
		storageArr(rhs.storageArr),
		initIndexArr(rhs.initIndexArr){};

	~CAIStrings(void);

public:
	//Data storage
	//These two arrays only!
	CHArray<theType,intType> storageArr;	//the array which stores contiguous immutable strings
	CHArray<intType,intType> initIndexArr;	//indexes of the starting element in the string

public:
	//Sorts primaryIndices and secondaryIndices
	//And builds a CAIS index of secondaryIndices for each primaryIndex
	//Saves the frequency information of secondary indices for each primaryIndex in freqCAIS
	template <class argIntType> void BuildIndex(CAIStrings<theType,intType>& freqCAIS,
												argIntType numPrimaryIndices,
												CHArray<theType,argIntType>& primaryIndices,
												CHArray<theType,argIntType>& secondaryIndices);


	intType Count() const {return initIndexArr.Count()-1;};
	intType NumElements() const {return Count();};
	void GetElementLengths(CHArray<intType,intType>& lengths) const;	//Save the lengths of all elements into "lengths"
	intType StorageUsed() const {return storageArr.GetNumPoints();};
	int64 DataSizeInBytes() const {return storageArr.DataSizeInBytes() + initIndexArr.DataSizeInBytes();};
	intType NumPointsInElement(intType elementNum) const {return initIndexArr[elementNum+1]-initIndexArr[elementNum];};
	intType CountInElement(intType elementNum) const {return NumPointsInElement(elementNum);};
	intType CountEmpty() const;
	intType CountNonEmpty() const {return Count() - CountEmpty();};

	//Getting elements, virtual elements, and pointers to elements
	//Will set the result to virtual, no memory copy
	template <class argIntType> void GetVirtualElement(intType index, CHArray<theType,argIntType>& result);
	template <class argIntType> void GetElementAt(intType index, CHArray<theType,argIntType>& result);
	theType* GetPointerToElement(intType elementNum) const {return storageArr.arr+initIndexArr[elementNum];};

	//Getting and adding char strings
	void AddCharString(const BString& string, bool fAddZero = true);		//adds a char string with a terminating zero or without
	void GetCharStringAt(intType index, BString& result);		//retrieves a char string stored with a terminating zero or without
	template <class argIntType> void AddArrayOfCharStrings(CHArray<BString,argIntType>& from, bool fAddZero = true);

	//Adding elements
	void AddElement(const theType* pointer, intType numToCopy);
	template <class argIntType> void AddElement(const CHArray<theType,argIntType>& element);	//accepts different intType
	//accepts CHArrays with different type and intType
	template <class argType, class argIntType> void ImportElement(const CHArray<argType,argIntType>& element);
	//Import all data from a CAIS of different type
	template <class argType, class argIntType> void ImportFrom(const CAIStrings<argType,argIntType>& rhs);

	//Permutes the elements of the CAIS according to the permutation provided
	template<class argIntType1, class argIntType2> void Permute(const CHArray<argIntType1, argIntType2>& perm);

	void Serialize(BArchive& archive);

	void ResizeIfSmaller(intType storageSize,intType maxElements);		//Resizes if smaller
	//Resizes if smaller based on the sizes of the CAIS provided
	template <class argType, class argIntType> void ResizeIfSmaller(const CAIStrings<argType,argIntType>& rhs)
	{ResizeIfSmaller((intType)rhs.NumElements(),(intType)rhs.storageArr.Count());};

	void ResizeToZero();
	void Clear();

	void SetDataAndIia(const CHArray<intType,intType>& iia, const CHArray<theType,intType>& dataArr);

	theType& operator()(intType elementNum, intType pointNum) const {return storageArr[initIndexArr[elementNum]+pointNum];};

	CCommon common;
};

//The number of empty elements in the array
//Caution - empty strings of chars may be saved with a terminating NULL
template <class theType, class intType>
intType CAIStrings<theType,intType>::CountEmpty() const
{
	intType result=0;
	intType count=Count();
	for(intType i=0;i<count;++i)
	{
		if(NumPointsInElement(i)==0) result++;
	}

	return result;
}

//Sorts primaryIndices and secondaryIndices
//And builds a CAIS of secondaryIndices for each primaryIndex
//Saves the frequency information of secondary indices for each primaryIndex in freqCAIS
template <class theType, class intType>
template <class argIntType>
void CAIStrings<theType,intType>::BuildIndex(CAIStrings<theType,intType>& freqCAIS,
											argIntType numPrimaryIndices,
											CHArray<theType,argIntType>& primaryIndices,
											CHArray<theType,argIntType>& secondaryIndices)
{
	//Clear all
	(*this).Clear();
	freqCAIS.Clear();

	//Resize as needed
	(*this).ResizeIfSmaller((intType)primaryIndices.Count(),(intType)numPrimaryIndices);
	freqCAIS.ResizeIfSmaller((intType)primaryIndices.Count(),(intType)numPrimaryIndices);

	//Sort first by secondary indices, then with stable sort by primary indices
	CHArray<argIntType,argIntType> perm;
	secondaryIndices.SortPermutation(perm,false,false);		//Regular sort
	secondaryIndices.Permute(perm);
	primaryIndices.Permute(perm);

	primaryIndices.SortPermutation(perm,false,true);	//Stable sort
	secondaryIndices.Permute(perm);
	primaryIndices.Permute(perm);

	//Build the initial index array on primaryIndices
	CHArray<argIntType,argIntType> iia;
	primaryIndices.InitialIndexArray(iia,numPrimaryIndices);

	//Find longest string
	argIntType longest=0;
	for(argIntType i=0;i<numPrimaryIndices;i++)
	{
		if((iia[i+1]-iia[i])>longest) longest=iia[i+1]-iia[i];
	}

	//Extract runs from each string of secIndices corresponding to each primaryIndex
	//And add them to (*this) and freqCAIS
	CHArray<theType,argIntType> curSecString;
	CHArray<theType,argIntType> runVals(longest);
	CHArray<argIntType,argIntType> runLengths(longest);
	for(argIntType i=0;i<numPrimaryIndices;i++)
	{
		curSecString.SetVirtual(secondaryIndices.arr+iia[i],iia[i+1]-iia[i]);
		curSecString.CountRuns(runVals,runLengths);

		(*this).AddElement(runVals);
		freqCAIS.ImportElement(runLengths);		//We have to call Import because argIntType does not necessarily match theType
	}
}

template <class theType, class intType>
template <class argIntType>
void CAIStrings<theType,intType>::AddArrayOfCharStrings(CHArray<BString,argIntType>& from, bool fAddZero)
{
	//Make sure we have enough space
	intType newStorage=storageArr.Count();
	intType newElements=initIndexArr.Count();

	if(fAddZero) newStorage+=(intType)(from.Count());
	newElements+=(intType)(from.Count());

	for(argIntType i=0;i<from.Count();i++)
	{
		newStorage+=(intType)(from[i].GetLength());
	}
	ResizeIfSmaller(newStorage,newElements);

	//Save all the strings
	for(argIntType i=0;i<from.Count();i++)
	{
		AddCharString(from[i],fAddZero);
	}
}

//Create CAIStrings from two arrays: one containing the data
//And the other containing initial index array for the elements in the data array
template <class theType, class intType>
CAIStrings<theType,intType>::CAIStrings(const CHArray<intType,intType>& iia, const CHArray<theType,intType>& dataArr):
initIndexArr(iia),
storageArr(dataArr)
{
}

template <class theType, class intType>
void CAIStrings<theType,intType>::SetDataAndIia(const CHArray<intType,intType>& iia, const CHArray<theType,intType>& dataArr)
{
	initIndexArr=iia;
	storageArr=dataArr;
}

template <class theType, class intType>
void CAIStrings<theType,intType>::Clear()
{
	initIndexArr.EraseArray();
	storageArr.EraseArray();

	initIndexArr.AddAndExtend(0);
}

template <class theType, class intType>
CAIStrings<theType,intType>::CAIStrings(intType storageSize, intType maxElements):
storageArr(storageSize),
initIndexArr(maxElements+1)
{
	initIndexArr.AddAndExtend(0);	//the start of the first element
}

template <class theType, class intType>
CAIStrings<theType,intType>::~CAIStrings(void)
{
}

//Save the lengths of all elements into "lengths"
template <class theType, class intType>
void CAIStrings<theType,intType>::GetElementLengths(CHArray<intType,intType>& lengths) const
{
	intType numElements=Count();
	lengths.ResizeIfSmaller(numElements,true);

	for(intType i=0;i<numElements;i++)
	{
		lengths[i]=NumPointsInElement(i);
	}
}

template <class theType, class intType>
void CAIStrings<theType,intType>::ResizeIfSmaller(intType storageSize,intType maxElements)
{
	if(storageSize>=storageArr.GetSize()) storageArr.ResizeArrayKeepPoints(storageSize);
	if(maxElements>=(initIndexArr.GetSize()-1)) initIndexArr.ResizeArrayKeepPoints(maxElements+1);
}

template <class theType, class intType>
void CAIStrings<theType,intType>::ResizeToZero()
{
	storageArr.ResizeArray(0);
	initIndexArr.ResizeArray(1);

	initIndexArr.AddAndExtend(0);
}

template <class theType, class intType>
void CAIStrings<theType,intType>::AddCharString(const BString& string, bool fAddZero)
{
	int length = string.GetLength();
	if(fAddZero) length += 1;
	AddElement((const char*)string,length);
}

template <class theType, class intType>
void CAIStrings<theType,intType>::GetCharStringAt(intType index, BString& result)
{
	result = BString(
		(const char*)(storageArr.arr+initIndexArr[index]),
		initIndexArr[index+1]-initIndexArr[index]);
}

template <class theType, class intType>
void CAIStrings<theType,intType>::AddElement(const theType* pointer, intType numToCopy)
{
	//Store the element
	for(intType i=0;i<numToCopy;i++)
	{
		storageArr.AddAndExtend(pointer[i]);
	}
	initIndexArr.AddAndExtend(storageArr.GetNumPoints());
}

//AddElement will accept arrays with differing intType
template <class theType, class intType>
template <class argIntType>
void CAIStrings<theType,intType>::AddElement(const CHArray<theType,argIntType>& element)
{
	argIntType numToCopy=element.GetNumPoints();

	//Store the element
	for(argIntType i=0;i<numToCopy;i++)
	{
		storageArr.AddAndExtend(element.arr[i]);
	}

	initIndexArr.AddAndExtend(storageArr.GetNumPoints());
}

//ImportElement accepts CHArrays with different type and intType
template <class theType, class intType>
template <class argType, class argIntType>
void CAIStrings<theType,intType>::ImportElement(const CHArray<argType,argIntType>& element)
{
	argIntType numToCopy=element.GetNumPoints();

	//Store the element
	for(argIntType i=0;i<numToCopy;i++)
	{
		storageArr.AddAndExtend((theType)element.arr[i]);
	}

	initIndexArr.AddAndExtend(storageArr.GetNumPoints());
}

//Import all data from a CAIS of different type
template <class theType, class intType>
template <class argType, class argIntType>
void CAIStrings<theType,intType>::ImportFrom(const CAIStrings<argType,argIntType>& rhs)
{
	storageArr.ImportFrom(rhs.storageArr);
	initIndexArr.ImportFrom(rhs.initIndexArr);
}

//Permutes the elements of the CAIS according to the permutation provided
template <class theType, class intType>
template<class argIntType1, class argIntType2>
void CAIStrings<theType,intType>::Permute(const CHArray<argIntType1, argIntType2>& perm)
{
	CAIStrings<theType,intType> copy(*this);
	Clear();

	CHArray<theType,intType> curElem;
	intType numInPerm=(intType)perm.Count();
	for(intType i=0;i<numInPerm;i++)
	{
		copy.GetVirtualElement((intType)perm[(argIntType2)i],curElem);
		AddElement(curElem);
	}
}

//Will set result to virtual, no memory copying
template <class theType, class intType>
template <class argIntType>
void CAIStrings<theType,intType>::GetVirtualElement(intType index, CHArray<theType,argIntType>& result)
{
	result.SetVirtual(storageArr.arr+initIndexArr[index],(argIntType)(initIndexArr[index+1]-initIndexArr[index]));
}

template <class theType, class intType>
template <class argIntType>
void CAIStrings<theType,intType>::GetElementAt(intType index, CHArray<theType,argIntType>& result)
{
	intType startPos=initIndexArr[index];
	intType endPos=initIndexArr[index+1];
	intType elemSize=endPos-startPos;

	if((intType)result.GetSize()<elemSize) result.ResizeArray((argIntType)elemSize);

	result.SetNumPoints((argIntType)elemSize);
	intType curPos=startPos;
	for(intType i=0;i<elemSize;i++)
	{
		result.arr[(argIntType)i]=storageArr.arr[startPos++];
	}
}

template <class theType, class intType>
void CAIStrings<theType,intType>::Serialize(BArchive& archive)
{
	storageArr.Serialize(archive);
	initIndexArr.Serialize(archive);
}

