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
#include "CAIStrings.h"
#include <map>

//Type-to-int bidirectional map
//Cannot have more than 2^32 elements, so uses only CHArray<theType,int>

template <class theType> class CBidirectionalMap : public Savable
{
public:
	CBidirectionalMap(int theMaxPoints=-1, bool fFrequencies=false);
	CBidirectionalMap(CHArray<theType>& rhsArray, bool fFrequencies=false);	//Will call AddFromArray()
	CBidirectionalMap(const BString& fileName, bool fArray=false, bool fFrequencies=false);	//Will call Load() if fArray==false
																						//or LoadFromArray() if true
	void CreateFromArray(CHArray<theType>& rhsArray);
	~CBidirectionalMap(void){}

public:
	//Iterator semantics
	theType* begin() { return wordArr.begin(); }
	theType* end() { return wordArr.end(); }

public:
	int GetNumPoints() const {return wordArr.Count();}
	int Count() const { return GetNumPoints(); }
	int Size() const { return wordArr.Size(); }
	const theType& operator[](int num){return wordArr[num];}

	bool IsFull() const {return wordArr.IsFull();}
	bool IsEmpty() const {return wordArr.IsEmpty();}

	//More proper names
	bool Insert(const theType& val, int numTimes = 1){ return AddWord(val,numTimes); }
	int InsertGetIndex(const theType& val, int numTimes = 1){ return AddWordGetIndex(val,numTimes); }

    bool AddWord(const theType& newWord, int numTimes=1);
	CBidirectionalMap<theType>& operator<<(const theType& newWord) { AddWord(newWord); return *this; }		//operator << adds a word
    int AddWordGetIndex(const theType& newWord, int numTimes=1);
	void AddFromArray(CHArray<theType>& theArray, int numTimes=1);
	void AddFromMap(CBidirectionalMap<theType>& otherMap);
	template<class intType> void AddBStringsFromCAIS(CAIStrings<char,intType>& strings, int numTimes=1);

	//More proper names
	void Remove(CBidirectionalMap<theType>& removeList, bool present = true){ RemoveWords(removeList, present); }
	void Remove(const theType& val){ RemoveWord(val); }
	void RemoveByIndex(int index){ RemoveWordByIndex(index); }

	void RemoveWords(CBidirectionalMap<theType>& removeList, bool present=true);		//Removes words by re-composing the map
    void RemoveWord(const theType& word);		//Removes a single word - O(N) because of word array shifting and decrementing in the map
	void RemoveWordByIndex(int index);			//Removes a single word - O(N) because of word array shifting and decrementing in the map
    int GetFrequency(const theType& word);
    int GetIndex(const theType& word) const;
	void GetIndexForArrayOfWords(CHArray<theType>& words, CHArray<int>& result); //calls GetIndex() on every element of words and saves in result
    bool IsPresent(const theType& word);

	void LoadFromArray(const BString& fileName, bool fFrequencies);	//Creates map from saved CHArray
	void Serialize(BArchive& ar);

	void Clear();
	void Resize(int newSize);
	void ResizeIfSmaller(int newSize){ if (newSize > Size()) Resize(newSize); };
	void ResizeKeepPoints(int newSize);
	void SortByFrequencies();

public:
	bool fFreq;						//if frequencies are counted for words being added
	CHArray<int> freqArr;			//array of frequencies
	CHArray<theType> wordArr;		//array of words - word stands for key values (theType) added to map
	std::map<theType,int> map;			//mapping between words and numbers (indexes in array)
};

template<>
template<class intType>
void CBidirectionalMap<BString>::AddBStringsFromCAIS(CAIStrings<char,intType>& strings, int numTimes)
{
	int numElements=(int)strings.Count();
	if( Size() < Count() + numElements) ResizeKeepPoints(Count() + numElements);	//Resize if needed

	BString curString;
	for(int i=0;i<numElements;i++)
	{
		strings.GetCharStringAt(i,curString);
		AddWord(curString,numTimes);
	}
}

template <class theType>
CBidirectionalMap<theType>::CBidirectionalMap(int theMaxPoints, bool fFrequencies):
fFreq(fFrequencies)
{
	if (theMaxPoints > 0) Resize(theMaxPoints);
}

template <class theType>
CBidirectionalMap<theType>::CBidirectionalMap(CHArray<theType>& rhsArray, bool fFrequencies):
fFreq(fFrequencies)
{
	CreateFromArray(rhsArray);
}

template <class theType>
void CBidirectionalMap<theType>::CreateFromArray(CHArray<theType>& rhsArray)
{
	ResizeIfSmaller(rhsArray.Count());
	Clear();

	AddFromArray(rhsArray);
}

template <class theType>
void CBidirectionalMap<theType>::Resize(int newSize)
{
	wordArr.ResizeArray(newSize);
	if(fFreq) freqArr.ResizeArray(newSize);

	Clear();
}

template <class theType>
void CBidirectionalMap<theType>::ResizeKeepPoints(int newSize)
{
	//Copy the data arrays
	CHArray<theType> copyWords(wordArr);
	CHArray<int> copyFreq(freqArr);

	//Resize the map - erases the data
	Resize(newSize);

	//Add the existing words back
	for(int i=0; i < copyWords.Count() && i < newSize; i++)
	{
		if(fFreq) AddWord(copyWords[i],copyFreq[i]);
		else AddWord(copyWords[i],1);
	}
}

template <class theType>
CBidirectionalMap<theType>::CBidirectionalMap(const BString& fileName, bool fArray, bool fFrequencies)
{
	if(fArray) LoadFromArray(fileName,fFrequencies);
	else Load(fileName);
}

template <class theType>
void CBidirectionalMap<theType>::LoadFromArray(const BString& fileName, bool fFrequencies)
{
	fFreq=fFrequencies;
	CHArray<theType> tempArray(fileName);
	CreateFromArray(tempArray);
}


template <class theType>
void CBidirectionalMap<theType>::RemoveWords(CBidirectionalMap<theType>& removeList, bool present)
{
	//if present==true, removes entries present in removeList
	//if present==false, removes entries not present in removeList

	CHArray<theType> oldWordArr(wordArr);
	CHArray<int> oldFreqArr(freqArr);
	int oldNumPoints=Count();
	Clear();

	for(int i=0; i < oldNumPoints; i++)
	{
		theType& word=oldWordArr[i];

		bool condition=removeList.IsPresent(word);
		if(present) condition=!condition;		//remove entries present in removeList if true

		if(condition)
		{
			AddWord(word);
			if(fFreq) freqArr.AddAndExtend(oldFreqArr[i]);
		}
	}
}

template <class theType>
void CBidirectionalMap<theType>::RemoveWord(const theType& word)
{
	//Removes a single word from the map and the word array
	//Slow operation - because the array needs to be shifted with each deletion
	//And all indices above the given index decremented

	int index=GetIndex(word);
	if(index==-1) return;

	//By this point, the word is in the map
	RemoveWordByIndex(index);
	return;
}

template <class theType>
void CBidirectionalMap<theType>::RemoveWordByIndex(int index)
{
	//Removes a single word from the map and the word array by its index
	//Slow operation - because the array needs to be shifted with each deletion
	//And all indices above the given index decremented

	map.erase(wordArr[index]);
	wordArr.RemovePointAt(index);
	if(fFreq) freqArr.RemovePointAt(index);
	
	for(int i=index; i < Count(); i++)
	{
		map[wordArr[i]]=i;
	}
}

template <class theType>
void CBidirectionalMap<theType>::SortByFrequencies()
{
	if(!fFreq) return;

	CHArray<int> perm;
	freqArr.SortPermutation(perm,true);

	wordArr.Permute(perm);
	freqArr.Permute(perm);

	CHArray<theType> oldWordArr(wordArr);
	CHArray<int> oldFreqArr(freqArr);

	Clear();

	for (int i = 0; i < oldWordArr.Count(); i++)
	{
		AddWord(oldWordArr[i],oldFreqArr[i]);
	}
}

template <class theType>
void CBidirectionalMap<theType>::Clear()
{
	freqArr.Clear();
	wordArr.Clear();
	map.clear();
}

template <class theType>
void CBidirectionalMap<theType>::Serialize(BArchive& ar)
{
	if(ar.IsStoring())
	{
		wordArr.Serialize(ar);
		freqArr.Serialize(ar);
		ar<<fFreq;
	}
	else
	{
		CHArray<theType> tempWordArr;
		CHArray<int> tempFreqArr;

		tempWordArr.Serialize(ar);
		tempFreqArr.Serialize(ar);

		ar>>fFreq;
		if(!fFreq) freqArr.ResizeArray(0);

		Resize(tempWordArr.Count());
		
		for(int i=0; i < tempWordArr.Count(); i++)
		{
			if(fFreq) AddWord(tempWordArr[i],tempFreqArr[i]);
			else AddWord(tempWordArr[i],1);
		}
	}
}

template <class theType>
void CBidirectionalMap<theType>::AddFromArray(CHArray<theType>& theArray, int numTimes)
{
	int newSize = Count() + theArray.Count();
	if(Size() < newSize ) ResizeKeepPoints(newSize);

	for(int i=0; i < theArray.Count(); i++) AddWord(theArray[i], numTimes);
}

template <class theType>
void CBidirectionalMap<theType>::AddFromMap(CBidirectionalMap<theType>& otherMap)
{
	int numTotal = Count() + otherMap.Count();
	if(Size() < numTotal) ResizeKeepPoints(numTotal);

	for(int i=0; i < otherMap.Count(); i++)
	{
		if(otherMap.fFreq) AddWord(otherMap.wordArr[i],otherMap.freqArr[i]);
		else AddWord(otherMap.wordArr[i]);
	}
}

template <class theType>
bool CBidirectionalMap<theType>::AddWord(const theType& newWord, int numTimes)		//returns true if word was already there
{
	int prevCount = Count();
	AddWordGetIndex(newWord, numTimes);

	//Return false if the count increased
	if (Count() > prevCount) return false;
	else return true;
}

template <class theType>
int CBidirectionalMap<theType>::AddWordGetIndex(const theType& newWord, int numTimes)		//returns index
{
	int index = GetIndex(newWord);
	if(index != -1)	//word already there
	{
		if(fFreq) freqArr[index]+=numTimes;
		return index;
	}
	else			//word not found
	{
		map[newWord] = Count();
		wordArr.AddAndExtend(newWord);
		if(fFreq) freqArr.AddAndExtend(numTimes);
		return(Count()-1);
	}
}

template <class theType>
int CBidirectionalMap<theType>::GetFrequency(const theType& word)
{
	if(!fFreq) return 0;

	int index = GetIndex(word);
	if(index !=-1) return freqArr[index];	
	else return 0;
}

//calls GetIndex() on every element of words and saves in result
template <class theType>
void CBidirectionalMap<theType>::GetIndexForArrayOfWords(CHArray<theType>& words, CHArray<int>& result)
{
	int numToProcess = words.Count();
	result.ResizeArray(numToProcess,true);

	for(int i=0;i<numToProcess;i++)
	{
		result[i]=GetIndex(words[i]);
	}
}

template <class theType>
int CBidirectionalMap<theType>::GetIndex(const theType& word) const
{
	auto it = map.find(word);
	if(it!=map.end())	//word is found
	{
		return (*it).second;
	}
	else return -1;
}

template <class theType>
bool CBidirectionalMap<theType>::IsPresent(const theType& word)
{
	if(GetIndex(word) >= 0) return true;	//word is found
	else return false;
}
