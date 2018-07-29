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
#include "Savable.h"

template <class theType, class intType=int> class CArrArr : public Savable
{
public:
	//Constructor from numbers of points
	CArrArr(intType numArrays=0, intType arrSize=0, bool fSetMaxPoints=false):
	arr(numArrays,true)
	{Resize(numArrays,arrSize,fSetMaxPoints);};

	//Create a virtual array of arrays
	CArrArr(const CArrArr<theType,intType>& otherArrArr, intType startNum, intType numToInclude):
	arr(otherArrArr.arr.arr + startNum, numToInclude, true){};

	explicit CArrArr(const BString& fileName){Load(fileName);};
	~CArrArr(void){};

//Container behavior
public:
	CHArray<theType, intType>* begin(){ return arr.arr; }
	CHArray<theType, intType>* end(){ return arr.arr + arr.Count(); }

//The only data member
public:
	CHArray<CHArray<theType,intType>,intType> arr;

public:
	CHArray<theType,intType>& operator[](intType num) const {return arr[num];};
	void Resize(intType numArrays, intType arrSize, bool fSetMaxPoints=false);
	void Serialize(BArchive& archive);
};

template <class theType, class intType>
void CArrArr<theType,intType>::Resize(intType numArrays, intType arrSize, bool fSetMaxPoints)
{
	arr.ResizeArray(numArrays,true);

	for(intType counter1=0;counter1<numArrays;counter1++)
	{
		arr[counter1].ResizeArray(arrSize,fSetMaxPoints);
	}
}

template <class theType, class intType>
void CArrArr<theType,intType>::Serialize(BArchive& archive)
{
	intType numArrays;

	if(archive.IsStoring())
	{
		numArrays=arr.GetNumPoints();
		archive<<numArrays;
	}
	else
	{
		archive>>numArrays;
		Resize(numArrays,0,false);
	}

	for(intType counter1=0;counter1<numArrays;counter1++)
	{
		arr[counter1].Serialize(archive);
	}
}
