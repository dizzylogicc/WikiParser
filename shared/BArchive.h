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

#include <iostream>
#include <type_traits>
#include <cstdio>

typedef long long int64;

//A simplified replacement for MFC CArchive class
//Accepts either std::ifstream or std::ofstream for costruction
//The streams should have std::ifstream::binary specified during construction

//Serialize into the archive with operator <<
//And out of the archive with >>

//Or use operator & to do the proper operation for this archive
//(the archive knows whether it's saving or loading)

//Functions IsSaving() and IsLoading() tell which kind of archive it is

//Will directly serialize the value of any non-class variables
//And that includes all pointers - just the pointer value, not what it points to

//For classes, will call 
//void Serialize(BArchive&)
//if it is declared in the class
//either as class's own method or an inherited method

//But if the class does not have 
//void Serialize(BArchive&) declared,
//The achive will only complain to std::cerr and do nothing!

class BArchive
{
public:
	//Default constructor declared, but not defined - no default is allowed
	BArchive();

	//Accepts either ostream or istream during construction
	//With ostream, it is a storing archive
	explicit BArchive(std::ostream& theOutStream):
	inStream(NULL),
	outStream(&theOutStream),
	isStoring(true)
	{
	};

	//With istream, it is a loading archive
	explicit BArchive(std::istream& theInStream):
	inStream(&theInStream),
	outStream(NULL),
	isStoring(false)
	{
	};

	~BArchive(){};

public:
	bool IsStoring() const {return isStoring;};
	bool IsLoading() const {return !isStoring;};

	//Store or retrieve a value
	template<class theType>
	BArchive& operator<<(theType& val){return Store(val);};

	template<class theType>
	BArchive& operator>>(theType& val){return Retrieve(val);};

	template<class theType>
	BArchive& operator&(theType& val)
	{
		if(IsStoring()) return Store(val);
		else return Retrieve(val);
	};

	//Store or retrieve an array
	template<class theType>
	BArchive& StoreArray(theType* val, int64 num)
	{
		if(!IsStoring()) return *this;
		DoStoreArray(val, num, std::is_class<theType>());
		return *this;
	}

	template<class theType>
	BArchive& RetrieveArray(theType* val, int64 num)
	{
		if(IsStoring()) return *this;
		DoRetrieveArray(val, num, std::is_class<theType>());
		return *this;
	}

	template<class theType>
	BArchive& HandleArray(theType* val, int64 num)
	{
		if(IsStoring()) return StoreArray(val,num);
		else return RetrieveArray(val,num);
		return *this;
	}

private:
	//Calls a different templated function depending on val type
	//Compile-time branching
	template<class theType>
	BArchive& Store(theType& val)
	{
		if(!isStoring) return *this;
		DoStore(val, std::is_class<theType>());
		return *this;
	}

	template<class theType>
	BArchive& Retrieve(theType& val)
	{
		if(isStoring) return *this;
		DoRetrieve(val, std::is_class<theType>());
		return *this;
	}

private:
	template<class theType>
	void DoStoreArray(theType* val, int64 num, std::false_type)
	{
		//If it is a non-class type, write it directly
		(*outStream).write( (const char*)val, sizeof(*val) * num );
	}

	template<class theType>
	void DoStoreArray(theType* val, int64 num, std::true_type)
	{
		//If it is a class, determine whether it has a serialize function
		HandleArrayClass(val, num);
	}

	template<class theType>
	void DoRetrieveArray(theType* val, int64 num, std::false_type)
	{
		//If it is a non-class type, read it directly
		(*inStream).read( (char*)val, sizeof(*val) * num );
	}

	template<class theType>
	void DoRetrieveArray(theType* val, int64 num, std::true_type)
	{
		//If it is a class, determine whether it has a serialize function
		HandleArrayClass(val, num);
	}

	template<class theType>
	void DoStore(theType& val, std::false_type)
	{
		//If it is a non-class type, write it directly
		(*outStream).write( (const char*)(&val), sizeof(val) );
	}

	template<class theType>
	void DoStore(theType& val, std::true_type)
	{
		//If it is a class, it is either non-serializable or a class with a member called Serialize()
		HandleClass(&val);
	}

	template<class theType>
	void DoRetrieve(theType& val, std::false_type)
	{
		//If it is a non-class type, read it directly
		(*inStream).read( (char*)(&val), sizeof(val) );
	}
	
	template<class theType>
	void DoRetrieve(theType& val, std::true_type)
	{
		//If it is a class, see if it has a Serialize() function
		HandleClass(&val);
	}

	//Dark magic to determine at compile-time whether class T has a void Serialize(BArchive&) function
	#define HAS_SERIALIZE_FUNC(func, name)											\
	template<typename T, typename Sign>												\
	struct name {																	\
	       typedef char yes[1];														\
	       typedef char no [2];														\
	       template <typename U, U> struct type_check;								\
	       template <typename _1> static yes &chk(type_check<Sign, &_1::func > *);	\
	       template <typename   > static no  &chk(...);								\
	       static bool const value = sizeof(chk<T>(0)) == sizeof(yes);				\
	}

	template<bool C, typename T = void>
	struct enable_if{typedef T type;};

	template<typename T>
	struct enable_if<false, T> { };

	HAS_SERIALIZE_FUNC(Serialize, has_serialize);

	template<typename T> 
	typename enable_if<has_serialize<T,void(T::*)(BArchive&)>::value, void>::type
	HandleClass(T * t)
	{
	   t->Serialize(*this);	/* When T has Serialize() */
	}

	template<typename T> 
	typename enable_if<!has_serialize<T,void(T::*)(BArchive&)>::value, void>::type
	HandleClass(T * t) 
	{
		/* When T does not have Serialize just complain and do nothing */
		std::cerr << "Error: attempting to serialize a class without a void Serialize(BArchive&) function.";
	}

	template<typename T> 
	typename enable_if<has_serialize<T,void(T::*)(BArchive&)>::value, void>::type
	HandleArrayClass(T* t, int64 num)
	{
		/* When T has Serialize() */
		for(int64 i=0;i<num;i++)
		{
			t->Serialize(*this);
			t++;
		}
	}

	template<typename T> 
	typename enable_if<!has_serialize<T,void(T::*)(BArchive&)>::value, void>::type
	HandleArrayClass(T* t, int64 num) 
	{
		/* When T does not have Serialize just complain and do nothing */
		std::cerr << "Error: attempting to serialize an array of class without a void Serialize(BArchive&) function.";
	}
	//End dark magic

private:
	bool isStoring;

	std::ostream* outStream;
	std::istream* inStream;
};

