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

//Array container class with tighter guarrantees on buffer and element access compared to std::vector<>

#pragma once
#include <fseek_large.h>
#include <math.h>
#include <algorithm>
#include <functional>
#include <BString.h>
#include <Savable.h>

#pragma warning(disable:4996)		//disable unsafe functions warning

typedef unsigned int uint;
typedef long long int64;
typedef unsigned long long uint64;
typedef unsigned char uchar;

template <class theType, class intType=int> class CHArray : public Savable
{
	//Add member vars
private:
	intType size;
	intType numPoints;
	bool fVirtual;

public:
	theType* arr;

	//Construction/destruction
public:
	CHArray(intType theSize=0, bool setMaxNumPoints=false);
	CHArray(theType start, theType end, intType theNumPoints);		//create vector from start to end with numPoints
	CHArray(const CHArray<theType,intType>& rhs);					//Copy constructor

	//Constructors don't allocate and delete the array if isVirtual==true
	//Will use the provided array.
	//The data is copied if false.
	CHArray(theType* thePointer, intType theSize, bool isVirtual=false);
	CHArray(const theType* thePointer, intType theSize);	//Const version - cannot create a virtual array
	CHArray(const BString& fileName);							//Will call Load()
	
	~CHArray();

public:
	void CopyFromPointer(const theType* thePointer, intType numToCopy);
	void SetVirtual(theType* thePointer, intType theSize);	//Makes array virtual
	//Importing from a different type of array with default cast
	template<class rhsType, class rhsIntType> void ImportFrom(const CHArray<rhsType, rhsIntType>& rhs);

public:
	bool IsIncreasing() const;
	bool IsDecreasing() const;
	bool IsEmpty() const {return (numPoints==0);}
	bool IsFull() const {return (numPoints==size);}
	bool IsDataSigned() const;		//Tests whether the data is signed or unsigned integer
	bool IsVirtual() const {return fVirtual;}
	intType GetSize() const {return size;}
	intType Size() const { return size; }
	intType GetNumPoints() const {return numPoints;}
	intType Count() const {return GetNumPoints();}		//alias
	intType GetNumNonZeros() const;						//returns the number of non-zero values in the array
	bool fDataPresent() const {return (numPoints!=0);}
	int64 DataSizeInBytes() const {return numPoints*sizeof(theType);}		//Size of the array in bytes

	//Container behavior
	theType* begin() const {return arr;}
	theType* end() const {return arr+numPoints;}

	//Stack-like behavior
	void Push(theType& val){AddAndExtend(val);};						//"push" for stack behavior
	theType Pop(){RemoveLastPoint();return arr[numPoints];}				//"pop" for stack behavior - will not work if array is empty!
	theType& Last() const {return arr[numPoints-1];}					//Reference to last element - will not work if array is empty!
	theType& First() const { return arr[0]; }							//Reference to first element - will not work if array is empty!

	void IntegralForm();
	void InitialIndexArray(CHArray<intType,intType>& result, intType numIndices) const;	//Compose initial index array (result) from indices for sparse mat
	void Convolve(const CHArray<theType,intType>& rhs, CHArray<theType,intType>& result) const;//O(N^2) convolution - dimensions of (*this) and rhs may be different
	CHArray<theType,intType>& AdjacentAveraging(intType num);
	CHArray<theType,intType>& BackNormAveraging(const CHArray<theType,intType>& weights);//Backwards normalized averaging - for example for exponential averaging
	CHArray<theType,intType>& LimitMaxValue(theType limit);
	CHArray<theType,intType>& LimitMinValue(theType limit);
	
	//Entropy of the distribution and entropic number of states
	double Entropy() const;
	double EntropicNumberOfStates() const;

	theType Sum() const;
	theType SumOfSquares() const;
	theType WeightedSum(const CHArray<theType, intType>& weights) const;
	theType Mean() const;
	theType StandardDeviation() const;		//Standard deviation from mean
	theType Variance() const;				//Variance from mean

	theType DirectProduct(const CHArray<theType,intType>& rhs, theType alpha=(theType)1) const; //Finds (|a|*|b|)^alpha*cos(angle)
	theType CosineMeasure(const CHArray<theType,intType>& rhs) const {return DirectProduct(rhs,0);}
	theType EuclideanDistance(const CHArray<theType,intType>& rhs) const;
	theType EuclideanNorm() const {return sqrt(SumOfSquares());}
	theType WeightedSumOfSquares(const CHArray<theType,intType>& weights) const;
	theType WeightedSumOfSquaredDevs(const CHArray<theType,intType>& rhs, const CHArray<theType,intType>& weights) const;	//weighted sum of squared deviations
	theType NormalizeNorm();				//Normalizes euclidean norm, returns the normalization constant
	theType NormalizeSumTo1();				//Returns normalization constant
	theType RootMeanSquareDev(const CHArray<theType,intType>& rhs) const;
	theType SumOfSquaredDevs(const CHArray<theType, intType>& rhs) const;
	theType MeanSquaredDev(const CHArray<theType, intType>& rhs) const;

	CHArray<theType,intType>& MultiplyAdd(const CHArray<theType,intType>& rhs, theType factor);	//Adds rhs*factor to *this
	CHArray<theType,intType>& Orthogonalize(const CHArray<theType,intType>& rhs);	//Orthogonalizes with respect to rhs
	theType ShiftedDirectProduct(const CHArray<theType,intType>& rhs, intType shift=0) const;	//direct product with a shift
																					//arrays do not have to have the same dimensions
																					//"shift" positions rhs over (*this) 

	bool IsPresent(const theType& val) const;										//Presence check - by comparing every element
	intType CountOccurrence(const theType& val) const;
	
	intType FindSequence(const CHArray<theType,intType>& sequence, intType startPos=0);		//Find a sequence of elements, returns -1 if not found

	intType PositionOf(const theType& val) const;
	intType PositionOfClosest(const theType& val) const;
	theType Min() const;
	theType Max() const;
	intType PositionOfMin() const;
	intType PositionOfMax() const;
	
	//Writing to and reading from files
	bool Write(BString name, BString form="%.5e") const;
	bool Read(BString name);												//Changes the size of the array as necessary
	bool WriteBinary(const BString& fileName);
	bool ReadBinary(const BString& fileName);										//Changes the size of the array as necessary
	bool ReadStrings(const BString& fileName);										//Changes the size of the array as necessary
	bool WriteStrings(const BString& fileName);
	void Serialize(BArchive& ar);

	//Serializing to memory buffer
	intType RequiredSpace() const;				//Space required to serialize to memory buffer, in bytes
	void SerializeToBuffer(char*& buffer);		//Serialize to memory buffer and icrement buffer
	void SerializeFromBuffer(char*& buffer);	//Serialize from memory buffer and increment buffer

	//Builds forward and reverse renumbering to a reduced subset based on this array
	//The value of -1 in *this signifies which indices will not be included in the reduced subset
	template <class argType, class argIntType> void BuildRenumbering(CHArray<argType,argIntType>& forwardRenum,
																		CHArray<argType,argIntType>& reverseRenum) const;

	//Renumbers values in the array by taking the index from renum at the location specified by the value
	template<class argIntType> void Renumber(const CHArray<intType,argIntType>& renum);

	void CountRuns(CHArray<theType,intType>& runVals,
			CHArray<intType,intType>& runLengths) const;	//Count runs of identical elements in the array

	void SetNumPoints(intType num){if(num<=size) numPoints=num; else numPoints=size;}
	inline void AddPoint(const theType& point);
	void AddAndExtend(const theType& point, intType factor=2);		//Tries to add a point, extending the array by "factor" if necessary
	CHArray<theType, intType>& operator<< (const theType& point){AddAndExtend(point);return *this;}	//Same as AddAndExtend()
	//Same as AddFromArray()
	CHArray<theType, intType>& operator<< (const CHArray<theType, intType>& source) {AddFromArray(source);return *this;}	
	

	void AddFromArray(const CHArray<theType,intType>& source);		//Will extend the array as needed
	void AddExFromString(const BString& source);					//Add-and-extend from string
	void EraseArray();
	void Clear(){EraseArray();}					//alias
	void RemoveAllPoinstAfter(intType num);
	void RemoveLastPoint();
	void RemoveByValue(const theType& val);									//Removes all points with the given value
	void DeletePointAt(intType theIndex);									//Slow - time proportional to number of points
	void RemovePointAt(intType theIndex){DeletePointAt(theIndex);}			//Alias for DeletePointAt(intType theIndex)
	void RemoveRepetitions();		//Removes elements that are the same as the preceding element - call on sorted arrays
	void ResizeArray(intType newSize, bool fSetMaxNumPoints=false);
	void Resize(intType newSize, bool fSetMaxNumPoints=false){ResizeArray(newSize,fSetMaxNumPoints);}	//alias
	void ResizeIfSmaller(intType newSize, bool fSetNumPoints=false);
	void ResizeArrayKeepPoints(intType newSize);
	void ResizeToZero() {ResizeArray(0);}

	//Decimates the array with provided step, starting from index 0 (0 is retained)
	//Every step-th element is retained
	void Decimate(intType step);
	
	intType GetNextMaxPos(intType curPos) const;
	intType GetNextMinPos(intType curPos) const;

	//Finds the position that splits the sorted array in two
	//Where the sum of squared deviations from the centers of the two new arrays is minimized
	//Always returns a split - i.e. none of the new arrays is equal to the old array
	//Position is the end() position for the leftmost new array
	//Used in PDDP cluster splitting, for example
	//Call on sorted arrays
	//O(N)
	//Optionally returns the reduction in squared dev that would be achieved with this split
	intType OptimalSplitSorted(theType* reductionInDev = NULL) const;

	int RandomIndex() const;	//Returns random index between 0 and numPoints-1, or -1 when array is empty

	CHArray<theType, intType>& SetValToPointNum();		//Sets the value of each point to its index
	CHArray<theType, intType>& SetToRandom();			//Sets all points to random value from the interval [0.0,1.0]

	//Take square root of every element
	CHArray<theType, intType>& Sqrt() {return Apply(std::bind(&CHArray<theType, intType>::InternalSqrt, this, std::placeholders::_1));}				
	//Replace every element with its absolute value
	CHArray<theType, intType>& Abs() { return Apply(std::bind(&CHArray<theType, intType>::InternalAbs, this, std::placeholders::_1)); }
	//Make equal to closest integer that is equal to value or lower
	CHArray<theType, intType>& Floor() { return Apply(std::bind(&CHArray<theType, intType>::InternalFloor, this, std::placeholders::_1)); }
	//Make equal to closest integer that is equal to value or higher
	CHArray<theType, intType>& Ceil() { return Apply(std::bind(&CHArray<theType, intType>::InternalCeil, this, std::placeholders::_1)); }
	//Replace with 1/array
	CHArray<theType, intType>& Inverse() { return Apply([](theType val)->theType {return 1 / val; }); }
	//Replace with -1 or 1 depending on the sign of the value
	CHArray<theType, intType>& Sign() { return Apply([](theType val)->theType {if (val < 0) return -1; else return 1; }); }
	//Multiply by -1
	CHArray<theType, intType>& InvertSign() { return (*this) *= -1; }
	
	intType FindBin(theType val) const;				//For increasing arrays with at least 2 points
	intType FindBinEquispaced(theType val) const;	//For increasing arrays with at least 2 points
	template<class resType>
	void Histogram(const CHArray<theType,intType>& binningArray, CHArray<resType,intType>& result, bool fEquispaced) const;
	template<class resType>
	void Histogram(theType start, theType end, intType numBins, CHArray<resType,intType>& result) const;
	theType Interpolate(double index) const;	//Linear interpolation at non-integer index
	double GetRealIndex(theType val) const;   		//For an increasing array, the non-integer index that this value would be placed at
												//that is, val == Interpolate(GetRealIndex(val)) (if val is within range of the increasing array)

	CHArray<theType,intType>& ExportPart(CHArray<theType,intType>& target, intType from, intType numToCopy) const;
	CHArray<theType,intType>& ExportFirst(CHArray<theType,intType>& target, intType numToCopy) const {return ExportPart(target,0,numToCopy);}
	CHArray<theType,intType>& ExportLast(CHArray<theType,intType>& target, intType numToCopy) const {return ExportPart(target,numPoints-numToCopy,numToCopy);}
	CHArray<theType,intType>& Concatenate(const CHArray<theType,intType>& rhs);		//Adds rhs to the end of the array, resizing *this

	//Sorting and permutatations
	void Sort(bool fDescending=false);		//Uses comb sort		
	void SortPermutation(CHArray<intType,intType>& intArray, bool fDescending=false, bool fStableSort=false) const;
	//Sorts provided indices according to the values in the array at those indices
	void SortIndices(CHArray<intType,intType>& indices, bool fDescending=false, bool fStableSort=false) const;
	//The first numToFind points of the sorted array will be in "result", no stable sort version
	void PartialSort(CHArray<theType,intType>& result, intType numToFind, bool fDescending=false) const;
	void PartialSortPermutation(CHArray<intType,intType>& perm, intType numToFind, bool fDescending=false) const;
	void PartialSortIndices(CHArray<intType,intType>& indices, intType numToFind, bool fDescending=false) const;
	void Permute(const CHArray<intType,intType>& permutation);
	void InvertPermutation();
	void SelectFrom(const CHArray<theType,intType>& source, const CHArray<intType,intType>& indices);	//Selects points from source according to indices
	
	//Non-comparison radix sort
	void RadixSort(bool fDescending=false);	//Stable non-comparison sort for any numerical data, relies on bit order - careful!
	void RadixSortWithPerm(CHArray<intType,intType>& perm, bool fDescending=false);	//Sorts the array and produces a permutation needed for the sort

private:	//Private auxilliary functions for radix sorting
	void InternalRadixSort(CHArray<theType,intType>& copyArray, bool fDescending);
	void InternalRadixSortWithPerm(CHArray<theType,intType>& copyArray,CHArray<intType,intType>& perm,
										CHArray<intType,intType>& copyPerm, bool fDescending);

public:
	void Reverse();											//Reverses the order of the elements
	void SwitchElements(intType index1, intType index2);	//Switches the two elements
	void TrimRight(intType newNumPoints);
	void TrimLeft(intType newNumPoints);
	
private:
	void DeleteArray();

public:
	//Apply a function to every element
	CHArray<theType, intType>& Apply(std::function<theType(theType)> func);


public:
	//////////////////////////////////////////////////Operators
	theType& operator[](intType num) const {return arr[num];}
	
	CHArray<theType,intType>& operator=(const CHArray<theType,intType>& rhs);

	CHArray<theType,intType>& operator=(theType val);
	CHArray<theType,intType> operator*(const CHArray<theType,intType>& rhs) 
						{return (BinaryOperationTwoArrays(rhs,&CHArray<theType,intType>::Multiply));}
	CHArray<theType,intType> operator+(const CHArray<theType,intType>& rhs)
						{return (BinaryOperationTwoArrays(rhs,&CHArray<theType,intType>::Add));}
	CHArray<theType,intType> operator-(const CHArray<theType,intType>& rhs)
						{return (BinaryOperationTwoArrays(rhs,&CHArray<theType,intType>::Subtract));}
	CHArray<theType,intType> operator/(const CHArray<theType,intType>& rhs)
						{return (BinaryOperationTwoArrays(rhs,&CHArray<theType,intType>::Divide));}
	CHArray<theType,intType> operator%(const CHArray<theType,intType>& rhs) 
						{return (BinaryOperationTwoArrays(rhs,&CHArray<theType,intType>::Remainder));}
	CHArray<theType,intType> operator^(const CHArray<theType,intType>& rhs)
						{return (BinaryOperationTwoArrays(rhs,&CHArray<theType,intType>::Power));}
	CHArray<theType,intType> operator*(theType val)
						{return (BinaryOperationtheType(val,&CHArray<theType,intType>::Multiply));}
	CHArray<theType,intType> operator+(theType val)
						{return (BinaryOperationtheType(val,&CHArray<theType,intType>::Add));}
	CHArray<theType,intType> operator-(theType val)
						{return (BinaryOperationtheType(val,&CHArray<theType,intType>::Subtract));}
	CHArray<theType,intType> operator/(theType val)
						{return (BinaryOperationtheType(val,&CHArray<theType,intType>::Divide));}
	CHArray<theType,intType> operator%(theType val)
						{return (BinaryOperationtheType(val,&CHArray<theType,intType>::Remainder));}
	CHArray<theType,intType> operator^(theType val)
						{return (BinaryOperationtheType(val,&CHArray<theType,intType>::Power));}
	CHArray<theType,intType>& operator*=(const CHArray<theType,intType>& rhs)
						{return (AssignmentTwoArrays(rhs,&CHArray<theType,intType>::Multiply));}
	CHArray<theType,intType>& operator+=(const CHArray<theType,intType>& rhs)
						{return (AssignmentTwoArrays(rhs,&CHArray<theType,intType>::Add));}
	CHArray<theType,intType>& operator-=(const CHArray<theType,intType>& rhs)
						{return (AssignmentTwoArrays(rhs,&CHArray<theType,intType>::Subtract));}
	CHArray<theType,intType>& operator/=(const CHArray<theType,intType>& rhs)
						{return (AssignmentTwoArrays(rhs,&CHArray<theType,intType>::Divide));}
	CHArray<theType,intType>& operator%=(const CHArray<theType,intType>& rhs)
						{return (AssignmentTwoArrays(rhs,&CHArray<theType,intType>::Remainder));}
	CHArray<theType,intType>& operator^=(const CHArray<theType,intType>& rhs)
						{return (AssignmentTwoArrays(rhs,&CHArray<theType,intType>::Power));}
	CHArray<theType,intType>& operator*=(theType val)
						{return (AssignmenttheType(val,&CHArray<theType,intType>::Multiply));}
	CHArray<theType,intType>& operator+=(theType val)
						{return (AssignmenttheType(val,&CHArray<theType,intType>::Add));}
	CHArray<theType,intType>& operator-=(theType val)
						{return (AssignmenttheType(val,&CHArray<theType,intType>::Subtract));}
	CHArray<theType,intType>& operator/=(theType val)
						{return (AssignmenttheType(val,&CHArray<theType,intType>::Divide));}
	CHArray<theType,intType>& operator%=(theType val)
						{return (AssignmenttheType(val,&CHArray<theType,intType>::Remainder));}
	CHArray<theType,intType>& operator^=(theType val)
						{return (AssignmenttheType(val,&CHArray<theType,intType>::Power));}
	CHArray<theType,intType>& Log(theType val=(theType)2.7182818284590452)
						{return (AssignmenttheType(val,&CHArray<theType,intType>::Logarithm));}
	CHArray<theType,intType>& Exp(theType val)
						{return (AssignmenttheType(val,&CHArray<theType,intType>::InterchangedPower));}
	CHArray<theType,intType>& Exp();

	bool operator==(CHArray<theType,intType>& rhs);
	bool operator!=(CHArray<theType,intType>& rhs);



//////////////////Operator functions
private:

	CHArray<theType,intType> BinaryOperationTwoArrays
		(const CHArray<theType,intType>& rhs,theType(CHArray<theType,intType>::*fPointer)(theType,theType));

	CHArray<theType,intType> BinaryOperationtheType
		(theType val,theType(CHArray<theType,intType>::*fPointer)(theType,theType));

	CHArray<theType,intType>& AssignmentTwoArrays
		(const CHArray<theType,intType>& rhs,theType(CHArray<theType,intType>::*fPointer)(theType,theType));

	CHArray<theType,intType>& AssignmenttheType
		(theType val,theType(CHArray<theType,intType>::*fPointer)(theType,theType));

	theType InternalSqrt(theType a) { return sqrt(a); };
	theType InternalFloor(theType a) { return floor(a); };
	theType InternalCeil(theType a) { return ceil(a); }
	theType InternalAbs(theType a) { return abs(a); };
	theType Remainder(theType a, theType b) {return a%b;}
	theType Add(theType a, theType b) {return a+b;}
	theType Subtract(theType a, theType b) {return a-b;}
	theType Multiply(theType a, theType b) {return a*b;}
	theType Divide(theType a, theType b) {return a/b;}
	theType Power(theType a, theType b) {return pow(a,b);}
	theType InterchangedPower(theType a, theType b) {return pow(b,a);}
	theType Logarithm(theType a, theType b) {return log(a)/log(b);}  //Log_b_(a)
};

template <class theType, class intType>
CHArray<theType, intType>& CHArray<theType, intType>::Apply(std::function<theType(theType)> func)
{
	for (int i = 0; i < numPoints; i++) arr[i] = func(arr[i]);

	return *this;
}

//Finds the position that splits the sorted array in two
//Where the sum of squared deviations from the centers of the two new arrays is minimized
//Always returns a split - i.e. none of the new arrays is equal to the old array for arrays larger than 1
//Position is the end() position for the leftmost new array
//Used in PDDP cluster splitting, for example
//Call on sorted arrays
//O(N)
//Optionally returns the reduction in squared devs after division
template <class theType,class intType>
intType CHArray<theType,intType>::OptimalSplitSorted(theType* reductionInDev) const
{
	if(numPoints<2) {if(reductionInDev) *reductionInDev=0; return 0;}

	//The sum of squared devs for each of the two new arrays is given by
	//Omega = Sum[a^2] - (Sum[a]^2)/N

	//Should we introduce an etropy term that favors more equal splitting?
	//It would have the form S = T * (n1 + n2) ( x1*Log[x1] + x2*Log[x2] )
	//Where x1=n1/(n1+n2), and x2=n2/(n1+n2)
	//It would enter into the curVal expression with a "+" sign

	//We will keep the sums for both new arrays and increment/decrement as we move to the right
	intType nLeft=0;
	theType sum2Left=0;
	theType sum1Left=0;

	intType nRight=numPoints;
	theType sum2Right = SumOfSquares();
	theType sum1Right = Sum();

	//These will be set on the first iteration
	intType curMinPos=0;
	theType curMinVal=0;
	if(reductionInDev) *reductionInDev = sum2Right - sum1Right * sum1Right / (theType)nRight;		//S term would aslo appear here

	//We need to iterate through all possible split positions - curVal is not a single-minimum function
	do
	{
		sum2Left+=arr[nLeft]*arr[nLeft];
		sum1Left+=arr[nLeft];

		sum2Right-=arr[nLeft]*arr[nLeft];
		sum1Right-=arr[nLeft];

		nLeft++;
		nRight--;

		//Sum of quadratic deviations from the means of the two elements
		//S term would appear here with a "+" sign
		theType curVal = sum2Left - sum1Left*sum1Left / (theType)nLeft + sum2Right - sum1Right * sum1Right / (theType)nRight;

		if(curVal < curMinVal || nLeft==1)
		{
			curMinVal = curVal;
			curMinPos = nLeft;
		}
	}
	while(nLeft < (numPoints-1));

	if(reductionInDev) *reductionInDev -= curMinVal;
	return curMinPos;
}


//Counts runs of identical elements in an array
//Especially useful for sorted data
//Exports the values and lengths for all runs
template <class theType,class intType>
void CHArray<theType,intType>::CountRuns(CHArray<theType,intType>& runVals, CHArray<intType,intType>& runLengths) const
{
	if(numPoints==0) {runVals.SetNumPoints(0);runLengths.SetNumPoints(0);return;};

	//First, count the total number of runs
	intType numRuns=1;
	for(intType i=1;i<numPoints;i++)
	{
		if(arr[i]!=arr[i-1]) numRuns++;
	}
	runVals.ResizeIfSmaller(numRuns);
	runLengths.ResizeIfSmaller(numRuns);
	runVals.EraseArray();
	runLengths.EraseArray();

	//Go through the array and extract run values and data
	runVals.AddPoint(arr[0]);
	intType curLength=1;
	for(intType i=1;i<numPoints;i++)
	{
		if(arr[i]!=arr[i-1])
		{
			runVals.AddPoint(arr[i]);
			runLengths.AddPoint(curLength);
			curLength=1;
		}
		else curLength++;
	}
	runLengths.AddPoint(curLength);
}


//Builds forward and reverse renumbering to a reduced subset based on this array
//The value of -1 in *this signifies which indices will not be included in the reduced subset
template <class theType,class intType>
template <class argType, class argIntType>
void CHArray<theType,intType>::BuildRenumbering(CHArray<argType,argIntType>& forwardRenum,
												CHArray<argType,argIntType>& reverseRenum) const
{
	forwardRenum.ResizeIfSmaller((argIntType)numPoints);

	//We'll do a double pass to avoid resizing reverseRenum
	argIntType numAdded=0;
	for(intType i=0;i<numPoints;i++)
	{
		if(arr[i]!=-1)
		{
			forwardRenum.AddPoint(numAdded);
			numAdded++;
		}
		else forwardRenum.AddPoint((argType)-1);
	}

	//Now the reverse renum
	reverseRenum.ResizeIfSmaller(numAdded,true);
	for(argIntType i=0; i<forwardRenum.numPoints; i++)
	{
		if(forwardRenum.arr[i]!=-1) reverseRenum[(argIntType)forwardRenum.arr[i]]=(argType)i;
	}
}


//Renumbers values in the array by taking the index from renum at the location specified by the value
template <class theType,class intType>
template<class argIntType>
void CHArray<theType,intType>::Renumber(const CHArray<intType,argIntType>& renum)
{
	for(intType i=0;i<numPoints;++i)
	{
		arr[i]=(theType)renum[(argIntType)arr[i]];
	}
}

//Switches the two array elements
//Using a temporary variable
template <class theType, class intType>
void CHArray<theType, intType>::SwitchElements(intType index1, intType index2)
{
	theType temp = arr[index1];
	arr[index1] = arr[index2];
	arr[index2] = temp;
}

//Helper function for radix sort
template <class theType,class intType>
void CHArray<theType,intType>::InternalRadixSort(CHArray<theType,intType>& copyArray,bool fDescending)
{
	//copyArray must have been correctly allocated
	//An internal function that sorts a positive array with radix sort
	
	///////////////////////////////////////////
	CHArray<intType,short> buckets(256+1,true);		//8-bit buckets
	char dataSize=sizeof(theType);	//Size of data points in bytes

	//Perform sorting for all bytes starting from the least significant byte
	for(char i=0;i<dataSize;i++)
	{
		buckets=0;

		for(intType j=0;j<numPoints;j++)	//first pass - counting numbers in each bucket
		{
			short curBucket=*(((uchar*)(arr+j))+i);
			if(fDescending) curBucket=(short)255-curBucket;
			
			buckets.arr[curBucket+1]++;
		}
		buckets.IntegralForm();

		for(intType j=0;j<numPoints;j++)	//second pass - sorting according to the current byte
		{
			short curBucket=*(((uchar*)(arr+j))+i);
			if(fDescending) curBucket=(short)255-curBucket;

			copyArray.arr[buckets.arr[curBucket]]=arr[j];
			buckets.arr[curBucket]++;
		}
		
		CopyFromPointer(copyArray.arr,numPoints);	//Copy without the possibility of array resizing
	}
}


template <class theType,class intType>
void CHArray<theType,intType>::RadixSort(bool fDescending)
{
	if(numPoints<2) return;

	//Find out whether the data type is signed or unsigned
	bool fSigned=IsDataSigned();

	//Auxilliary structures
	CHArray<theType,intType> copyArray(numPoints,true);
	CHArray<intType,short> buckets(256+1,true);

	if(!fSigned)	//Unsigned data - simplest case
	{
		InternalRadixSort(copyArray,fDescending);
	}
	else	//signed
	{
		//Lose signed zeros
		for(intType i=0;i<numPoints;i++)
		{
			if(arr[i]==(theType)0) arr[i]=+0;
		}

		//Separate negative and positive parts
		intType numPositive=0;
		intType numNegative=0;
		CHArray<theType,intType> posPart(arr,0,true);	//virtual arrays
		CHArray<theType,intType> negPart(arr,0,true);

		for(intType i=0;i<numPoints;i++)
		{
			if(arr[i]>=0) numPositive++;
			else numNegative++;
		}

		if(fDescending)	//positives come before negatives
		{
			posPart.SetVirtual(arr,numPositive);
			negPart.SetVirtual(arr+numPositive,numNegative);
		}
		else	//ascending - negatives come before positives
		{
			negPart.SetVirtual(arr,numNegative);
			posPart.SetVirtual(arr+numNegative,numPositive);
		}

		copyArray.CopyFromPointer(arr,numPoints);

		intType curPos=0;
		intType curNeg=0;
		for(intType i=0;i<numPoints;i++)
		{
			if(copyArray.arr[i]>=0) posPart[curPos++]=copyArray.arr[i];
			else negPart[curNeg++]=copyArray.arr[i];
		}

		negPart.InvertSign();	//Invert negatives

		//Sort
		posPart.InternalRadixSort(copyArray,fDescending);
		negPart.InternalRadixSort(copyArray,!fDescending);

		negPart.InvertSign();	//Invert negatives again
	}
}


//Internal auxilliary function used by RadixSortWithPerm
//copyArray, perm and copyPerm must have been allocated previously
template <class theType,class intType>
void CHArray<theType,intType>::InternalRadixSortWithPerm(CHArray<theType,intType>& copyArray,
										CHArray<intType,intType>& perm, CHArray<intType,intType>& copyPerm, bool fDescending)
{
	char dataSize=sizeof(theType);	//Size of data points in bytes
	CHArray<intType,short> buckets(256+1,true);

	//Perform sorting for all bytes starting from the least significant byte
	for(char i=0;i<dataSize;i++)
	{
		buckets=0;

		for(intType j=0;j<numPoints;j++)	//first pass - counting numbers in each bucket
		{
			short curBucket=*(((uchar*)(arr+j))+i);
			if(fDescending) curBucket=(short)255-curBucket;

			buckets.arr[curBucket+1]++;
		}
		buckets.IntegralForm();

		for(intType j=0;j<numPoints;j++)	//second pass - sorting according to the current byte
		{
			short curBucket=*(((uchar*)(arr+j))+i);
			if(fDescending) curBucket=(short)255-curBucket;

			copyArray.arr[buckets.arr[curBucket]]=arr[j];
			copyPerm.arr[buckets.arr[curBucket]]=perm.arr[j];
			buckets.arr[curBucket]++;
		}

		CopyFromPointer(copyArray.arr,numPoints);		//Copy without the possibility of array resizing
		perm.CopyFromPointer(copyPerm.arr,numPoints);
	}
}


template <class theType,class intType>
void CHArray<theType,intType>::RadixSortWithPerm(CHArray<intType,intType>& perm, bool fDescending)
{
	perm.ResizeIfSmaller(numPoints,true);
	perm.SetValToPointNum();
	if(numPoints<2) return;

	//Find out whether the data type is signed or unsigned
	bool fSigned=IsDataSigned();

	//Auxilliary structures
	CHArray<theType,intType> copyArray(numPoints,true);
	CHArray<intType,intType> copyPerm(perm);
	CHArray<intType,short> buckets(256+1,true);

	if(!fSigned)	//Unsigned data - simplest case
	{
		InternalRadixSortWithPerm(copyArray,perm,copyPerm,fDescending);
	}
	else	//signed
	{
		//Lose signed zeros
		for(intType i=0;i<numPoints;i++)
		{
			if(arr[i]==(theType)0) arr[i]=+0;
		}

		//Separate negative and positive parts
		intType numPositive=0;
		intType numNegative=0;
		CHArray<theType,intType> posPart(arr,0,true);	//virtual arrays
		CHArray<theType,intType> negPart(arr,0,true);
		CHArray<intType,intType> posPartPerm(perm.arr,0,true);
		CHArray<intType,intType> negPartPerm(perm.arr,0,true);

		for(intType i=0;i<numPoints;i++)
		{
			if(arr[i]>=0) numPositive++;
			else numNegative++;
		}

		if(fDescending)	//positives come before negatives
		{
			posPart.SetVirtual(arr,numPositive);
			negPart.SetVirtual(arr+numPositive,numNegative);
			posPartPerm.SetVirtual(perm.arr,numPositive);
			negPartPerm.SetVirtual(perm.arr+numPositive,numNegative);
		}
		else	//ascending - negatives come before positives
		{
			negPart.SetVirtual(arr,numNegative);
			posPart.SetVirtual(arr+numNegative,numPositive);
			negPartPerm.SetVirtual(perm.arr,numNegative);
			posPartPerm.SetVirtual(perm.arr+numNegative,numPositive);
		}

		copyArray.CopyFromPointer(arr,numPoints);

		intType curPos=0;
		intType curNeg=0;
		for(intType i=0;i<numPoints;i++)
		{
			if(copyArray.arr[i]>=0)
			{
				posPart[curPos]=copyArray.arr[i];
				posPartPerm[curPos]=copyPerm.arr[i];
				curPos++;
			}
			else
			{
				negPart[curNeg]=copyArray.arr[i];
				negPartPerm[curNeg]=copyPerm.arr[i];
				curNeg++;
			}
		}

		negPart.InvertSign();	//Invert negatives

		//Sort
		posPart.InternalRadixSortWithPerm(copyArray,posPartPerm,copyPerm,fDescending);
		negPart.InternalRadixSortWithPerm(copyArray,negPartPerm,copyPerm,!fDescending);

		negPart.InvertSign();	//Invert negatives again
	}
}

//O(N^2) convolution - dimensions of (*this) and rhs may be different
template <class theType,class intType>
void CHArray<theType,intType>::Convolve(const CHArray<theType,intType>& rhs, CHArray<theType,intType>& result) const
{
	intType resultNumPoints=numPoints+rhs.numPoints-1;
	result.ResizeArray(resultNumPoints, true);
	result=0;
	
	for(intType i=0; i<numPoints; i++)
	{
		theType curVal=arr[i];

		for (intType j = 0; j<rhs.numPoints; j++) result.arr[i + j] += curVal * rhs.arr[j];
	}
}


//direct product with a shift
//arrays do not have to have the same dimensions
//"shift" positions rhs over (*this) 
template <class theType,class intType>
theType CHArray<theType,intType>::ShiftedDirectProduct(const CHArray<theType,intType>& rhs, intType shift) const
{
	//startPos and endPos positions in (*this)
	intType startPos=shift;
	if(startPos<0) startPos=0;
	
	intType endPos=shift+rhs.numPoints;
	if(endPos>numPoints) endPos=numPoints;

	theType result=0;
	for (intType i = startPos; i<endPos; i++) result += arr[i] * rhs.arr[i - shift];

	return result;
}

//Linear interpolation at a non-integer index
template <class theType,class intType>
theType CHArray<theType,intType>::Interpolate(double index) const
{
	intType left, right;

	double f = floor(index);
	intType intF = (intType)f;

	if(intF < 0) {left=0; right=0;}
	if(intF >= (numPoints-1)) {left=numPoints-1; right=numPoints-1;}
	if(intF >= 0 && intF<(numPoints-1)) {left=intF; right=left+1;}
	
	double residue = index - f;

	return arr[left]+(arr[right]-arr[left])*residue;
}

//Get non-integer index that val would be placed at in the increasing array
// val == Interpolate(GetRealIndex(val)), if val is within the range of the array
template <class theType, class intType>
double CHArray<theType,intType>::GetRealIndex(theType val) const
{
	if(numPoints < 2) return 0;

	//First element that is greater or equal to val
	intType lowerIndex = (intType)(std::lower_bound(begin(),end(),val) - begin());
	if(lowerIndex == numPoints) return (numPoints-1);	//Val is larger than the largest element
	if(lowerIndex == 0) return 0;						//Val is smaller than the first element

	//By now lowerIndex is in [1, numPoints-1]
	if(arr[lowerIndex] == val) return lowerIndex;		//Exact match

	//By now the value will be indexed between (lowerIndex - 1) and lowerIndex
	return (double)(lowerIndex - 1) + (double)(val - arr[lowerIndex-1])/(double)(arr[lowerIndex]-arr[lowerIndex-1]);
}


//Orthogonalize with respect to another array
template <class theType,class intType>
CHArray<theType,intType>& CHArray<theType,intType>::Orthogonalize(const CHArray<theType,intType>& rhs)
{
	//Compute direct product and self-product
	theType dirProduct=DirectProduct(rhs);
	theType sumOfSquares=rhs.SumOfSquares();

	//Subtract projection
	MultiplyAdd(rhs,-dirProduct/sumOfSquares);
	return (*this);
}

//Exponentiate all values
template <class theType,class intType>
CHArray<theType,intType>& CHArray<theType,intType>::Exp()
{
	for(intType i=0;i<numPoints;i++)
	{
		arr[i]=exp(arr[i]);
	}

	return (*this);
}

template <class theType,class intType>
intType CHArray<theType,intType>::GetNumNonZeros() const
{
	intType result=0;

	for (intType i = 0; i<numPoints; i++) if (arr[i] != 0) result++;

	return result;
}

template <class theType,class intType>
template<class resType>
void CHArray<theType,intType>::Histogram(theType start, theType end, intType numBins, CHArray<resType,intType>& result) const
{
	//Computes the number of points from this array that go into the numBins bins, running from start to end
	//End is greater than start

	result.ResizeArray(numBins,true);
	result=0;

	theType invStep=(theType)numBins/(end-start);	//inverse step, to avoid multiple divisions

	intType bin;
	for(intType i=0; i<numPoints; i++)
	{
		if(arr[i]>end || arr[i]<start) continue;	//point out of bounds
		bin=(intType)( (arr[i]-start) * invStep);
		result.arr[bin] + =1;						//Adding (resType)1
	}

}

template <class theType,class intType>
template<class resType>
void CHArray<theType,intType>::Histogram(const CHArray<theType,intType>& binningArray, CHArray<resType,intType>& result, bool fEquispaced) const
{
	//Computes the number of points from this array that go into the bins of the (increasing) binning array
	//The number of points in result will be number of points in the binning array minus 1
	//fEquispaced accelerates computation if binning array is equispaced
	//The type of the result array can vary - int, float, double, etc.

	if(fEquispaced)	//Equispaced arrray - call efficient function and return
	{
		Histogram(binningArray[0],binningArray[binningArray.numPoints-1],binningArray.numPoints-1,result);
		return;
	}

	//Array is not treated as equispaced
	result.ResizeArray(binningArray.numPoints-1,true);
	result=0;

	intType bin;
	for(intType i=0; i<numPoints; i++)
	{
		bin=binningArray.FindBin(arr[i]);

		if(bin!=-1) result.arr[bin]+=1;	//Adding (resType)1
	}
}


//Composing initial index array, for example, for compressed-row (or column) based sparse matrix
//This array must contain sorted indices (0 to max numIndices-1 assumed)
//Some indices may be missing; 0 and the last index may also be missing
//numIndices determines how long the result will be (in case the last index is missing)
template <class theType,class intType>
void CHArray<theType,intType>::InitialIndexArray(CHArray<intType,intType>& result, intType numIndices) const
{
	result.ResizeArray(numIndices+1);		//+1 for artificial last point

	intType curIndex=0;
	result.AddPoint(0);
	for(intType i=0; i<numPoints; i++)
	{
		while(arr[i]>curIndex) {curIndex++; result.AddPoint(i);}
	}

	//Add end points - in case several indices at the end are not present
	//Last point is artificial, so that N(i)=index(i+1)-index(i)
	while(!result.IsFull()) result.AddPoint(numPoints);
}


//Sets all points to random values from the interval of [0.0,1.0]
template <class theType,class intType>
CHArray<theType, intType>& CHArray<theType, intType>::SetToRandom()
{
	for(intType i=0; i<numPoints; i++)
	{
		arr[i] = ((theType)rand())/((theType)RAND_MAX);
	}
}


//Returns randomly chosen index from 0 to numPoints-1
//Return -1 if the array is empty
template <class theType,class intType>
int CHArray<theType,intType>::RandomIndex() const
{
	if(numPoints==0) return -1;

	return rand() % numPoints;
}

template <class theType,class intType>
void CHArray<theType,intType>::PartialSort(CHArray<theType,intType>& result, intType numToFind, bool fDescending) const
{
	if(numToFind>numPoints) numToFind=numPoints;

	result.ResizeIfSmaller(numPoints);
	result.AddFromArray(*this);

	if(fDescending)
		std::partial_sort(result.begin(),result.begin()+numToFind,result.end(),std::greater<theType>());
	else
		std::partial_sort(result.begin(),result.begin()+numToFind,result.end());

	result.SetNumPoints(numToFind);
}

//Partial sorts provided indices according to the values in the array at those indices
template <class theType,class intType>
void CHArray<theType,intType>::PartialSortIndices(CHArray<intType,intType>& indices, intType numToFind, bool fDescending) const
{
	if(fDescending)
		std::partial_sort(indices.begin(),indices.begin()+numToFind,indices.end(),
						[this](const intType& a, const intType& b)->bool{return arr[a] > arr[b];});
	else
		std::partial_sort(indices.begin(),indices.begin()+numToFind,indices.end(),
						[this](const intType& a, const intType& b)->bool{return arr[a] < arr[b];});
}

template <class theType,class intType>
void CHArray<theType,intType>::PartialSortPermutation(CHArray<intType,intType>& perm, intType numToFind, bool fDescending) const
{
	if(numToFind>numPoints) numToFind=numPoints;

	perm.ResizeIfSmaller(numPoints,true);
	perm.SetValToPointNum();

	PartialSortIndices(perm,numToFind,fDescending);

	perm.SetNumPoints(numToFind);
}

template <class theType,class intType>
CHArray<theType,intType>& CHArray<theType,intType>::MultiplyAdd(const CHArray<theType,intType>& rhs, theType factor)
{
	if(numPoints!=rhs.numPoints) return *this;

	for (intType i = 0; i<numPoints; i++) arr[i] += rhs.arr[i] * factor;

	return *this;
}

template <class theType,class intType>
bool CHArray<theType,intType>::IsPresent(const theType& val) const
{
	for (intType i = 0; i<numPoints; i++) if (arr[i] == val) return true;

	return false;
}

template <class theType,class intType>
intType CHArray<theType,intType>::CountOccurrence(const theType& val) const
{
	intType result=0;
	for(intType i=0;i<numPoints;++i)
	{
		if(arr[i]==val) result++;
	}

	return result;
}

template <class theType,class intType>
CHArray<theType,intType>::CHArray(intType theSize, bool setMaxNumPoints):
numPoints(0),
arr(0)
{
	size=theSize;
	arr=new theType[(size_t)size];
	if(setMaxNumPoints) SetNumPoints(size);				//if the array is used as s "vector"
	fVirtual=false;
}

template <class theType,class intType>
CHArray<theType,intType>::CHArray(theType start, theType end, intType theNumPoints):
arr(0)
{
	numPoints=theNumPoints;
	size=theNumPoints;
	fVirtual=false;
	arr=new theType[size];

	theType step;
	if(theNumPoints!=1) step=(end-start)/(theType)(theNumPoints-1);
	else step=0;
	theType curVal=start;
	for(intType i=0; i<numPoints; i++)
	{
		arr[i]=curVal;
		curVal+=step;
	}
}

//Can create a virtual array
template <class theType,class intType>
CHArray<theType,intType>::CHArray(theType* thePointer, intType theSize, bool isVirtual)
{
	fVirtual=isVirtual;
	size=theSize;
	numPoints=size;

	if(fVirtual)		//Create a virtual array - caution! Array cannot be deleted or allocated
	{
		arr=thePointer;
	}
	else
	{
		arr=new theType[size];

		for (intType i = 0; i<numPoints; i++) arr[i] = thePointer[i];
	}
}

template <class theType,class intType>
void CHArray<theType,intType>::CopyFromPointer(const theType* thePointer, intType numToCopy)
{
	ResizeIfSmaller(numToCopy,true);

	for (intType i = 0; i<numToCopy; i++) arr[i] = thePointer[i];
}

//Const version - cannot create a virtual array with this one
template <class theType,class intType>
CHArray<theType,intType>::CHArray(const theType* thePointer, intType theSize)
{
	fVirtual=false;
	size=theSize;
	numPoints=size;

	arr=new theType[size];

	for (intType i = 0; i<numPoints; i++) arr[i] = thePointer[i];
}

template <class theType,class intType>
CHArray<theType,intType>::CHArray(const CHArray<theType,intType>& rhs)			//Copy constructor
{	
	fVirtual=false;
	size=rhs.GetSize();
	arr=new theType[size];
	
	numPoints=rhs.numPoints;
	for (intType i = 0; i<numPoints; i++) arr[i] = rhs.arr[i];
}

template <class theType,class intType>
template<class rhsType, class rhsIntType>
void CHArray<theType,intType>::ImportFrom(const CHArray<rhsType,rhsIntType>& rhs)
{
	ResizeIfSmaller((intType)rhs.Count(),true);

	for(intType i=0; i<numPoints; i++)
	{
		arr[i]=(theType)(rhs.arr[(rhsIntType)i]);
	}
}

template <class theType,class intType>
void CHArray<theType,intType>::SelectFrom(const CHArray<theType,intType>& source, const CHArray<intType,intType>& indices)
{
	ResizeIfSmaller(indices.Count(),true);

	for(intType i=0; i<numPoints; i++)
	{
		arr[i] = source.arr[indices[i]];
	}
}

template <class theType,class intType>
CHArray<theType,intType>::CHArray(const BString& fileName)
{
	fVirtual=false;
	size=1;
	numPoints=0;
	arr=new theType[size];

	Load(fileName);
}

template <class theType,class intType>
CHArray<theType,intType>::~CHArray()
{
	DeleteArray();
}

template <class theType,class intType>
void CHArray<theType,intType>::DeleteArray()
{
	if( arr!=0 && !fVirtual) delete[] arr;
}

template <class theType,class intType>
void CHArray<theType,intType>::SetVirtual(theType* thePointer, intType theSize)
{
	DeleteArray();

	fVirtual=true;
	size=theSize;
	numPoints=size;

	arr=thePointer;
}

//Removes points that are the same as the preceding element - call on sorted arrays
template <class theType,class intType>
void CHArray<theType,intType>::RemoveRepetitions()
{
	if(numPoints==0) return;

	intType numInserted=1;
	intType readPoint;

	for(readPoint=1;readPoint<numPoints;readPoint++)
	{
		if(arr[readPoint]==arr[numInserted-1]) continue;
		else
		{
			if(readPoint!=numInserted) arr[numInserted]=arr[readPoint];
			numInserted++;
		}
	}

	SetNumPoints(numInserted);
}

//Decimates the array with provided step, starting from index 0 (0 is retained)
//Every step-th element is retained
template <class theType, class intType>
void CHArray<theType, intType>::Decimate(intType step)
{
	if (step <= 1 || numPoints < 2) return;

	intType j = 1;			//Counting over copy destinations - increments by 1
	intType k = step;		//Counting by copy source - increments by step
	intType numCopied = 1;	//Zeroth element has already been retained

	while (k < numPoints)
	{
		arr[j] = arr[k];
		j++;
		k += step;
		numCopied++;
	}

	SetNumPoints(numCopied);
}

template <class theType,class intType>
void CHArray<theType,intType>::DeletePointAt(intType index)
{
	if(numPoints==0) return;
	if(index<0) return;
	if(index>(numPoints-1)) return;

	for (intType i = index; i<(numPoints - 1); i++) arr[i] = arr[i + 1];

	numPoints--;
}

template <class theType,class intType>
bool CHArray<theType,intType>::operator==(CHArray<theType,intType>& rhs)
{
	if(numPoints!=rhs.numPoints) return false;

	for(intType i=0; i<numPoints; i++)
	{
		if(arr[i] != rhs.arr[i]) return false;
	}

	return true;
}

template <class theType,class intType>
bool CHArray<theType,intType>::operator!=(CHArray<theType,intType>& rhs)
{
	return !((*this)==rhs);
}

template <class theType,class intType>
void CHArray<theType,intType>::TrimRight(intType newNumPoints)
{
	if(newNumPoints>=numPoints) return;
	
	SetNumPoints(newNumPoints);
}

template <class theType,class intType>
void CHArray<theType,intType>::TrimLeft(intType newNumPoints)
{
	if(newNumPoints >= numPoints) return;

	intType curPos=0;
	for(intType i = numPoints-newNumPoints; i < numPoints; i++)
	{
		arr[curPos++]=arr[i];
	}

	SetNumPoints(newNumPoints);
}

template <class theType,class intType>
void CHArray<theType,intType>::Reverse()
{
	if(numPoints<2) return;

	theType interm;
	intType halfway=numPoints/2;
	for(intType i=0; i<halfway; i++)
	{
		intType left=i;
		intType right=numPoints-1-i;

		interm=arr[left];
		arr[left]=arr[right];
		arr[right]=interm;
	}
}

template <class theType,class intType>
bool CHArray<theType,intType>::ReadBinary(const BString& fileName)
{
	FILE* fp;
	fp=fopen(fileName,"rb");		//Open as binary
	if(fp==NULL)
	{
		std::cerr << "Could not load: " << fileName << ".";
		return false;
	}

	fseek_large(fp,0,SEEK_END);				//Seek to the end of file
	long long fileLength=ftell_large(fp);	//Read position
	fseek_large(fp,0,SEEK_SET);				//Seek to the start of file
	
	intType newNumPoints=(intType)fileLength/sizeof(theType);
	ResizeArray(newNumPoints, true);
	
        size_t res = fread(arr,1,(size_t)fileLength,fp);

	fclose(fp);
	return true;
}

template <class theType,class intType>
void CHArray<theType,intType>::SerializeFromBuffer(char*& buffer)
{
	//Read number of points and resize array
	intType newNumPoints=*((intType*)buffer); buffer+=sizeof(intType);
	ResizeArray(newNumPoints,true);

	//Read array elements
	theType* p=(theType*)buffer;
	for(intType i=0;i<numPoints;i++)
	{
		arr[i]=*p;
		p++;
	}

	buffer=(char*)p;
}

//Full instantiation to <BString, int>
template <> inline void CHArray<BString,int>::SerializeFromBuffer(char*& buffer)
{
	//Read number of points and resize array
	int newNumPoints=*((int*)buffer); buffer+=sizeof(int);
	ResizeArray(newNumPoints,true);

	//Read array elements
	for(int i=0;i<numPoints;i++)
	{
		arr[i]=buffer;
		buffer+=(arr[i].GetLength()+1);
	}
}

template <class theType,class intType>
void CHArray<theType,intType>::SerializeToBuffer(char*& buffer)
{
	//Write number of points
	*((intType*)buffer)=numPoints; buffer+=sizeof(intType);

	//Write array elements
	theType* p=(theType*)buffer;
	for(intType i=0;i<numPoints;i++)
	{
		*p=arr[i];
		p++;
	}

	buffer=(char*)p;
}

//Full instantiation to <BString, int>
template<> inline void CHArray<BString,int>::SerializeToBuffer(char*& buffer)
{
	//Write number of points
	*((int*)buffer)=numPoints; buffer+=sizeof(int);

	//Write array elements
	for(int i=0;i<numPoints;i++)
	{
		char* curString=(char*)((const char*)arr[i]);
		while(*curString!=0)
		{
			*buffer=*curString;
			buffer++;curString++;
		}
		*buffer=0;buffer++;
	}
}

template <class theType,class intType>
intType CHArray<theType,intType>::RequiredSpace() const
{
	return sizeof(intType)+GetNumPoints()*sizeof(theType);
}

//Full instantiation to <BString, int>
template<> inline int CHArray<BString,int>::RequiredSpace() const
{
	int result=sizeof(int);
	for(int i=0;i<numPoints;i++)
	{
		result+=(arr[i].GetLength()+1);		//Also stores terminating zero char
	}
	return result;
}

template <class theType,class intType>
bool CHArray<theType,intType>::WriteBinary(const BString& fileName)
{
	FILE* fp;
	fp=fopen(fileName,"wb");
	if(fp==NULL) return false;

	fwrite(arr,1,(size_t)(numPoints*sizeof(theType)),fp);
	fclose(fp);

	return true;
}

template <class theType,class intType>
void CHArray<theType,intType>::RemoveByValue(const theType& val)
{
	//Removes points with the value specified
	//Does an in-place removal

	intType numIncluded=0;

	for(intType i=0;i<numPoints;i++)
	{
		if(val!=arr[i])	//the point is retained
		{
			if(numIncluded!=i)
			{
				arr[numIncluded]=arr[i];
			}
			numIncluded++;
		}
	}

	numPoints=numIncluded;
}

template <class theType,class intType>
void CHArray<theType,intType>::RemoveLastPoint()
{
	if(numPoints!=0) numPoints--;
}

template <class theType,class intType>
bool CHArray<theType,intType>::ReadStrings(const BString& fileName)
{
	CHArray<char,int64> text;
	if(!text.ReadBinary(fileName)) return false;
	int64 textLength=text.GetNumPoints();
	if(textLength==0) {ResizeArray(0,true);return true;};

	///////Remove all CRs
	int64 newLength=0;
	for(int64 i=0;i<textLength;i++)
	{
		if(text.arr[i]!=13)
		{
			if(i!=newLength) text.arr[newLength]=text.arr[i];
			newLength++;
		}
	}
	text.SetNumPoints(newLength);
	textLength=newLength;
	if(textLength==0) {ResizeArray(0,true);return true;};
	
	//No LF at the end
	if(text.arr[textLength-1]==10)
	{
		textLength--;
		text.SetNumPoints(textLength);
		if(textLength==0) {ResizeArray(0,true);return true;};
	}

	///////Count the number of lines
	intType numLines=1;
	for(int64 i=0;i<textLength;i++)
	{
		if(text.arr[i]==10) numLines++;
	}
	ResizeArray(numLines);

	//Maximum length of the element
	intType maxLength=0;
	intType curMaxLength=0;
	for(int64 i=0;i<=textLength;i++)
	{
		if(i<textLength && text.arr[i]!=10)	curMaxLength++;
		else
		{
			if(curMaxLength>maxLength) {maxLength=curMaxLength;}
			curMaxLength=0;
		}
	}
	if(curMaxLength>maxLength) maxLength=curMaxLength;
	
	char* curString=new char[maxLength+1];
	int64 curStringPos=0;

	for(int64 i=0;i<=textLength;i++)
	{
		if(i<textLength && text.arr[i]!=10)	curString[curStringPos++]=text.arr[i];
		else
		{
                        curString[curStringPos]=0;
			AddPoint(curString);
			curStringPos=0;
		}
	}	

	delete[] curString;
	return true;
}

template <class theType,class intType>
bool CHArray<theType,intType>::WriteStrings(const BString& fileName)
{
	//double pass - first count the number of symbols, then write to a char array
	int64 numSymbols=0;
	for(intType i=0;i<numPoints;i++)
	{
		numSymbols+=arr[i].GetLength();
	}
	if(numPoints>0) numSymbols+=(numPoints-1);	//for "\n" after every line, except the last one

	CHArray<char,int64> charArr(numSymbols,true);
	int64 curPos=0;
	for(intType i=0;i<numPoints;i++)
	{
		intType curLength=arr[i].GetLength();

		for(int j=0;j<curLength;j++)	{charArr.arr[curPos++]=arr[i][j];}
		if(i<(numPoints-1)) charArr.arr[curPos++]='\n';
	}

	return charArr.WriteBinary(fileName);
}

template <class theType,class intType>
void CHArray<theType,intType>::Serialize(BArchive& archive)
{
	if(archive.IsStoring())
	{
		archive<<numPoints;
	}
	else
	{
		intType newSize;
		archive>>newSize;
		ResizeArray(newSize,true);
	}
	archive.HandleArray(arr,numPoints);
}

//Find an element in an unsorted array
//That matches val
template <class theType,class intType>
intType CHArray<theType,intType>::PositionOf(const theType& val) const
{
	intType result=-1;
	for(intType i=0;i<numPoints;i++)
	{
		if(arr[i]==val) {result=i;break;}
	}
	return result;
}

//Find the position of an element in an unsorted array that is closest to val
template <class theType, class intType>
intType CHArray<theType, intType>::PositionOfClosest(const theType& val) const
{
	if (IsEmpty()) return -1;

	intType result = 0;
	theType curMinDiff = abs(val - arr[0]);

	for (intType i = 1; i < numPoints; i++)
	{
		theType diff = abs(val - arr[i]);
		if (diff < curMinDiff) { curMinDiff = diff; result = i; }
	}

	return result;
}

//Find position of a sequence of elements
//Returns -1 if not found
template <class theType,class intType>
intType CHArray<theType,intType>::FindSequence(const CHArray<theType,intType>& sequence, intType startPos)
{
	if(sequence.numPoints==0) return -1;
	if(startPos<0) return -1;

	intType maxNumPoints=numPoints-sequence.numPoints+1;
	for(intType i=startPos;i<maxNumPoints;i++)
	{
		intType j;
		for(j=0;j<sequence.numPoints && arr[i+j]==sequence.arr[j];j++){};
		if(j==sequence.numPoints) return i;
	}

	return -1;
}

template <class theType,class intType>
intType CHArray<theType,intType>::FindBinEquispaced(theType val) const
{
	//The array has to have at least 2 points and be increasing

	if(val<arr[0] || val>arr[numPoints-1]) return -1;	//val is out of bounds

	theType step=arr[1]-arr[0];

	theType result=(val-arr[0])/step;
	
	return (intType)result;
}

template <class theType,class intType>
void CHArray<theType,intType>::Sort(bool fDescending)
{
	if(fDescending) std::sort( begin(), end(), std::greater<theType>() );	//std sort descending

	else std::sort( begin(), end() );	//ascending std sort
}

template <class theType,class intType>	//intArray is for storing result only
void CHArray<theType,intType>::SortPermutation(CHArray<intType,intType>& perm, bool fDescending, bool fStableSort) const
{
	perm.ResizeIfSmaller(numPoints,true);
	perm.SetValToPointNum();

	SortIndices(perm,fDescending,fStableSort);
}

//Sorts provided indices according to the values in the array at those indices
template <class theType,class intType>
void CHArray<theType,intType>::SortIndices(CHArray<intType,intType>& indices, bool fDescending, bool fStableSort) const
{
	if(fStableSort)
	{
		if(fDescending) std::stable_sort( indices.begin(), indices.end(),
										[this](const intType& a, const intType& b)->bool{return arr[a] > arr[b];});

		else std::stable_sort( indices.begin(), indices.end(),
							[this](const intType& a, const intType& b)->bool{return arr[a] < arr[b];});
	}
	else
	{
		if(fDescending) std::sort( indices.begin(), indices.end(),
								[this](const intType& a, const intType& b)->bool{return arr[a] > arr[b];});

		else std::sort(	indices.begin(), indices.end(),
						[this](const intType& a, const intType& b)->bool{return arr[a] < arr[b];});
	}
}

template <class theType,class intType>	//Permutes according to the provided int array
void CHArray<theType,intType>::Permute(const CHArray<intType,intType>& permutation)
{
	if(permutation.GetNumPoints()!=numPoints) return;

	CHArray<theType,intType> interm(*this);

	for(intType i=0;i<numPoints;i++) arr[i]=interm.arr[permutation.arr[i]];
}

template <class theType,class intType>	//Calculates inverse permutation, makes sense for <int> arrays
void CHArray<theType,intType>::InvertPermutation()
{
	CHArray<theType,intType> interm(*this);

	for(intType i=0;i<numPoints;i++) arr[interm.arr[i]]=i;
}

template <class theType,class intType>
bool CHArray<theType,intType>::IsIncreasing() const
{
	if(numPoints<2) return true;

	if(arr[0]<=arr[1]) return true;
	else return false;
}

template <class theType,class intType>
bool CHArray<theType,intType>::IsDecreasing() const
{
	return !IsIncreasing(); 
}

template <class theType,class intType>
bool CHArray<theType,intType>::IsDataSigned() const
{
	theType data=0;
	data--;
	if(data<0) return true;
	else return false;
}

template <class theType,class intType>
CHArray<theType, intType>& CHArray<theType, intType>::SetValToPointNum()
{
	for (intType i = 0; i<numPoints; i++) arr[i] = (theType)i;

	return *this;
}

template <class theType,class intType>
intType CHArray<theType,intType>::FindBin(theType val) const
{
	//The array has to have at least two points and be increasing

	if(val < arr[0] || val > arr[numPoints-1]) return -1;	//val is out of bounds

	for(intType i=0; i<(numPoints-1); i++)
	{
		if(arr[i] <= val && arr[i+1] > val) return i;
	}
	return -1;
}

template <class theType,class intType>
theType CHArray<theType,intType>::Mean() const
{
	if (IsEmpty()) return 0;

	return Sum()/(theType)numPoints;
}

template <class theType,class intType>
theType CHArray<theType,intType>::StandardDeviation() const
{
	return sqrt(Variance());
}

template <class theType,class intType>
theType CHArray<theType,intType>::Variance() const
{
	theType mean=Mean();
	theType sumSquared=0;
	theType curVal;
	for(intType i=0; i<numPoints; i++)
	{
		curVal = arr[i]-mean;
		sumSquared += curVal*curVal;
	}

	return sumSquared/(theType)numPoints;
}

//Backwards normalized averaging - for example for exponential averaging
template <class theType,class intType>
CHArray<theType,intType>& CHArray<theType,intType>::BackNormAveraging(const CHArray<theType,intType>& weights)
{
	//Each point of *this is set to weighted average of preceding (weights.numPoints) elements
	//the weights are normalized to 1 for every point (with edge effects taken into account)
	CHArray<theType,intType> result(numPoints,true);

	intType numWeights=weights.numPoints;

	for(intType i=0;i<numPoints;i++)
	{
		theType sum1=0;		//weighted sum
		theType sum2=0;		//sum of weights

		for(intType j=0;(j<numWeights) && (j<=i);j++)
		{
			sum1+=arr[i-j]*weights.arr[j];
			sum2+=weights.arr[j];
		}

		result.arr[i]=sum1/sum2;
	}

	(*this)=result;
	return *this;
}

template <class theType,class intType>
CHArray<theType,intType>& CHArray<theType,intType>::AdjacentAveraging(intType num)
{
	if(num%2==0) num--;					////Only odd numbers of averaging points
	if(num<=0) return *this;

	CHArray<theType,intType> result(size);

	for(intType i=0; i<numPoints; i++)
	{
		intType beginNum;
		intType endNum;

		beginNum=i-(num-1)/2;
		endNum=i+(num-1)/2;

		if((beginNum<0)&&(endNum>(numPoints-1)))
		{
			if((i)<((numPoints-1)-i))
			{
				beginNum=0;
				endNum=2*i;
			}

			else
			{
				endNum=(numPoints-1);
				beginNum=(numPoints-1)-2*((numPoints-1)-i);
			}
		}

		if((beginNum<0)&&(endNum<=(numPoints-1)))
		{
			beginNum=0;
			endNum=2*i;
		}

		if((beginNum>=0)&&(endNum>=(numPoints-1)))
		{
			endNum=(numPoints-1);
			beginNum=(numPoints-1)-2*((numPoints-1)-i);
		}
		
		theType theNumPoints=(theType)(endNum-beginNum)+(theType)1.0;

		theType sum=0;
		for (intType j = beginNum; j <= endNum; j++) sum += arr[j];
		
		result.AddPoint(sum/theNumPoints);
	}

	(*this)=result;

	return *this;
}

template <class theType,class intType>
inline void CHArray<theType,intType>::AddPoint(const theType& point)
{
	if(numPoints >= size) return;
	
	else
	{
		arr[numPoints]=point;
		numPoints++;
		return;
	}
}

template <class theType,class intType>
void CHArray<theType,intType>::AddAndExtend(const theType& point, intType factor)
{
	//Adds a point, extending the array size by "factor" if necessary
	if(IsFull())
	{
		if(size==0) ResizeArrayKeepPoints(factor);
		else ResizeArrayKeepPoints(size*factor);
	}

	AddPoint(point);
}

template <class theType,class intType>
void CHArray<theType,intType>::AddFromArray(const CHArray<theType,intType>& source)
{
	int newSize = Count() + source.Count();
	if (newSize > Size()) ResizeArrayKeepPoints(newSize);

	for (int i = 0; i < source.Count(); i++) AddPoint(source.arr[i]);
}

//Add and extend from string
template <class theType,class intType>
void CHArray<theType,intType>::AddExFromString(const BString& source)
{
	intType length=(intType)source.GetLength();
	for(intType i=0;i<length;i++) AddAndExtend(source[i]);
}

template <class theType,class intType>
intType CHArray<theType,intType>::GetNextMaxPos(intType curPos) const
{
	if(numPoints<3) return -1;
	if(curPos<1) curPos=1;
	if(curPos>(numPoints-2)) curPos=numPoints-2;

	intType result=curPos;

	if((arr[curPos]>=arr[curPos-1])	//if already in maximum, move one point to the right
			&&(arr[curPos]>=arr[curPos+1]))
	{
		curPos++;
		if(curPos>(numPoints-1)) curPos=numPoints-1;
	}

	for(intType i=curPos; i<(numPoints-1); i++)
	{
		if((arr[i] >= arr[i-1])	&& (arr[i] >= arr[i+1])) {result=i;break;}
	}

	return result;
}

template <class theType,class intType>
intType CHArray<theType,intType>::GetNextMinPos(intType curPos) const
{
	if(numPoints<3) return -1;
	if(curPos<1) curPos=1;
	if(curPos>(numPoints-2)) curPos=numPoints-2;

	intType result=curPos;

	if((arr[curPos]<=arr[curPos-1])		//if already in minimum, move one point to the right
			&&(arr[curPos]<=arr[curPos+1]))
	{
		curPos++;
		if(curPos>(numPoints-1)) curPos=numPoints-1;
	}

	for(intType i=curPos;i<(numPoints-1);i++)
	{
		if((arr[i]<=arr[i-1]) && (arr[i]<=arr[i+1])) {result=i;break;}
	}

	return result;
}

template <class theType,class intType>
intType CHArray<theType,intType>::PositionOfMin() const
{
	if(!fDataPresent()) return -1;
	intType result=0;

	for (intType i = 0; i<numPoints; i++)
	{
		if (arr[result] > arr[i]) result = i;
	}

	return result;
}

template <class theType,class intType>
intType CHArray<theType,intType>::PositionOfMax() const
{
	if(!fDataPresent()) return -1;
	intType result=0;

	for(intType i=0;i<numPoints;i++)
	{
		if(arr[result] < arr[i]) result=i;
	}

	return result;
}

template <class theType,class intType>
theType CHArray<theType,intType>::Min() const
{
	if(!fDataPresent()) return 0;
	intType minPos=PositionOfMin();
	return arr[minPos];
}

template <class theType,class intType>
theType CHArray<theType,intType>::Max() const
{
	if(!fDataPresent()) return 0;
	intType maxPos=PositionOfMax();
	return arr[maxPos];
}

template <class theType,class intType>
void CHArray<theType,intType>::EraseArray()
{
	numPoints=0;
}

template <class theType,class intType>
void CHArray<theType,intType>::RemoveAllPoinstAfter(intType num)
{
	if((num>(numPoints-1))||(num<0)) return;

	else
	{
		numPoints=num+1;
		return;
	}
}

template <class theType,class intType>
void CHArray<theType,intType>::ResizeArray(intType newSize, bool fSetMaxNumPoints)
{
	if(fVirtual) return;

	if(size!=newSize)
	{
		DeleteArray();
		arr=new theType[(size_t)newSize];
		size=newSize;
	}

	if(fSetMaxNumPoints) numPoints=size;
	else numPoints=0;
	return;
}

template <class theType,class intType>
void CHArray<theType,intType>::ResizeIfSmaller(intType newSize, bool fSetNumPoints)
{
	if(fVirtual) return;

	if(size<newSize)
	{
		DeleteArray();
		arr=new theType[newSize];
		size=newSize;
	}

	if(fSetNumPoints) numPoints=newSize;
	else numPoints=0;
	return;
}

template <class theType,class intType>
void CHArray<theType,intType>::ResizeArrayKeepPoints(intType newSize)
{
	if(fVirtual) return;

	intType newPoints;
	if(numPoints<newSize) newPoints=numPoints;
	else newPoints=newSize;

	theType* newArr=new theType[newSize];

	for (intType i = 0; i<newPoints; i++) newArr[i] = arr[i];

	DeleteArray();

	size=newSize;
	numPoints=newPoints;
	arr=newArr;
	
	return;
}

template <class theType,class intType>
CHArray<theType,intType>& CHArray<theType,intType>::operator=(const CHArray<theType,intType>& rhs)
{
	if(this==&rhs) return *this;		//Avoiding self-assignment

	if(size!=rhs.size) ResizeArray(rhs.size);

	if(!fVirtual) numPoints=rhs.numPoints;
	intType numToCopy=std::min(numPoints,rhs.numPoints);
	
	for(intType i=0;i<numToCopy;i++) arr[i]=rhs.arr[i];
	
	return *this;
}

template <class theType,class intType>
CHArray<theType,intType>& CHArray<theType,intType>::ExportPart(CHArray<theType,intType>& target, intType from, intType numToCopy) const
{
	if(this==&target) return target;

	if(from>(numPoints-1)) from=numPoints-1;
	if(numToCopy>(numPoints-from)) numToCopy=numPoints-from;
    
	if(target.size<numToCopy) target.ResizeArray(numToCopy,true);
	else target.SetNumPoints(numToCopy);

	intType endPos=from+numToCopy;
	intType curPos=0;
	for(intType i=from;i<endPos;i++)
	{
		target.arr[curPos++]=arr[i];
	}

	return target;
}

template <class theType,class intType>
CHArray<theType,intType>& CHArray<theType,intType>::operator=(theType val)          //Fills the array with val to numPoints
{
	for(intType i=0; i<numPoints; i++) arr[i]=val;
		
	return *this;
}

template <class theType,class intType>
CHArray<theType,intType>& CHArray<theType,intType>::Concatenate(const CHArray<theType,intType> &rhs)	//Resizes the array if needed
{
	intType newPoints = numPoints + rhs.numPoints;
	if(newPoints > size) ResizeArrayKeepPoints(newPoints);

	for (auto& val : rhs) AddPoint(val);

	return *this;
}


template <class theType,class intType>
CHArray<theType,intType> CHArray<theType,intType>::BinaryOperationTwoArrays
		(const CHArray<theType,intType>& rhs, theType(CHArray<theType,intType>::*fPointer)(theType,theType))
{
	CHArray<theType,intType> retVal(*this);
	if(numPoints!=rhs.numPoints) return retVal;

	else
	{
		for (intType i = 0; i<numPoints; i++) retVal.arr[i] = (*this.*fPointer)(arr[i], rhs[i]);

		return retVal;
	}
}

template <class theType,class intType>
CHArray<theType,intType> CHArray<theType,intType>::BinaryOperationtheType
		(theType val, theType(CHArray<theType,intType>::*fPointer)(theType,theType))
{
	CHArray<theType,intType> retVal(*this);

	for (intType i = 0; i<numPoints; i++) retVal.arr[i] = ((*this.*fPointer)(arr[i], val));

	return retVal;
}

template <class theType,class intType>
CHArray<theType,intType>& CHArray<theType,intType>::AssignmentTwoArrays
		(const CHArray<theType,intType>& rhs, theType(CHArray<theType,intType>::*fPointer)(theType,theType))
{
	if(numPoints!=rhs.numPoints) return *this;

	else
	{
		for (intType i = 0; i<numPoints; i++) arr[i] = (*this.*fPointer)(arr[i], rhs[i]);

		return *this;
	}
}

template <class theType,class intType>
CHArray<theType,intType>& CHArray<theType,intType>::AssignmenttheType
		(theType val, theType(CHArray<theType,intType>::*fPointer)(theType,theType))
{
	for (intType i = 0; i<numPoints; i++) arr[i] = (*this.*fPointer)(arr[i], val);

	return *this;
}

template <class theType,class intType>
bool CHArray<theType,intType>::Read(BString name)
{
	CHArray<char,int64> text;
	if(!text.ReadBinary(name)) return false;
	int64 textLength=text.GetNumPoints();
	
	intType newNum=0;			//Counting the number of lines in the file
	if(text.arr[0]>32) newNum++;
	for(int64 i=0;i<(textLength-1);i++)
	{
		if((text.arr[i]<=32)&&(text.arr[i+1]>32)) newNum++;
	}

	ResizeArray(newNum);

	char buffer[1000];
	int64 numBuffered=0;
	int64 curPos=0;

	while(curPos<textLength)	
	{
		while(curPos<textLength && text.arr[curPos]<=32) curPos++;	//remove leading blanks				
		
		while(curPos<textLength && text.arr[curPos]>32)				//read meaningful characters
		{
			buffer[numBuffered++]=text.arr[curPos++];
		}
                buffer[numBuffered]=0;

		if(numBuffered>0) AddPoint((theType)atof(buffer));
	
		numBuffered=0;
	}
	
	return true;
}

template <class theType,class intType>
bool CHArray<theType,intType>::Write(BString name, BString form) const
{
	if(numPoints==0) return false;

	//double pass - first count the number of symbols, then write to a char array
	int64 numSymbols=0;
	BString buffer;
	for(intType i=0;i<numPoints;i++)
	{
		buffer.Format(form,arr[i]);
		numSymbols+=buffer.GetLength();
	}
	numSymbols+=(numPoints-1);	//for "\n" after every line, except the last one

	CHArray<char,int64> charArr(numSymbols,true);
	int64 curPos=0;
	for(intType i=0;i<numPoints;i++)
	{
		buffer.Format(form,arr[i]);
		intType curLength=buffer.GetLength();

		for(int j=0;j<curLength;j++)	{charArr.arr[curPos++]=buffer[j];}
		if(i<(numPoints-1)) charArr.arr[curPos++]='\n';
	}

	return charArr.WriteBinary(name);
}

template <class theType,class intType>
theType CHArray<theType,intType>::Sum() const
{
	theType result=0;
	for (intType i = 0; i<numPoints; i++) result += arr[i];
	return result;
}

template <class theType,class intType>
theType CHArray<theType,intType>::WeightedSum(const CHArray<theType,intType>& weights) const
{
	theType result=0;
	for (intType i = 0; i<numPoints; i++) result += arr[i] * weights.arr[i];
	return result;
}

template <class theType,class intType>
theType CHArray<theType,intType>::NormalizeSumTo1()
{
	theType sum = Sum();
	if(sum==0) sum=(theType)1;

	(*this) *= 1 / sum;

	return sum;
}

template <class theType,class intType>
theType CHArray<theType,intType>::NormalizeNorm()
{
	theType sqrtSumOfSquares=sqrt(SumOfSquares());
	if(sqrtSumOfSquares==0) sqrtSumOfSquares=1;
	
	(*this) *= 1 / sqrtSumOfSquares;

	return sqrtSumOfSquares;
}

template <class theType,class intType>
theType CHArray<theType,intType>::SumOfSquares() const
{
	theType result=0;
	for (intType i = 0; i < numPoints; i++) result += arr[i] * arr[i];
	return result;
}

template <class theType,class intType>
theType CHArray<theType,intType>::WeightedSumOfSquares(const CHArray<theType,intType>& weights) const
{
	theType result=0;
	for (intType i = 0; i < numPoints; i++) result += arr[i] * arr[i] * weights.arr[i];
	return result;
}

template <class theType,class intType>
theType CHArray<theType,intType>::WeightedSumOfSquaredDevs(const CHArray<theType,intType>& rhs, const CHArray<theType,intType>& weights) const
{
	theType result=0;
	for(intType i=0; i<numPoints; i++)
	{
		theType diff=arr[i]-rhs.arr[i];
		result+=diff*diff*weights.arr[i];
	}
	return result;
}

template <class theType,class intType>
theType CHArray<theType,intType>::DirectProduct(const CHArray<theType,intType> &rhs, theType alpha) const  //computes (r1*r2)^alpha * cos(theta)
{
	theType result=0;

	if(numPoints!=rhs.numPoints) return result;

	for (intType i = 0; i<numPoints; i++) result += arr[i] * rhs.arr[i];
	
	if(alpha==1) return result;					//if simple direct product

	theType mag1=DirectProduct(*this,1);		//Squares of magnitudes of the two vectors
	theType mag2=rhs.DirectProduct(rhs,1);

	if(alpha==0) return result/sqrt(mag1*mag2);	//If cosine between two vectors is requested

	return result/((theType)pow((double)mag1*mag2,(double)(1.-alpha)/2.));	//If any other alpha
}

template <class theType,class intType>
theType CHArray<theType,intType>::EuclideanDistance(const CHArray<theType,intType>& rhs) const
{
	if(numPoints!=rhs.numPoints) return 0;
	theType result=0;

	for (intType i = 0; i<numPoints; i++) result += (arr[i] - rhs.arr[i])*(arr[i] - rhs.arr[i]);

	return sqrt(result);
}

//Sum of squared deviations from another array
template <class theType, class intType>
theType CHArray<theType, intType>::SumOfSquaredDevs(const CHArray<theType, intType>& other) const
{
	if (numPoints != other.numPoints) return 0;

	theType result = 0;
	theType diff;
	for (int i = 0; i < Count(); i++)
	{
		diff = arr[i] - other.arr[i];
		result += diff * diff;
	}

	return result;
}

//Mean squared deviation from another array
template <class theType, class intType>
theType CHArray<theType, intType>::MeanSquaredDev(const CHArray<theType, intType>& other) const
{
	if (numPoints != other.numPoints) return 0;
	if (IsEmpty()) return 0;

	return SumOfSquaredDevs(other) / numPoints;
}


//Root mean square deviation from another array
template <class theType,class intType>
theType CHArray<theType,intType>::RootMeanSquareDev(const CHArray<theType,intType>& other) const
{
	if( numPoints != other.numPoints ) return 0;
	if (IsEmpty()) return 0;

	return sqrt( MeanSquaredDev(other) );
}

template <class theType,class intType>
CHArray<theType,intType>& CHArray<theType,intType>::LimitMinValue(theType limit)
{
	for (intType i = 0; i<numPoints; i++) if (arr[i]<limit) arr[i] = limit;

	return *this;
}

template <class theType,class intType>
CHArray<theType,intType>& CHArray<theType,intType>::LimitMaxValue(theType limit)
{
	for (intType i = 0; i<numPoints; i++) if (arr[i]>limit) arr[i] = limit;

	return *this;
}

template <class theType,class intType>
double CHArray<theType,intType>::Entropy() const		//double so that the entropy of int arrays can be computed
{
	if(!fDataPresent()) return 0;

	double result=0;

	double invSum=0;
	for (intType i = 0; i<numPoints; i++) invSum += (double)arr[i];

	if(invSum==0) return 0;
	invSum = 1.0/invSum;

	for(intType i=0;i<numPoints;i++)
	{
		double curTerm=((double)arr[i])*invSum;
		if(curTerm==0) continue;
		else result+=-curTerm*log(curTerm);
	}

	return result;
}

template <class theType,class intType>
double CHArray<theType,intType>::EntropicNumberOfStates() const
{
	return exp(Entropy());
}

template <class theType,class intType>
void CHArray<theType,intType>::IntegralForm()
{
	theType sum=0;
	for(intType i=0;i<numPoints;i++)
	{
		sum+=arr[i];
		arr[i]=sum;
	}
}
