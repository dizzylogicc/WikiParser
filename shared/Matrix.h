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

#include "mkl.h"

#include "Array.h"
#include "Timer.h"
#include "ArrArr.h"
#include "CAIStrings.h"
#include "Savable.h"
#include <list>

#define MKL_INT int
#undef max

template <class theType, class intType=int> class CMatrix : public Savable
{
public:
	CMatrix(intType theCols=0, intType theRows=0);
	explicit CMatrix(const BString& fileName);						//Will call Load() 
	explicit CMatrix(const CMatrix<theType,intType>& theMat);		//Will copy everything from another matrix

	//Will create a virtual matrix in an existing one that only includes a certain number of columns
	CMatrix(const CMatrix<theType,intType>& otherMat, intType startCol, intType numColsToInclude);
	~CMatrix(void){};
	
//All data - nothing needs to be removed in the destructor
public:
	intType cols;
	intType rows;
	intType numRowsAdded;							//for adding rows to an empty matrix
	CHArray<theType,intType> theArray;				//the contiguous array that stores the matrix elements
	CArrArr<theType,intType> colArrays;				//virtual arrays corresponding to columns

////Container behavior
public:
	CHArray<theType, intType>* begin(){ return colArrays.begin(); }
	CHArray<theType, intType>* end(){ return colArrays.end(); }

//Managing virtual arrays
private:
	void SetColArrays();	//Sets virtual arrays, to access columns as arrays

//Import and serialization
public:
	template<class rhsType> void ImportFrom(const CMatrix<rhsType,intType>& rhs);
	void Serialize(BArchive& ar)
	{
		if(ar.IsStoring()) { ar<<cols<<rows; theArray.Serialize(ar); }
		else { ar>>cols>>rows; theArray.Serialize(ar); SetColArrays(); }
	};


public:
//Size and resizing
	intType GetSize(){return cols*rows;};
	void ResizeMatrix(intType newCols, intType newRows);
	void ResizeMatrixKeepPoints(intType newCols, intType newRows);
	void ResizeToZero() {ResizeMatrix(0,0);};				//Frees the memory associated with the matrix

//Column, row and diagonal access
	theType* GetPointerToColumn(intType colNum);
	void GetColumn(intType colNum, CHArray<theType,intType>& result) const;
	void GetRow(intType rowNum, CHArray<theType,intType>& result) const;
	void SetRow(intType rowNum, const CHArray<theType,intType>& source);
	void SetColumn(intType colNum, const CHArray<theType,intType>& source);
	void SetDiagonal(const CHArray<theType,intType>& source);
	void SetDiagonal(const theType& val);
	void GetDiagonal(CHArray<theType,intType>& result) const;
	void ExportColsToArrayOfArrays(CHArray<CHArray<theType,intType>,intType>& result);
	void ExportRowsToArrayOfArrays(CHArray<CHArray<theType,intType>,intType>& result);
	void ImportRowsFromArrayOfArrays(CHArray<CHArray<theType,intType>,intType>& source);
	void ImportColsFromArrayOfArrays(CHArray<CHArray<theType,intType>,intType>& source);

//Concatenation
	void Concatenate(const CMatrix<theType,intType>& rhs, bool fConcatenateCols);
	void ConcatenateCols(const CMatrix<theType,intType>& rhs) {Concatenate(rhs,true);};
	void ConcatenateRows(const CMatrix<theType,intType>& rhs) {Concatenate(rhs,false);};

//Copying
	//Copies the overlapping area from rhs - offsets position *this over rhs
	void CopyFromMatrix(const CMatrix<theType,intType>& rhs, intType colOffset=0, intType rowOffset=0);

	//Copy single column or row
	void CopyRowFrom(CMatrix<theType,intType>& rhs, intType rowTo, intType rowFrom);			//Will not resize, will truncate if needed
	void CopyColumnFrom(CMatrix<theType,intType>& rhs, intType colTo, intType colFrom);			//Will not resize, will truncate if needed
	void CopyRowAsColumnFrom(CMatrix<theType,intType>& rhs, intType rowFrom, intType colTo);	//Same
	void CopyColumAsRowFrom(CMatrix<theType,intType>& rhs, intType colFrom, intType rowTo);		//Same

	//Copy all data with transposition, resize *this if necessary
	void CopyTransposeFrom(CMatrix<theType,intType>& rhs);			//Copies with transposition from matrix, resizes if needed
	
//Transposition
	void Transpose();

//Modifying structure
	void PermuteColumns(const CHArray<intType,intType>& perm);
	void SwitchColumns(intType colNum1, intType colNum2);
	void AddRow(CMatrix<theType,intType>& rhs, intType rowFrom);	//Only if "free" rows are already allocated! Uses numRowsAdded.
																	//Will not change the actual number of rows in the matrix
	void AddRow(CHArray<theType,intType>& rhs);						//Same

//Modifying columns and rows
	CMatrix<theType,intType>& MultiplyColumnsBy(const CHArray<theType,intType>& rhs);
	CMatrix<theType,intType>& MultiplyRowsBy(const CHArray<theType,intType>& rhs);
	theType NormalizeColumn(intType colNum);				//Normalizes euclidean norm
	void NormalizeAllColumns();								//Normalizes norms for all columns
	void NormalizeAllRows();								//Normalizes norms for all rows
	
//Reading and writing to files
	bool ReadStrings(const BString& fileName);
	bool WriteStrings(const BString& fileName, bool fOnlyAddedRows = false);
	bool Read(const BString& fileName);
	bool Write(const BString& fileName, BString format="%.5e");
		
//Interpolation	
	theType Interpolate(double colIndex, double rowIndex) const;	//Bilinear interpolation
																	//with non-integer indices

//Creating special matrices
	bool SetToIdentityMatrix();
	//Sets the matrix to the outer product of two vectors
	void OuterProduct(const CHArray<theType,intType>& colVector, const CHArray<theType,intType> rowVector);


	theType DirProdOfColumns(intType col1, intType col2, theType alpha=1);		//Direct product of two columns

//Used when rows are added one by one
	void SetNumRowsAdded(intType theNum){numRowsAdded=theNum;};

//Orthogonalizations
	void OrthonormalizeWithSVD(bool fCol=true);		//Orthonormalizes columns (fCol=true) or rows (false) by finding the SVD basis
	CMatrix<theType,intType>& GramSchmidt();		//Stabilized Gram-Schimdt orthonormalization of columns
	CMatrix<theType,intType>& GramSchmidtComplex();	////Interleaved complex storage: first column contains real part
													//the following column contains im part

	//Orthogonalizes vec with respect to each column of (*this)
	//Without using BLAS
	void OrthogonalizeVecNoBlas(CHArray<theType,intType>& vec) const;

	//Orthogonalizes or orthonormalizes a column of a matrix
	//With regard to all columns preceeding it
	void IncompleteOrthogonalize(intType colNum, CHArray<theType,intType>& workArr);	//Used by the GramSchmidt procedure
	void IncompleteOrthonormalize(intType colNum, CHArray<theType,intType>& workArr);	//Used by the GramSchmidt procedure

	//Orthogonalize vector with respect to the columns of the matrix
	//vec should have the same number of rows
	//workArr will be resized to numCols in the matrix
	//Assumes that the columns of the matrix form an orthogonal basis
	bool OrthogonalizeVec(CHArray<theType,intType>& vec, CHArray<theType,intType>& workArr) const;
	bool OrthonormalizeVec(CHArray<theType,intType>& vec, CHArray<theType,intType>& workArr) const; //Same, but also normalizes the vector

	//Orthogonalize the columns (rows) of a matrix with respect to the columns (rows) of this matrix
	//Assumes that the columns (rows) of this matrix form an orthogonal basis
	//Does not normalize the columns (rows) of mat and does not orthogonalize them with respect to one another
	//fCol determines whether the columns or rows are orthogonalized
	bool OrthogonalizeMatPartial(CMatrix<theType,intType>& mat, CMatrix<theType,intType>& workMat, bool fCol=true) const;
	bool OrthogonalizeMatPartial(CMatrix<theType,intType>& mat, bool fCol=true) const
					{CMatrix<theType,intType> workMat; return OrthogonalizeMatPartial(mat,workMat,fCol);};

	//Finds column (fCol=true) or row (fCol=false) singular vectors
	//Useful for finding the orthogonal basis for thin matrices with few columns or rows - very fast!
	//When the number of columns (rows) is less than the number of rows (columns)
	//Result stored in eVecs, in cols if fCol=true, and in rows in fCol=false
	//If requested, will export the intermediate basis that converts the current vectors into eigenvectors
	BString SVDVectors(CHArray<theType,intType>& eVals, CMatrix<theType,intType>& eVecs, bool fCol);
	BString SVDVectors(CHArray<theType,intType>& eVals, CMatrix<theType,intType>& eVecs, bool fCol, CMatrix<theType,intType>& intermBasis);

//Operators
	CMatrix<theType,intType>& operator=(const CMatrix<theType,intType>& rhs);
	CMatrix<theType,intType>& operator=(theType val){theArray=val;return *this;};
	CMatrix<theType,intType>& operator+=(CMatrix<theType,intType>& rhs){theArray+=rhs.theArray;return *this;};
	CMatrix<theType,intType>& operator-=(CMatrix<theType,intType>& rhs){theArray-=rhs.theArray;return *this;};
	CMatrix<theType,intType>& operator*=(const CHArray<theType,intType>& rhs){return MultiplyColumnsBy(rhs);};
	theType& ElementAt(intType theCol, intType theRow) const {return theArray[theCol*rows+theRow];};
	theType& operator()(intType theCol, intType theRow) const {return theArray[theCol*rows+theRow];};
	CHArray<theType,intType>& operator[](intType theCol) const {return colArrays[theCol];};

//Linear-algebraic and other complex operations
	//Matrix-vector multiply, result<-alpha* a.vec + beta*result
	CHArray<theType,intType>& MatVecMultiply(CHArray<double,intType>& vec, CHArray<double,intType>& result,
									bool fTransposeA=false, double alpha=1, double beta=0) const;
	CHArray<theType,intType>& MatVecMultiply(CHArray<float,intType>& vec, CHArray<float,intType>& result,
									bool fTransposeA=false, float alpha=1, float beta=0) const;

	//Matrix multiplication, C<-alpha*A.B+beta*C
	CMatrix<theType,intType>& MatMultiply(const CMatrix<double,intType>& B, CMatrix<double,intType>& C, 
		bool fTransposeA=false, bool fTransposeB=false,
			double alpha=1, double beta=0) const;		//double precision
	CMatrix<theType,intType>& MatMultiply(const CMatrix<float,intType>& B, CMatrix<float,intType>& C, 
		bool fTransposeA=false, bool fTransposeB=false,
			float alpha=1, float beta=0) const;		//float

	//Find full eigensystem of a symmetric matrix; divide-and-conquer
	//Overwrites the matrix with eigenvectors in columns
	//Eigenvalues are arranged lowest to largets
	BString EigenDAC(CHArray<double,intType>& evalues);								//double precision
	BString EigenDAC(CHArray<float,intType>& evalues);									//float - does not work for large mats

	//Complete eigendecomposition of a non-symmetric real matrix
	//Eigenvalues and eigenvectors are either real, or come in complex-conjugate pairs
	//In the complex case, the first eigenvector in the pair is the real part, and the second one is positive im part
	BString EigenNonsym(CHArray<double,intType>& eValsRe, CHArray<double,intType>& eValsIm, CMatrix<double,intType>& eVecs) const;
	BString EigenNonsym(CHArray<float,intType>& eValsRe, CHArray<float,intType>& eValsIm, CMatrix<float,intType>& eVecs) const;

	//Solve linear system of equations with symmetric positive definite A
	//Add a Tikhonov regularization constant alpha
	//Solves (A+alpha*I).x=b for x, where I is the identity matrix
	BString SolveSPDLinearSystem(CHArray<double,intType>& b, CHArray<double,intType>& x, double tykhonovAlpha=0);

	//Find n largest (or smallest) eig. vectors and values of a symmetric matrix; relatively robust representations (RRR)
	//Finds vectors and values to tolerance absTol (0 - min tolerance)
	//Overwrites the matrix with intermediate data
	//if fLargest==true, finds largest; if false, finds smallest
	//writes eigenvectors in columns of "vectors"
	bool EigenRRR(intType n, CHArray<double,intType>& values, CMatrix<double,intType>& vectors, double absTol=0, bool fLargest=true);	//double
	bool EigenRRR(intType n, CHArray<float,intType>& values, CMatrix<float,intType>& vectors, float absTol=0, bool fLargest=true);		//float

	/////Singular value decomposition
	//leftRight='L' or 'R' - left or right singular vectors
	//If 'L', overwrites min(cols,rows) columns with sing. vectors, if 'R' - rows
	BString SVD(char leftRight, CHArray<double,intType>& values);		//double
	BString SVD(char leftRight, CHArray<float,intType>& values);		//float

	//Full SVD with both left and right singular vectors found
	//The contents of the matrix itself is destroyed
	//Only min(cols,vecs) vectors are computed. Left sing. vecs are stored as cols, right vecs - as rows
	BString FullSVD(CHArray<double,intType>& values, CMatrix<double,intType>& leftVecs, CMatrix<double,intType>& rightVecs);
	BString FullSVD(CHArray<float,intType>& values, CMatrix<float,intType>& leftVecs, CMatrix<float,intType>& rightVecs);

	//Partial SVD - computes a given number of largest values (and their vectors) by calling eigenRRR
	//Both column and row singular vectors are computed
	//values and vectors are written least to largest in vals and vecs
	bool PartialSVD(intType numVecs, CHArray<theType,intType>& vals, CMatrix<theType,intType>& colVecs, CMatrix<theType,intType>& rowVecs) const;

	//K-means clustering - Lloyd's algorithm
	//The coordinates of points should be in columns, there are N columns (N points) grouped into K clusters
	//Result will contain K arrays, each contaning int point numbers that will go into that cluster.
	//Arrays are sorted so that clusters with larger numbers of points come first
	//Will use Euclidean distances
	bool KmeansClustering(intType k, CHArray<CHArray<intType,intType>,intType>& result);

	//Agglomerative clustering
	//The coordinates of points should be in columns
	//Merges groups until the entropic number of groups is below entropicLimit, N >= entropicLimit >= 1
	//Result will contain arrays, each contaning int point numbers that will go into that cluster.
	bool AgglomerativeClustering(theType entropicLimitNum, CHArray<CHArray<intType,intType>,intType>& result);

	//Clusters the points in the matrix with Principal Direction Divisive Partitioning
	//Until the size of the clusters is no more than maxNumInCluster
	//The coordinates of points should be in matrix columns
	//The columns are interchanged to group cluster members together
	//The columns are interchanged to group cluster members together
	//Result contains the clusters of original column indices
	//The order in result and in the permuted (*this) matrix are the same
	void PDDPclustering(intType maxNumInCluster, CAIStrings<intType,intType>& result);

private:
	//Helper function for PDDP clustering
	//Creates a copy of the matrix
	//Finds the projection of each column on the primary eigenvector
	//Sorts the columns according to the projection
	//Finds optimal division point
	//And returns the division point and corresponding reduction in squared devs in divPoint and reduction
	//Reorders the columns of the matrix according to the projection on the primary principal direction!
	//arrayToPermute is the (virtual) part of the permutation that the main function is building that corresponds to
	//the current (virtual) matrix
	void PDDPhelper(intType& divPoint, theType& reduction, CHArray<intType,intType>& arrayToPermute);

private:
	//Transpose helper function
	//Fills the matrix from an array - sizes must match exactly
	//Must be called after cols and rows are switched
	void HelperCopyTransposeFrom(CHArray<theType,intType>& arr, intType targetCols, intType targetRows);
};

template <class theType, class intType>
CMatrix<theType,intType>& CMatrix<theType,intType>::MultiplyColumnsBy(const CHArray<theType,intType>& rhs)
{
	for(intType colCounter=0;colCounter<cols;colCounter++)
	{
		(*this)[colCounter]*=rhs.arr[colCounter];
	}

	return *this;
}

template <class theType, class intType>
CMatrix<theType,intType>& CMatrix<theType,intType>::MultiplyRowsBy(const CHArray<theType,intType>& rhs)
{
	for(intType colCounter=0;colCounter<cols;colCounter++)
	{
		for(intType rowCounter=0;rowCounter<rows;rowCounter++)
		{
			theArray.arr[colCounter*rows+rowCounter]*=rhs.arr[rowCounter];
		}
	}

	return *this;
}

template <class theType, class intType>
void CMatrix<theType,intType>::OrthogonalizeVecNoBlas(CHArray<theType,intType>& vec) const
{
	const CMatrix<theType,intType>& thisMat=(*this);
	for(intType counter1=0;counter1<cols;counter1++)
	{
		vec.Orthogonalize(thisMat[counter1]);
	}
}

template <class theType, class intType>
bool CMatrix<theType,intType>::PartialSVD(intType numVecs, CHArray<theType,intType>& vals, CMatrix<theType,intType>& colVecs, CMatrix<theType,intType>& rowVecs) const
{
	//Find the smaller dimension of the matrix
	bool fCol=(rows<cols);

	CMatrix<theType,intType> selfProd;
	const CMatrix<theType,intType>& thisMat=(*this);

	if(fCol)
	{
		thisMat.MatMultiply(thisMat,selfProd,false,true);
		bool res=selfProd.EigenRRR(numVecs,vals,colVecs);
		if(!res) return false;
		thisMat.MatMultiply(colVecs,rowVecs,true,false);
		rowVecs.NormalizeAllColumns();
	}
	else
	{
		thisMat.MatMultiply(thisMat,selfProd,true,false);
		bool res=selfProd.EigenRRR(numVecs,vals,rowVecs);
		if(!res) return false;
		thisMat.MatMultiply(rowVecs,colVecs,false,false);
		colVecs.NormalizeAllColumns();
	}

	vals.Sqrt();
	return true;
}

template <class theType, class intType>
theType CMatrix<theType,intType>::Interpolate(double colIndex, double rowIndex) const
{
	//Bilinear interpolation with non-integer indices
	if(cols*rows == 0) return 0;

	intType top, bottom, left, right;	//top, bottom - row indices; left, right - col indices

	double colF=floor(colIndex);
	double rowF=floor(rowIndex);
	intType intColF=(intType)colF;
	intType intRowF=(intType)rowF;

	if(intColF < 0) {left=0; right=0;}
	if(intColF >= (cols-1)) {left=cols-1; right=cols-1;}
	if(intColF>=0 && intColF<(cols-1)) {left=intColF; right=left+1;}

	if(intRowF < 0) {top=0; bottom=0;}
	if(intRowF >= (rows-1)) {top=rows-1; bottom=rows-1;}
	if(intRowF>=0 && intRowF<(rows-1)) {top=intRowF; bottom=top+1;}

	double colResidue=colIndex-colF;
	double rowResidue=rowIndex-rowF;

	theType leftTop=ElementAt(left,top);
	theType leftBottom=ElementAt(left,bottom);
	theType rightTop=ElementAt(right,top);
	theType rightBottom=ElementAt(right,bottom);

	return
		leftTop +
		(rightTop - leftTop)*colResidue +
		(leftBottom - leftTop)*rowResidue +
		(leftTop + rightBottom - rightTop - leftBottom)*colResidue*rowResidue;
}

template <class theType, class intType>
void CMatrix<theType,intType>::OuterProduct(const CHArray<theType,intType>& colVector, const CHArray<theType,intType> rowVector)
{
	ResizeMatrix(rowVector.GetNumPoints(),colVector.GetNumPoints());

	for(intType colCounter=0;colCounter<cols;colCounter++)
	{
		for(intType rowCounter=0;rowCounter<rows;rowCounter++)
		{
			theArray.arr[colCounter*rows+rowCounter]=colVector.arr[rowCounter]*rowVector.arr[colCounter];
		}
	}
}

template <class theType, class intType>
void CMatrix<theType,intType>::SetDiagonal(const CHArray<theType,intType>& source)
{
	intType minDim=std::min(cols,rows);
	if(minDim!=source.GetNumPoints()) return;

	for(intType counter1=0;counter1<minDim;counter1++)
	{
		theArray.arr[counter1*rows+counter1]=source.arr[counter1];
	}
}

template <class theType, class intType>
void CMatrix<theType,intType>::SetDiagonal(const theType& val)
{
	for(intType i=0; i<cols && i<rows; i++) (*this)(i,i)=val;
}

template <class theType, class intType>
void CMatrix<theType,intType>::GetDiagonal(CHArray<theType,intType>& result) const
{
	intType minDim=std::min(cols,rows);
	result.ResizeArray(minDim);

	for(intType counter1=0;counter1<minDim;counter1++)
	{
		result.arr[counter1]=theArray.arr[counter1*rows+counter1];
	}

	return;
}

template <class theType, class intType>
void CMatrix<theType,intType>::SwitchColumns(intType colNum1, intType colNum2)
{
	if(colNum1==colNum2) return;

	CMatrix<theType,intType>& thisMat=(*this);

	CHArray<theType,intType> temp;
	temp=thisMat[colNum1];
	thisMat[colNum1]=thisMat[colNum2];
	thisMat[colNum2]=temp;

	return;
}

template <class theType, class intType>
bool CMatrix<theType,intType>::SetToIdentityMatrix()
{
	if(cols!=rows) return false;	//Matrix is not square

	theArray=0;
	for(intType counter1=0;counter1<rows;counter1++)
	{
		theArray.arr[counter1+rows*counter1]=1;
	}

	return true;
}

template <class theType, class intType>
BString CMatrix<theType,intType>::EigenNonsym(CHArray<float,intType>& eValsRe, CHArray<float,intType>& eValsIm, CMatrix<float,intType>& eVecs) const
{
	if(cols!=rows) return "EigenNonsym: matrix must be square.";

	eValsRe.ResizeArray(cols,true);
	eValsIm.ResizeArray(cols,true);
	eVecs.ResizeMatrix(cols,cols);

	//Make a copy of the matrix, place it into eVecs
	eVecs=(*this);

	//Convert matrix to upper Hessenberg form, H
	intType n=cols;
	intType ilo=1;			//no balancing of the matrix
	intType ihi=n;			//no balancing
	CHArray<theType,intType> tau(n-1,true);		//Will contain information on Q - the orthonormal matrix that converts the original matrix to H
	intType lwork=n;
	CHArray<theType,intType> work(lwork,true);
	intType info;
	
	sgehrd(&n, &ilo, &ihi, eVecs.theArray.arr, &n, tau.arr, work.arr, &lwork, &info);
	if(info!=0) return "EigenNonsym: error in _gehrd.";

	//eVecs now contains the upper Hessenberg matrix H; values that are supposed to be 0 are not - they contain information about Q
	CMatrix<theType,intType> hess(eVecs);

	//Calculate orthogonal matrix Q
	sorghr(&n, &ilo, &ihi, eVecs.theArray.arr, &n, tau.arr, work.arr, &lwork, &info);
	if(info!=0) return "EigenNonsym: error in _orghr.";

	//eVecs now contains orthogonal matrix Q

	//Compute all the eigenvalues and Schur factorization of the upper Hessenberg matrix H
	char job='S';	//Schur form is required
	char compz='V';		//Compute Schur vectors of the original matrix, before it was transformed to upper Hessenberg form

	shseqr(&job, &compz, &n, &ilo, &ihi, hess.theArray.arr, &n, eValsRe.arr, eValsIm.arr, eVecs.theArray.arr, &n, work.arr, &lwork, &info);
	//info may be greater than 0 - it means that not all eigenvalues could be computed

	//eValsRe and eValsIm now contain the eigenvalues; hess contains the upper quasi-triangular matrix in the Schur basis
	//eVecs contains the Schur basis of the original matrix

	//Compute eigenvectors of the upper quasi-triangular matrix stored in hess
	char side='R'; //right eigenvectors
	char howmny='B';	//compute all and backtransform then by the Schur basis of the original matrix
	intType select;			//placeholder - not referenced
	intType m;				//output - will be set to n
	work.ResizeArray(3*n,true);

	strevc(&side, &howmny, &select, &n, hess.theArray.arr, &n, eVecs.theArray.arr, &n, eVecs.theArray.arr, &n,
			&n, &m, work.arr, &info);
	if(info!=0) return "EigenNonsym: error in _trevc.";

	//eVecs now contains the unnormalized eigenvectors of the original matrix
	//they need to be normalized
	for(intType counter1=0;counter1<n;counter1++)
	{
		if(eValsIm.arr[counter1]==0.0)	//real eigenvalue and vector
		{
			eVecs[counter1].NormalizeNorm();
		}
		else					//a pair of complex eigenvectors
		{
			theType norm=sqrt(eVecs[counter1].SumOfSquares()+eVecs[counter1+1].SumOfSquares());

			eVecs[counter1]/=norm;
			eVecs[counter1+1]/=norm;

			counter1++;			//Skip the next vector - it is part of the same pair
		}
	}
	
	return "EigenNonsym: normal termination.";
}

template <class theType, class intType>
BString CMatrix<theType,intType>::EigenNonsym(CHArray<double,intType>& eValsRe, CHArray<double,intType>& eValsIm, CMatrix<double,intType>& eVecs) const
{
	if(cols!=rows) return "EigenNonsym: matrix must be square.";

	eValsRe.ResizeArray(cols,true);
	eValsIm.ResizeArray(cols,true);
	eVecs.ResizeMatrix(cols,cols);

	//Make a copy of the matrix, place it into eVecs
	eVecs=(*this);

	//Convert matrix to upper Hessenberg form, H
	intType n=cols;
	intType ilo=1;			//no balancing of the matrix
	intType ihi=n;			//no balancing
	CHArray<theType,intType> tau(n-1,true);		//Will contain information on Q - the orthonormal matrix that converts the original matrix to H
	intType lwork=n;
	CHArray<theType,intType> work(lwork,true);
	intType info;
	
	dgehrd(&n, &ilo, &ihi, eVecs.theArray.arr, &n, tau.arr, work.arr, &lwork, &info);
	if(info!=0) return "EigenNonsym: error in _gehrd.";

	//eVecs now contains the upper Hessenberg matrix H; values that are supposed to be 0 are not - they contain information about Q
	CMatrix<theType,intType> hess(eVecs);

	//Calculate orthogonal matrix Q
	dorghr(&n, &ilo, &ihi, eVecs.theArray.arr, &n, tau.arr, work.arr, &lwork, &info);
	if(info!=0) return "EigenNonsym: error in _orghr.";

	//eVecs now contains orthogonal matrix Q

	//Compute all the eigenvalues and Schur factorization of the upper Hessenberg matrix H
	char job='S';	//Schur form is required
	char compz='V';		//Compute Schur vectors of the original matrix, before it was transformed to upper Hessenberg form

	dhseqr(&job, &compz, &n, &ilo, &ihi, hess.theArray.arr, &n, eValsRe.arr, eValsIm.arr, eVecs.theArray.arr, &n, work.arr, &lwork, &info);
	//info may be greater than 0 - it means that not all eigenvalues could be computed

	//eValsRe and eValsIm now contain the eigenvalues; hess contains the upper quasi-triangular matrix in the Schur basis
	//eVecs contains the Schur basis of the original matrix

	//Compute eigenvectors of the upper quasi-triangular matrix stored in hess
	char side='R'; //right eigenvectors
	char howmny='B';	//compute all and backtransform then by the Schur basis of the original matrix
	intType select;			//placeholder - not referenced
	intType m;				//output - will be set to n
	work.ResizeArray(3*n,true);

	dtrevc(&side, &howmny, &select, &n, hess.theArray.arr, &n, eVecs.theArray.arr, &n, eVecs.theArray.arr, &n,
			&n, &m, work.arr, &info);
	if(info!=0) return "EigenNonsym: error in _trevc.";

	//eVecs now contains the unnormalized eigenvectors of the original matrix
	//they need to be normalized
	for(intType counter1=0;counter1<n;counter1++)
	{
		if(eValsIm.arr[counter1]==0.0)	//real eigenvalue and vector
		{
			eVecs[counter1].NormalizeNorm();
		}
		else					//a pair of complex eigenvectors
		{
			theType norm=sqrt(eVecs[counter1].SumOfSquares()+eVecs[counter1+1].SumOfSquares());

			eVecs[counter1]/=norm;
			eVecs[counter1+1]/=norm;

			counter1++;			//Skip the next vector - it is part of the same pair
		}
	}
	
	return "EigenNonsym: normal termination.";
}

template <class theType, class intType>
void CMatrix<theType,intType>::SetColArrays()
{
	//Sets the virtual arrays
	colArrays.Resize(cols,0);

	for(intType colCounter=0;colCounter<cols;colCounter++)
	{
		colArrays.arr[colCounter].SetVirtual(GetPointerToColumn(colCounter),rows);
	}
}

template <class theType, class intType>
theType* CMatrix<theType,intType>::GetPointerToColumn(intType colNum)
{
	return theArray.arr+rows*colNum;
}

template <class theType, class intType>
template <class rhsType>
void CMatrix<theType,intType>::ImportFrom(const CMatrix<rhsType,intType>& rhs)
{
	ResizeMatrix(rhs.cols,rhs.rows);
	theArray.ImportFrom(rhs.theArray);
}

template <class theType, class intType>
theType CMatrix<theType,intType>::DirProdOfColumns(intType col1, intType col2, theType alpha)
{
	return colArrays[col1].DirectProduct(colArrays[col2], alpha);
}

//Stabilized Gram-Schmidt orthonormalization of columns
template <class theType, class intType>
CMatrix<theType,intType>& CMatrix<theType,intType>::GramSchmidt()
{
	CHArray<theType,intType> workArr;

	for(intType i=1;i<cols;i++) IncompleteOrthonormalize(i,workArr);
	return *this;
}

//Orthogonalize column colNum with respect to all columns preceding it
template <class theType, class intType>
void CMatrix<theType,intType>::IncompleteOrthogonalize(intType colNum, CHArray<theType,intType>& workArr)
{
	if(colNum==0) return;
		
	CMatrix<theType,intType> tempMat(*this,0,colNum);	//Virtual matrix
	tempMat.OrthogonalizeVec((*this)[colNum],workArr);
}

template <class theType, class intType>
void CMatrix<theType,intType>::IncompleteOrthonormalize(intType colNum, CHArray<theType,intType>& workArr)
{
	//Orthonormalize column number colNum with respect to all columns preceding it
	IncompleteOrthogonalize(colNum,workArr);
	(*this)[colNum].NormalizeNorm();
}

//Orthogonalize vector with respect to the columns of the matrix
//vec should have the same number of rows
//workArr will be resized to numCols in the matrix
//Assumes that the columns of this matrix form an orthogonal basis
template <class theType, class intType>
bool CMatrix<theType,intType>::OrthogonalizeVec(CHArray<theType,intType>& vec, CHArray<theType,intType>& workArr) const
{
	if(vec.Count()!=rows) return false;
	workArr.ResizeIfSmaller(cols,true);

	(*this).MatVecMultiply(vec,workArr,true);
	(*this).MatVecMultiply(workArr,vec,false,(theType)-1,(theType)1);
	return true;
}

//Same as OrthogonalizeVec, but also normalizes the vec at the end
template <class theType, class intType>
bool CMatrix<theType,intType>::OrthonormalizeVec(CHArray<theType,intType>& vec, CHArray<theType,intType>& workArr) const
{
	bool result=OrthogonalizeVec(vec,workArr);
	if(result) vec.NormalizeNorm();
	return result;
}

//Orthogonalize the columns (rows) of a matrix with respect to the columns (rows) of this matrix
//Assumes that the columns (rows) of this matrix form an orthogonal basis
//Does not normalize the columns (rows) of mat and does not orthogonalize them with respect to one another
//fCol determines whether the columns or rows are orthogonalized
template <class theType, class intType>
bool CMatrix<theType,intType>::OrthogonalizeMatPartial(CMatrix<theType,intType>& mat,
											CMatrix<theType,intType>& workMat, bool fCol/*=true*/) const
{
	if(fCol)
	{
		if(mat.rows!=rows) return false;
		(*this).MatMultiply(mat,workMat,true,false);
		(*this).MatMultiply(workMat,mat,false,false,(theType)-1,(theType)1);
	}
	else
	{
		if(mat.cols!=cols) return false;
		(*this).MatMultiply(mat,workMat,false,true);
		workMat.MatMultiply((*this),mat,true,false,(theType)-1,(theType)1);
	}

	return true;
}

//Orthonormalizes columns (fCol=true) or rows (false) by finding the SVD basis
template <class theType, class intType>
void CMatrix<theType,intType>::OrthonormalizeWithSVD(bool fCol)
{
	CHArray<theType,intType> evals;
	CMatrix<theType,intType> evecs;

	SVDVectors(evals,evecs,fCol);
	(*this)=evecs;
}

template <class theType, class intType>
CMatrix<theType,intType>& CMatrix<theType,intType>::GramSchmidtComplex()
{
	//Stabilized Gram-Schmidt orthonormalization of columns
	//Interleaved complex storage: first column contains real part, the following column contains im part
	
	for(intType counter1=0;counter1<cols;counter1+=2)
	{
		//Declare first pair of virtual arrays
		CHArray<theType,intType> vec1Real(theArray.arr+rows*counter1,rows,true);
		CHArray<theType,intType> vec1Im(theArray.arr+rows*(counter1+1),rows,true);
		CHArray<theType,intType> vec1Full(theArray.arr+rows*counter1,rows*2,true);	//Both real and complex, for normalization
		
		for(intType counter2=0;counter2<counter1;counter2+=2)
		{
			//Declare second virtual array
			CHArray<theType,intType> vec2Real(theArray.arr+rows*counter2,rows,true);
			CHArray<theType,intType> vec2Im(theArray.arr+rows*(counter2+1),rows,true);

			//Calculate direct product with the conjugate of vec2
			theType dirProductReal=vec2Real.DirectProduct(vec1Real)+vec2Im.DirectProduct(vec1Im);
			theType dirProductIm=vec2Real.DirectProduct(vec1Im)-vec2Im.DirectProduct(vec1Real);

			//Subtract projection
			vec1Real.MultiplyAdd(vec2Real,-dirProductReal);
			vec1Real.MultiplyAdd(vec2Im,dirProductIm);

			vec1Im.MultiplyAdd(vec2Real,-dirProductIm);
			vec1Im.MultiplyAdd(vec2Im,-dirProductReal);
		}
		vec1Full.NormalizeNorm();
	}

	return *this;
}

template <class theType, class intType>
BString CMatrix<theType,intType>::SolveSPDLinearSystem(CHArray<double,intType>& b, CHArray<double,intType>& x, double tykhonovAlpha)
{
	if(cols!=rows) return "SolveSPDLinearSystem: not square.";

	CHArray<double,intType> bCopy(b);

	CMatrix<double,intType> eigVectors(*this);
	CHArray<double,intType> invEigVals;

	if(tykhonovAlpha!=0)			//Add Tykhonov regularization
	{
		for(intType counter1=0;counter1<eigVectors.cols;counter1++)
		{
			eigVectors(counter1,counter1)+=tykhonovAlpha;
		}
	}

	eigVectors.EigenDAC(invEigVals);
	invEigVals^=-1;
	
	eigVectors.MatVecMultiply(bCopy,x,true);
	bCopy=x;
	bCopy*=invEigVals;
	eigVectors.MatVecMultiply(bCopy,x,false);

	return "SolveLinearSystem: OK.";
}

template <class theType, class intType>
void CMatrix<theType,intType>::ExportColsToArrayOfArrays(CHArray<CHArray<theType,intType>,intType>& result)
{
	if(result.GetSize()<cols) result.ResizeArray(cols);
	result.SetNumPoints(cols);

	for(intType counter1=0;counter1<cols;counter1++)
	{
		GetColumn(counter1, result[counter1]);
	}
}

template <class theType, class intType>
void CMatrix<theType,intType>::ExportRowsToArrayOfArrays(CHArray<CHArray<theType,intType>,intType>& result)
{
	if(result.GetSize()<rows) result.ResizeArray(rows);
	result.SetNumPoints(rows);

	for(intType counter1=0;counter1<rows;counter1++)
	{
		GetRow(counter1, result[counter1]);
	}
}

template <class theType, class intType>
void CMatrix<theType,intType>::ImportRowsFromArrayOfArrays(CHArray<CHArray<theType,intType>,intType>& source)
{
	ResizeMatrix(source[0].GetNumPoints(), source.GetNumPoints());

	for(intType counter1=0;counter1<rows;counter1++)
	{
		SetRow(counter1,source[counter1]);
	}
}

template <class theType, class intType>
void CMatrix<theType,intType>::ImportColsFromArrayOfArrays(CHArray<CHArray<theType,intType>,intType>& source)
{
	ResizeMatrix(source.GetNumPoints(), source[0].GetNumPoints());

	for(intType counter1=0;counter1<cols;counter1++)
	{
		SetColumn(counter1,source[counter1]);
	}
}

//Clusters the points in the matrix with Principal Direction Divisive Partitioning
//Until the size of the clusters is no more than maxNumInCluster
//The coordinates of points should be in matrix columns
//The columns are interchanged to group cluster members together
//Result contains the clusters of original column indices
//The order in result and in the permuted (*this) matrix are the same
template <class theType, class intType>
void CMatrix<theType,intType>::PDDPclustering(intType maxNumInCluster, CAIStrings<intType,intType>& result)
{
	CHArray<intType,intType> perm;
	perm.ResizeIfSmaller(cols,true);
	perm.SetValToPointNum();

	//Cluster description
	struct cluster
	{
		cluster(intType theStart, intType theEnd, intType theDiv, intType theRed, bool isFin):
				start(theStart),end(theEnd),div(theDiv),red(theRed),fin(isFin){};

		intType Count() const {return end-start;};

		intType start;	//position where the cluster starts
		intType end;	//and ends, [...)
		intType div;	//optimal division position for the cluster - it's an offset from start, not absolute numbering!
		theType red;	//reduction in total sq devs achieved after optimal division
		bool fin;		//Whether the cluster is "finished" - has no more points than required
	};
	std::list<cluster> clusterList;

	int numFinished=0;			//Number of columns already assigned to clusters with < than maxNumInCluster elements

	//Specify the top cluster and add it to list
	cluster topCluster(0,cols,0,0,false);
	(*this).PDDPhelper(topCluster.div,topCluster.red,perm);
	if(cols<=maxNumInCluster) {topCluster.fin=true;numFinished=cols;}

	clusterList.push_back(topCluster);
	
	//While there are clusters to divide, select the one with largest reduction
	//Divide it and run PDDPhelper on each of the two new clusters
	while(numFinished < cols)
	{
		theType curRed=-1;					//this initial choice is always changed in cluster selection
		auto curPick=clusterList.end();		//this initial choice is always changed in cluster selection

		//pick the cluster to divide - the cluster with the largest reduction
		for(auto it=clusterList.begin();it!=clusterList.end();it++)
		{
			cluster& curCluster=*it;
			if(!curCluster.fin && curCluster.red > curRed)
			{
				curPick=it;
				curRed=curCluster.red;
			}
		}

		//Current picked cluster
		cluster& picked=*curPick;
				
		cluster newCluster(picked.start + picked.div,picked.end,0,0,false);	//new cluster to be inserted after the picked one
		picked.end = picked.start + picked.div;			//removing the elements from the picked cluster that were moved to the new cluster
		
		//If the sizes have been reached, mark as finished and increment numFinished
		if(newCluster.Count() <= maxNumInCluster)
		{
			newCluster.fin=true;
			numFinished += newCluster.Count();
		}
		if(picked.Count() <= maxNumInCluster)
		{
			picked.fin=true;
			numFinished += picked.Count();
		}

		//Compute the division point and the sq dev reduction on the two new clusters
		//And permute them according to projection on the principal direction
		{
			CMatrix<theType,intType> virtMatrix(*this,picked.start,picked.Count());
			CHArray<intType,intType> virtArray(perm.arr + picked.start,picked.Count(),true);
			virtMatrix.PDDPhelper(picked.div,picked.red,virtArray);
		}

		{
			CMatrix<theType,intType> virtMatrix(*this,newCluster.start,newCluster.Count());
			CHArray<intType,intType> virtArray(perm.arr + newCluster.start,newCluster.Count(),true);
			virtMatrix.PDDPhelper(newCluster.div,newCluster.red,virtArray);
		}

		//Insert the new cluster after the picked cluster
		curPick++;
		clusterList.insert(curPick,newCluster);
	}

	//Compose the result CAIS from perm and the cluster list
	CHArray<intType,intType> iia((intType)clusterList.size()+1);
	iia.AddPoint(0);
	for(auto it=clusterList.begin();it!=clusterList.end();it++) iia.AddPoint((*it).end);
	result.SetDataAndIia(iia,perm);
}

//Helper function for PDDP clustering
//Creates a copy of the matrix
//Finds the projection of each column on the primary eigenvector
//Sorts the columns according to the projection
//Finds optimal division point
//And returns the division point and corresponding reduction in squared devs in divPoint and reduction
//Reorders the columns of the matrix according to the projection on the primary principal direction!
//arrayToPermute is the (virtual) part of the permutation that the main function is building that corresponds to
//the current (virtual) matrix
template <class theType, class intType>
void CMatrix<theType,intType>::PDDPhelper(intType& divPoint, theType& reduction, CHArray<intType,intType>& arrayToPermute)
{
	//Create a copy of curMat so that we can subtract the average from it
	CMatrix<theType,intType> copy(*this);

	//Compute the average on the columns
	CHArray<theType,intType> average(copy.rows,true);
	average=0;
	for(intType i=0;i<copy.cols;i++) average+=copy[i];
	average /= (theType) cols;

	//Subtract the average from each column
	for(int i=0;i<copy.cols;i++) copy[i]-=average;

	//Assumes that curMat has relatively few rows (but possibly many columns)
	//example is projections of words on eigenvectors in columns
	CMatrix<theType,intType> temp;
	copy.MatMultiply(copy,temp,false,true);		//mat product of curMat with itself to find the largest eigenvector
	CMatrix<theType,intType> tempEvecs;
	CHArray<theType,intType> tempEvals;

	//Find one largest eigenvector of temp
	temp.EigenRRR(1,tempEvals,tempEvecs);
	CHArray<theType,intType>& primEvec=tempEvecs[0];

	//Compute projections of each column onto the primary eigenvector
	CHArray<theType,intType> proj;
	copy.MatVecMultiply(primEvec,proj,true);

	//Permutation to sort the columns according to the projection
	CHArray<intType,intType> perm;
	proj.SortPermutation(perm);
	proj.Permute(perm);

	//Permute the columns of (*this) and the arrayToPermute
	(*this).PermuteColumns(perm);
	arrayToPermute.Permute(perm);

	//Find optimal division point and reduction for proj
	divPoint=proj.OptimalSplitSorted(&reduction);			//We are done
}

template <class theType, class intType>
bool CMatrix<theType,intType>::AgglomerativeClustering(theType entropicLimit, CHArray<CHArray<intType,intType>,intType>& result)
{
	if((entropicLimit>cols)||(entropicLimit<1)) return false;

	intType curNumClusters=cols;		//Number of groups will decrease as clusters are merged
	intType numCoords=rows;

	CHArray<intType,intType> clustering(cols,true);				//Array assigning points to a given cluster
	clustering.SetValToPointNum();					//Clusters will need to be renumbered 0 to max at the end

	CHArray<intType,intType> numPointsInClusters(cols,true);	//Points are deleted from this array as the number of clusters decreases
	numPointsInClusters=1;							//When merging, one cluster takes the points of both

	CHArray<CHArray<theType,intType>,intType> clusters(cols);		//Average coordinates of clusters
	ExportColsToArrayOfArrays(clusters);

	CMatrix<theType,intType> distances(cols,cols);						//Pairwise distances between clusters
	for(intType counter1=0;counter1<(cols-1);counter1++)			//Initialize distance matrix
	{
		for(intType counter2=counter1+1;counter2<cols;counter2++)
		{
			distances.ElementAt(counter1,counter2)=clusters[counter1].EuclideanDistance(clusters[counter2]);
		}
	}

	CHArray<intType,intType> remainingClusters(cols,true);		//Registration of remaining clusters to position number in "clusters"
	remainingClusters.SetValToPointNum();			//Points are deleted as clusters are merged

	//do while entropic limit is not reached
	while(numPointsInClusters.EntropicNumberOfStates()>entropicLimit)
	{
		//Find minimum pairwise distance between remaining clusters
		theType minDist=distances.ElementAt(remainingClusters[0],remainingClusters[1]);
		intType minCounter1=0;
		intType minCounter2=1;

		theType dist;
		for(intType counter1=0;counter1<(curNumClusters-1);counter1++)
		{
			intType index1=remainingClusters[counter1];
			for(intType counter2=counter1+1;counter2<curNumClusters;counter2++)
			{
				intType index2=remainingClusters[counter2];

				dist=distances.ElementAt(index1,index2);

				if(dist<minDist)
				{
					minDist=dist;
					minCounter1=counter1;
					minCounter2=counter2;
				}
			}
		}

		//At this point, the minimum distance is found
		//Now merge the second min dist cluster into the first one
		intType index1=remainingClusters[minCounter1];
		intType index2=remainingClusters[minCounter2];

		intType numPointsIn1=numPointsInClusters[minCounter1];
		intType numPointsIn2=numPointsInClusters[minCounter2];

		clusters[index1]*=(theType)numPointsIn1;
		clusters[index2]*=(theType)numPointsIn2;

		clusters[index1]+=clusters[index2];
		clusters[index1]*=((theType)1.0/(theType)(numPointsIn1+numPointsIn2));

		numPointsInClusters[minCounter1]+=numPointsIn2;

		//Now remove references to the second cluster
		remainingClusters.DeletePointAt(minCounter2);
		numPointsInClusters.DeletePointAt(minCounter2);

		for(intType counter1=0;counter1<cols;counter1++)
		{
			if(clustering[counter1]==index2) clustering[counter1]=index1;
		}

		curNumClusters--;

		//And update the distance table - index1 is the cluster that has kept its number
		for(intType counter1=0;counter1<curNumClusters;counter1++)
		{
			dist=clusters[index1].EuclideanDistance(clusters[remainingClusters[counter1]]);
			distances.ElementAt(index1,remainingClusters[counter1])=dist;
			distances.ElementAt(remainingClusters[counter1],index1)=dist;
		}
	}

	//Renumber the clusters in "clustering" to indices in "remainingClusters"
	CHArray<intType,intType> renumbering(cols,true);
	for(intType counter1=0;counter1<curNumClusters;counter1++)
	{
		renumbering[remainingClusters[counter1]]=counter1;
	}

	for(intType counter1=0;counter1<cols;counter1++)
	{
		clustering[counter1]=renumbering[clustering[counter1]];
	}

	//Convert data in "clustering" into array of arrays
	result.ResizeArray(curNumClusters, true);

	CHArray<intType,intType> perm;
	CHArray<intType,intType> invPerm;									//Inverse permutation
	numPointsInClusters.SortPermutation(perm,1);			//Sort clusters according to the number of points in them
	invPerm=perm;
	invPerm.InvertPermutation();
	numPointsInClusters.Permute(perm);

	for(intType counter1=0;counter1<curNumClusters;counter1++)				//Resize result arrays
	{
		result[counter1].ResizeArray(numPointsInClusters[counter1],false);
	}

	for(intType counter1=0;counter1<cols;counter1++)				//Add point numbers to result arrays
	{
		result[invPerm[clustering[counter1]]].AddPoint(counter1);
	}

	return true;
}

template <class theType, class intType>
bool CMatrix<theType,intType>::KmeansClustering(intType k, CHArray<CHArray<intType,intType>,intType>& result)
{
	intType numPoints=cols;
	intType numCoords=rows;

	if((k>numPoints)||(k<1)) return false;

	CHArray<intType,intType> clustering(numPoints,true);		//Indicates which cluster the point belongs to

	//Simple initialization of clustering array
	clustering=0;
	for(intType counter1=0;counter1<k;counter1++)
	{
		clustering[counter1]=counter1;
	}

	//Arrays for the coordinates of the mean of each group
	CHArray<CHArray<theType,intType>,intType> meanGroupCoord(k);
	for(intType counter1=0;counter1<k;counter1++)
	{
		meanGroupCoord[counter1].ResizeArray(numCoords, true);
	}

	//Number of points in each group
	CHArray<intType,intType> numPointsInGroup(k, true);

	//Distances between the mean and the point
	CHArray<theType,intType> distances(k,true);

	//Auxilliary array for extracting a column from the matrix - could be replaced by CHArray<CHArray<theType>>
	CHArray<theType,intType> aux(numCoords,true);

	//Old clustering - for the termination condition
	CHArray<intType,intType> oldClustering(clustering);

	//Iterate
	while(1)
	{
		//Compute the mean of each group
		numPointsInGroup=0;
		for(intType counter1=0;counter1<k;counter1++)
		{
			meanGroupCoord[counter1]=0;
		}

		for(intType counter1=0;counter1<numPoints;counter1++)
		{
			intType curGroup=clustering[counter1];
			numPointsInGroup[curGroup]+=1;

			GetColumn(counter1, aux);
			meanGroupCoord[curGroup]+=aux;
		}

		for(intType counter1=0;counter1<k;counter1++)
		{
			meanGroupCoord[counter1]*=((theType)1./(theType)numPointsInGroup[counter1]);
		}

		//reassign points to groups
		for(intType counter1=0;counter1<numPoints;counter1++)
		{
			GetColumn(counter1, aux);

			//Compute distances to centers of all groups
			for(intType counter2=0;counter2<k;counter2++)
			{
				distances[counter2]=meanGroupCoord[counter2].EuclideanDistance(aux);
			}

			clustering[counter1]=distances.FindPositionOfMin();
		}

		//Terminate if no change from last time
		if(clustering==oldClustering) break;

		oldClustering=clustering;
	}

	//Now reformat the data in "clustering" into k int arrays
	result.ResizeArray(k, true);

	CHArray<intType,intType> perm;
	CHArray<intType,intType> invPerm;									//Inverse permutation
	numPointsInGroup.SortPermutation(perm,1);				//Sort clusters according to the number of points in them
	invPerm=perm;
	invPerm.InvertPermutation();
	numPointsInGroup.Permute(perm);

	for(intType counter1=0;counter1<k;counter1++)				//Resize result arrays
	{
		result[counter1].ResizeArray(numPointsInGroup[counter1],false);
	}

	for(intType counter1=0;counter1<numPoints;counter1++)				//Add point numbers to result arrays
	{
		result[invPerm[clustering[counter1]]].AddPoint(counter1);
	}
	
	return true;
}

template <class theType, class intType>
bool CMatrix<theType,intType>::Write(const BString& fileName, BString format)
{
	CHArray<char,int64> charArr;
	if(cols==0 || rows==0) {return charArr.WriteBinary(fileName);}

	//double pass - first count the number of symbols, then write to a char array
	int64 numSymbols=0;
	BString buffer;
	for(intType i=0;i<rows;i++)
	{
		for(intType j=0;j<cols;j++)
		{
			buffer.Format(format,ElementAt(j,i));
			numSymbols+=buffer.GetLength();
		}
	}
	numSymbols+=(rows-1);			//for "\n" after every line, except the last one
	numSymbols+=rows*(cols-1);		//for "\t" after every element, except the last one in each row

	charArr.Resize(numSymbols,true);
	int64 curPos=0;
	for(intType i=0;i<rows;i++)
	{
		for(intType j=0;j<cols;j++)
		{
			buffer.Format(format,ElementAt(j,i));
			int64 curLength=buffer.GetLength();
			for(int k=0;k<curLength;k++)
			{
				charArr[curPos++]=buffer[k];
			}
			if(j<(cols-1)) charArr[curPos++]='\t';
		}
		if(i<(rows-1)) charArr[curPos++]='\n';
	}

	return charArr.WriteBinary(fileName);
}

template <class theType, class intType>
bool CMatrix<theType,intType>::Read(const BString& fileName)
{
	CHArray<char,int64> text;
	if(!text.ReadBinary(fileName)) return false;
	int64 textLength=text.GetNumPoints();
	if(textLength==0) {ResizeMatrix(0,0);return true;}

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
	if(textLength==0) {ResizeMatrix(0,0);return true;}

	//Remove terminating LF - there should be no LF at the end
	if(text[textLength-1]==10)
	{
		textLength--;
		text.SetNumPoints(textLength);
		if(textLength==0) {ResizeMatrix(0,0);return true;}
	}

	//Count the number of lines in the file
	intType numLines=1;
	for(int64 i=0;i<textLength;i++)
	{
		if(text.arr[i]==10) numLines++;
	}

	//Count the number of elements in the first row
	intType newNumCols=1;
	for(int64 i=1;i<textLength && text.arr[i]!=10;i++)
	{
		if(text.arr[i-1]<=32 && text.arr[i]>32) newNumCols++;
	}

	ResizeMatrix(newNumCols,numLines);

	//Maximum length of the line
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

	//Strings for current line and current element
	char* curLine=new char[maxLength+1];
	char* curElement=new char[maxLength+1];
	
	int64 numBufferedInLine=0;
	int64 numBufferedInElement=0;
	int64 curPosInText=0;
	int64 curPosInLine=0;

	intType curCol=0;
	intType curRow=0;

	while(curPosInText<textLength)
	{
		while(curPosInText<textLength && text.arr[curPosInText]!=10)		//copy current line to curLine
		{
			curLine[curPosInLine++]=text.arr[curPosInText++];
		}
		curPosInText++;
		numBufferedInLine=curPosInLine;
		curPosInLine=0;
	
		while(curPosInLine<numBufferedInLine)	
		{
			while(curPosInLine<numBufferedInLine && curLine[curPosInLine]<=32) curPosInLine++;	//remove leading blanks				
			
			while(curPosInLine<numBufferedInLine && curLine[curPosInLine]>32)				//read meaningful characters
			{
				curElement[numBufferedInElement++]=curLine[curPosInLine++];
			}
			curPosInLine++;
			curElement[numBufferedInElement]=NULL;

			if(numBufferedInElement>0) {ElementAt(curCol,curRow)=(theType)atof(curElement);}
	
			numBufferedInElement=0;
			curCol++;
		}
		curRow++;
		curCol=0;

		numBufferedInLine=0;
		curPosInLine=0;
	}
	
	delete[] curLine;
	delete[] curElement;
	return true;
}



template <class theType, class intType>
void CMatrix<theType,intType>::AddRow(CMatrix<theType,intType>& rhs, intType rowFrom)
{
	if(numRowsAdded>=rows) return;

	CopyRowFrom(rhs,numRowsAdded,rowFrom);
	numRowsAdded++;
}

template <class theType, class intType>
void CMatrix<theType,intType>::AddRow(CHArray<theType,intType>& rhs)
{
	if(numRowsAdded>=rows) return;

	SetRow(numRowsAdded,rhs);
	numRowsAdded++;
}

template <class theType, class intType>
theType CMatrix<theType,intType>::NormalizeColumn(intType colNum)
{
	return colArrays[colNum].NormalizeNorm();
}

template <class theType, class intType>
void CMatrix<theType,intType>::NormalizeAllColumns()
{
	for(intType counter1=0;counter1<cols;counter1++)
	{
		colArrays[counter1].NormalizeNorm();
	}
}

//Normalizes Euclidean norms for all rows
template <class theType, class intType>
void CMatrix<theType,intType>::NormalizeAllRows()
{
	//Does consequtive memory access
	CHArray<theType,intType> norms(rows,true);
	norms=0;

	//First pass - compute the norms
	for(intType colCounter=0;colCounter<cols;colCounter++)
	{
		for(intType rowCounter=0;rowCounter<rows;rowCounter++)
		{
			theType val=theArray.arr[colCounter*rows+rowCounter];
			norms.arr[rowCounter]+=val*val;
		}
	}

	norms.Sqrt();
	norms.Inverse();

	//second pass - normalize
	for(intType colCounter=0;colCounter<cols;colCounter++)
	{
		for(intType rowCounter=0;rowCounter<rows;rowCounter++)
		{
			theArray.arr[colCounter*rows+rowCounter]*=norms.arr[rowCounter];
		}
	}
}

template <class theType, class intType>
void CMatrix<theType,intType>::CopyRowFrom(CMatrix<theType,intType>& rhs,intType rowTo, intType rowFrom)
{
	intType numColsCopied=std::min(cols,rhs.cols);		//Truncation

	for(intType i=0;i<numColsCopied;i++)
	{
		theArray.arr[i*rows+rowTo]=rhs.theArray.arr[i*rhs.rows+rowFrom];
	}
}

template <class theType, class intType>
void CMatrix<theType,intType>::CopyColumnFrom(CMatrix<theType,intType>& rhs,intType colTo, intType colFrom)
{
	intType numRowsCopied=std::min(rows,rhs.rows);	//Truncation

	for(intType i=0;i<numRowsCopied;i++)
	{
		theArray.arr[colTo*rows+i]=rhs.theArray.arr[colFrom*rhs.rows+i];
	}
}

template <class theType, class intType>
void CMatrix<theType,intType>::CopyRowAsColumnFrom(CMatrix<theType,intType>& rhs, intType rowFrom, intType colTo)
{
	intType numRowsCopied=std::min(rows,rhs.cols);	//Truncation

	for(intType i=0;i<numRowsCopied;i++)
	{
		theArray.arr[colTo*rows+i]=rhs.theArray.arr[i*rhs.rows+rowFrom];
	}
}

template <class theType, class intType>
void CMatrix<theType,intType>::CopyColumAsRowFrom(CMatrix<theType,intType>& rhs, intType colFrom, intType rowTo)
{
	intType numColsCopied=std::min(cols,rhs.rows);		//Truncation

	for(intType i=0;i<numColsCopied;i++)
	{
		theArray.arr[i*rows+rowTo]=rhs.theArray.arr[colFrom*rhs.rows+i];
	}
}

//Copy all data with transposition, resize *this if necessary
template <class theType, class intType>
void CMatrix<theType,intType>::CopyTransposeFrom(CMatrix<theType, intType>& rhs)
{
	if(cols!=rhs.rows || rows!=rhs.cols) ResizeMatrix(rhs.rows,rhs.cols);
	HelperCopyTransposeFrom(rhs.theArray,rhs.cols,rhs.rows);
}

//Fills the matrix from an array - sizes must match exactly!
template <class theType, class intType>
void CMatrix<theType,intType>::HelperCopyTransposeFrom(CHArray<theType,intType>& arr, intType targetCols, intType targetRows)
{
	intType curPoint=0;
	for(intType rowCounter=0;rowCounter<targetRows;rowCounter++)
	{
		for(intType colCounter=0;colCounter<targetCols;colCounter++)
		{
			theArray.arr[curPoint++]=arr.arr[colCounter*targetRows+rowCounter];
		}
	}
}

template <class theType, class intType>
void CMatrix<theType,intType>::CopyFromMatrix(const CMatrix<theType,intType>& rhs, intType colOffset, intType rowOffset)
{
	//Copies an overlapping area from rhs to (*this)
	//The offsets position (*this) matrix over rhs

	intType startCol=std::max(0,-colOffset);
	intType limitCol=std::min(cols, rhs.cols-colOffset);		//this is end column number + 1

	intType startRow=std::max(0,-rowOffset);
	intType limitRow=std::min(rows, rhs.rows-rowOffset);		//this is end row number + 1

	for(intType colCounter=startCol;colCounter<limitCol;colCounter++)
	{
		for(intType rowCounter=startRow;rowCounter<limitRow;rowCounter++)
		{
			theArray.arr[colCounter*rows+rowCounter]=
				rhs.theArray.arr[(colCounter+colOffset)*rhs.rows+(rowCounter+rowOffset)];
		}
	}
}

template <class theType, class intType>		//double
BString CMatrix<theType,intType>::FullSVD(CHArray<double,intType>& values, CMatrix<double,intType>& leftVecs, CMatrix<double,intType>& rightVecs)
{
	intType numVals=std::min(rows,cols);				//number of singular values and vectors

	if(values.GetSize()<numVals) values.ResizeArray(numVals);
	values.SetNumPoints(numVals);

	if((leftVecs.rows!=rows)||(leftVecs.cols!=numVals)) leftVecs.ResizeMatrix(numVals,rows);
	if((rightVecs.cols!=cols)||(rightVecs.rows!=numVals)) rightVecs.ResizeMatrix(cols,numVals);

	char jobU='S';
	char jobVT='S';

	intType info;

	intType lwork=-1;
	double* work=new double[5];

	dgesvd(&jobU,&jobVT,&rows,&cols,theArray.arr,&rows/*lda*/,
			values.arr,leftVecs.theArray.arr,&leftVecs.rows/*ldu*/,
			rightVecs.theArray.arr,&rightVecs.rows/*ldvt*/,work,&lwork,&info);

	lwork=(intType)work[0];
	delete[] work;
	work=new double[lwork];

	dgesvd(&jobU,&jobVT,&rows,&cols,theArray.arr,&rows/*lda*/,
			values.arr,leftVecs.theArray.arr,&leftVecs.rows/*ldu*/,
			rightVecs.theArray.arr,&rightVecs.rows/*ldvt*/,work,&lwork,&info);

	delete[] work;

	if(info==0) return "DGESVD: ok.\n";
	if(info<0) return "DGESVD: illegal argument value.\n";
	/*if(info>0)*/ return "DGESVD: incomplete convergence.\n";
}

template <class theType, class intType>		//float
BString CMatrix<theType,intType>::FullSVD(CHArray<float,intType>& values, CMatrix<float,intType>& leftVecs, CMatrix<float,intType>& rightVecs)
{
	intType numVals=std::min(rows,cols);				//number of singular values and vectors

	if(values.GetSize()<numVals) values.ResizeArray(numVals);
	values.SetNumPoints(numVals);

	if((leftVecs.rows!=rows)||(leftVecs.cols!=numVals)) leftVecs.ResizeMatrix(numVals,rows);
	if((rightVecs.cols!=cols)||(rightVecs.rows!=numVals)) rightVecs.ResizeMatrix(cols,numVals);

	char jobU='S';
	char jobVT='S';

	intType info;

	intType lwork=-1;
	float* work=new float[5];

	sgesvd(&jobU,&jobVT,&rows,&cols,theArray.arr,&rows/*lda*/,
			values.arr,leftVecs.theArray.arr,&leftVecs.rows/*ldu*/,
			rightVecs.theArray.arr,&rightVecs.rows/*ldvt*/,work,&lwork,&info);

	lwork=(intType)work[0];
	delete[] work;
	work=new float[lwork];

	sgesvd(&jobU,&jobVT,&rows,&cols,theArray.arr,&rows/*lda*/,
			values.arr,leftVecs.theArray.arr,&leftVecs.rows/*ldu*/,
			rightVecs.theArray.arr,&rightVecs.rows/*ldvt*/,work,&lwork,&info);

	delete[] work;

	if(info==0) return "SGESVD: ok.\n";
	if(info<0) return "SGESVD: illegal argument value.\n";
	/*if(info>0)*/ return "SGESVD: incomplete convergence.\n";
}

template <class theType, class intType>		//float
BString CMatrix<theType,intType>::SVD(char leftRight, CHArray<float,intType>& values)
{
	intType numVals=std::min(rows,cols);				//number of singular values
	
	if(values.GetSize()<numVals) values.ResizeArray(numVals);
	values.SetNumPoints(numVals);

	char jobU;
	char jobVT;

	if(leftRight=='L') {jobU='O'; jobVT='N';}
	if(leftRight=='R') {jobU='N'; jobVT='O';}

	float u;	//unused
	float vt;	//unused
	intType ldu=1;
	intType ldvt=1;
	intType info;

	intType lwork=-1;
	float* work=new float[5];

	sgesvd(&jobU,&jobVT,&rows,&cols,theArray.arr,&rows/*lda*/,values.arr,&u,&ldu,&vt,&ldvt,work,&lwork,&info);

	lwork=(intType)work[0];
	
	delete[] work;
	work=new float[lwork];

	sgesvd(&jobU,&jobVT,&rows,&cols,theArray.arr,&rows/*lda*/,values.arr,&u,&ldu,&vt,&ldvt,work,&lwork,&info);

	delete[] work;

	if(info==0) return "SGESVD: ok.\n";
	if(info<0) return "SGESVD: illegal argument value.\n";
	/*if(info>0)*/ return "SGESVD: incomplete convergence.\n";
}

template <class theType, class intType>		//double
BString CMatrix<theType,intType>::SVD(char leftRight, CHArray<double,intType>& values)
{
	intType numVals=std::min(rows,cols);				//number of singular values

	if(values.GetSize()<numVals) values.ResizeArray(numVals);
	values.SetNumPoints(numVals);

	char jobU;
	char jobVT;

	if(leftRight=='L') {jobU='O'; jobVT='N';}
	if(leftRight=='R') {jobU='N'; jobVT='O';}

	double u;	//unused
	double vt;	//unused
	intType ldu=1;
	intType ldvt=1;
	intType info;

	intType lwork=-1;
	double* work=new double[5];

	dgesvd(&jobU,&jobVT,&rows,&cols,theArray.arr,&rows/*lda*/,values.arr,&u,&ldu,&vt,&ldvt,work,&lwork,&info);

	lwork=(intType)work[0];
	delete[] work;
	work=new double[lwork];

	dgesvd(&jobU,&jobVT,&rows,&cols,theArray.arr,&rows/*lda*/,values.arr,&u,&ldu,&vt,&ldvt,work,&lwork,&info);

	delete[] work;

	if(info==0) return "DGESVD: ok.\n";
	if(info<0) return "DGESVD: illegal argument value.\n";
	/*if(info>0)*/ return "DGESVD: incomplete convergence.\n";
}

template <class theType, class intType>		//float
bool CMatrix<theType,intType>::EigenRRR(intType n, CHArray<float,intType>& values, CMatrix<float,intType>& vectors, float absTol, bool fLargest)
{
	if(cols!=rows) return false;

	if(n>rows) n=rows;

	if(values.GetSize()<n) values.ResizeArray(n,true);

	if((vectors.cols!=n)||(vectors.rows)!=rows) vectors.ResizeMatrix(n,rows);

	float vl;		//Unused
	float vu;		//Unused

	intType il, iu;
	if(fLargest)
	{
		il=rows-n+1;	//Lower eigenvalue index - start from
		iu=rows;		//Higher eigenvalue index - go to
	}
	else					//Find n smallest
	{
		il=1;
		iu=n;
	}

	intType m;			//output - how many ev found - unused

	intType lwork=-1;
	float* work=new float[5];
	intType liwork=-1;
	intType* iwork=new intType[5];
	intType* isuppz=new intType[2*rows];
	intType info;

	ssyevr("V", "I", "U", &rows, theArray.arr, &rows/*lda*/, &vl,&vu,&il,&iu,&absTol,&m,
		values.arr, vectors.theArray.arr, &rows/*ldz*/, isuppz,work,&lwork,iwork,&liwork,&info);		//Work space request
	lwork=(intType)work[0];
	liwork=iwork[0];

	delete[] work;
	delete[] iwork;
	work=new float[lwork];
	iwork=new intType[liwork];

	ssyevr("V", "I", "U", &rows, theArray.arr, &rows/*lda*/, &vl, &vu, &il, &iu, &absTol,&m,
		values.arr, vectors.theArray.arr, &rows/*ldz*/,isuppz,work,&lwork,iwork,&liwork,&info);		//Decomposition

	delete[] work;
	delete[] iwork;
	delete[] isuppz;

	if(!info) return true;
	else return false;
}

template <class theType, class intType>	//double
bool CMatrix<theType,intType>::EigenRRR(intType n, CHArray<double,intType>& values, CMatrix<double,intType>& vectors, double absTol, bool fLargest)
{
	if(cols!=rows) return false;

	if(n>rows) n=rows;

	if(values.GetSize()<n) values.ResizeArray(n, true);

	if((vectors.cols!=n)||(vectors.rows)!=rows) vectors.ResizeMatrix(n,rows);

	double vl;		//Unused
	double vu;		//Unused
	
	intType il, iu;
	if(fLargest)
	{
		il=rows-n+1;	//Lower eigenvalue index - start from
		iu=rows;		//Higher eigenvalue index - go to
	}
	else					//Find n smallest
	{
		il=1;
		iu=n;
	}

	intType m;			//output - how many ev found - unused

	intType lwork=-1;
	double* work=new double[5];
	intType liwork=-1;
	intType* iwork=new intType[5];
	intType* isuppz=new intType[2*rows];
	intType info;

	dsyevr("V", "I", "U", &rows, theArray.arr, &rows/*lda*/, &vl,&vu,&il,&iu,&absTol,&m,
		values.arr, vectors.theArray.arr, &rows/*ldz*/, isuppz,work,&lwork,iwork,&liwork,&info);		//Work space request
	lwork=(intType)work[0];
	liwork=iwork[0];

	delete[] work;
	delete[] iwork;
	work=new double[lwork];
	iwork=new intType[liwork];

	dsyevr("V", "I", "U", &rows, theArray.arr, &rows/*lda*/, &vl, &vu, &il, &iu, &absTol,&m,
		values.arr, vectors.theArray.arr, &rows/*ldz*/,isuppz,work,&lwork,iwork,&liwork,&info);		//Decomposition

	delete[] work;
	delete[] iwork;
	delete[] isuppz;

	if(!info) return true;
	else return false;
}

//Finds column (fCol=true) or row (fCol=false) singular vectors
//Useful for finding the orthogonal basis for columns or rows - very fast!
//And SVD of large sparse matrices
//When the number of columns (rows) is less than the number of rows (columns)
//Result stored in eVecs, in cols if fCol=true, and in rows in fCol=false
template <class theType, class intType>
BString CMatrix<theType,intType>::SVDVectors(CHArray<theType,intType>& eVals, CMatrix<theType,intType>& eVecs, bool fCol)
{
	CMatrix<theType,intType> intermBasis;
	return SVDVectors(eVals,eVecs,fCol,intermBasis);
}

template <class theType, class intType>
BString CMatrix<theType,intType>::SVDVectors(CHArray<theType,intType>& eVals, CMatrix<theType,intType>& eVecs, bool fCol,
												CMatrix<theType,intType>& intermBasis)
{
	(*this).MatMultiply(*this,intermBasis,fCol,!fCol);	//Interm basis is A.Transpose(A) or Transpose(A).A

	BString retVal=intermBasis.EigenDAC(eVals);		//intermBasis is overwritten with eigenvectors in columns
	eVals.Sqrt();										//Need to take the square root to get singular values

	if(fCol)
	{
		(*this).MatMultiply(intermBasis,eVecs);
		eVecs.NormalizeAllColumns();
	}
	else
	{
		intermBasis.MatMultiply(*this,eVecs,true,false);
		eVecs.NormalizeAllRows();
	}

	return "SVDVectors: "+retVal;
}

template <class theType, class intType>
BString CMatrix<theType,intType>::EigenDAC(CHArray<double,intType>& evalues)
{
	if(cols!=rows) return "EigenDAC: Not square.\n";

	if(evalues.GetSize()<cols) evalues.ResizeArray(cols);
	evalues.SetNumPoints(cols);

	intType lwork=-1;
	intType liwork=-1;
	double* work=new double[5];
	intType* iwork=new intType[5];
	
	intType info;

	dsyevd("V", "U", &cols, theArray.arr, &cols, evalues.arr, work, &lwork, iwork, &liwork, &info);
	lwork=(intType)work[0];
	liwork=iwork[0];

	delete[] work;
	delete[] iwork;
	work=new double[lwork];
	iwork=new intType[liwork];

	dsyevd("V", "U", &cols, theArray.arr, &cols, evalues.arr, work, &lwork, iwork, &liwork, &info);
	delete[] work;
	delete[] iwork;
	if(!info) return "Dsyevd: OK.\n";
	else return "Dsyevd: unusual termination.\n";
}

template <class theType, class intType>
BString CMatrix<theType,intType>::EigenDAC(CHArray<float,intType>& evalues)		//for some reason, does not work
{
	if(cols!=rows) return "EigenDAC: Not square.\n";

	if(evalues.GetSize()<cols) evalues.ResizeArray(cols);
	evalues.SetNumPoints(cols);

	intType lwork=-1;
	intType liwork=-1;
	float* work=new float[5];
	intType* iwork=new intType[5];
	
	intType info;

	ssyevd("V", "U", &cols, theArray.arr, &cols, evalues.arr, work, &lwork, iwork, &liwork, &info);
	lwork=(intType)work[0];
	liwork=iwork[0];

	delete[] work;
	delete[] iwork;
	work=new float[lwork];
	iwork=new intType[liwork];

	ssyevd("V", "U", &cols, theArray.arr, &cols, evalues.arr, work, &lwork, iwork, &liwork, &info);
	delete[] work;
	delete[] iwork;
	if(!info) return "Ssyevd: OK.\n";
	else return "Ssyevd: unusual termination.\n";
}

//Permute columns according to the provided permutation
template <class theType, class intType>
void CMatrix<theType,intType>::PermuteColumns(const CHArray<intType,intType>& perm)
{
	if(perm.Count()!=cols) return;

	CMatrix<theType,intType> tempMat(*this);
	for(intType i=0;i<cols;i++)
	{
		(*this)[i]=tempMat[perm[i]];
	}
}

template <class theType, class intType>
void CMatrix<theType,intType>::Transpose()
{
	CHArray<theType,intType> copy(theArray);
	HelperCopyTransposeFrom(copy,cols,rows);
	if(rows!=cols) ResizeMatrix(rows,cols);
}

template <class theType, class intType>
void CMatrix<theType,intType>::ResizeMatrix(intType newCols, intType newRows)
{
	if(cols==newCols && rows==newRows) return;

	if((newCols*newRows)!=(cols*rows)) theArray.ResizeArray(newCols*newRows,true);

	cols=newCols;
	rows=newRows;

	SetColArrays();
}

template <class theType, class intType>
void CMatrix<theType,intType>::ResizeMatrixKeepPoints(intType newCols, intType newRows)
{
	if(cols==newCols && rows==newRows) return;

	CMatrix<theType,intType> temp(*this);
	ResizeMatrix(newCols, newRows);
	CopyFromMatrix(temp);
}

template <class theType, class intType>
CHArray<theType,intType>& CMatrix<theType,intType>::MatVecMultiply(CHArray<double,intType>& vec, CHArray<double,intType>& result,
										bool fTransposeA, double alpha, double beta) const
{
	const CMatrix<theType,intType>& A=*this;

	char transA;

	if(fTransposeA)	{transA='T';result.ResizeIfSmaller(cols,true);}
	else			{transA='N';result.ResizeIfSmaller(rows,true);}

	intType incVec=1;		//Spacing in vec array
	intType incResult=1;	//Spacing in result array
	dgemv(&transA,&A.rows,&A.cols,&alpha,A.theArray.arr,&A.rows/*lda*/,vec.arr,&incVec,&beta,result.arr,&incResult);

	return result;
}

template <class theType, class intType>
CHArray<theType,intType>& CMatrix<theType,intType>::MatVecMultiply(CHArray<float,intType>& vec, CHArray<float,intType>& result,
										bool fTransposeA, float alpha, float beta) const
{
	const CMatrix<theType,intType>& A=*this;

	char transA;

	if(fTransposeA)	{transA='T';result.ResizeIfSmaller(cols,true);}
	else			{transA='N';result.ResizeIfSmaller(rows,true);}

	intType incVec=1;		//Spacing in vec array
	intType incResult=1;	//Spacing in result array
	sgemv(&transA,&A.rows,&A.cols,&alpha,A.theArray.arr,&A.rows/*lda*/,vec.arr,&incVec,&beta,result.arr,&incResult);

	return result;
}

template <class theType, class intType>
CMatrix<theType,intType>& CMatrix<theType,intType>::MatMultiply(const CMatrix<double,intType>& B, CMatrix<double,intType>& C, 
		bool fTransposeA, bool fTransposeB, double alpha, double beta) const
{
	const CMatrix<theType,intType>& A=*this;
	
	intType a_cols, a_rows, b_cols, b_rows;					 //Number of rows and cols, mathematically

	char transA;
	char transB;

	if(fTransposeA){a_cols=A.rows;a_rows=A.cols;transA='T';}
	else {a_cols=A.cols;a_rows=A.rows;transA='N';}

	if(fTransposeB){b_cols=B.rows;b_rows=B.cols;transB='T';}
	else {b_cols=B.cols;b_rows=B.rows;transB='N';}

	if(a_cols!=b_rows) return C;						//Dimensional mismatch

	if((C.rows!=a_rows)||(C.cols!=b_cols)) C.ResizeMatrix(b_cols,a_rows);

	dgemm(&transA, &transB, &a_rows, &b_cols, &a_cols, &alpha,
		A.theArray.arr, &A.rows/*lda*/, B.theArray.arr, &B.rows/*ldb*/, &beta, C.theArray.arr, &C.rows/*ldc*/);

	return C;
}

template <class theType, class intType>
CMatrix<theType,intType>& CMatrix<theType,intType>::MatMultiply(const CMatrix<float,intType>& B, CMatrix<float,intType>& C, 
		bool fTransposeA, bool fTransposeB, float alpha, float beta) const
{
	const CMatrix<theType,intType>& A=*this;
	
	intType a_cols, a_rows, b_cols, b_rows;					 //Number of rows and cols, mathematically

	char transA;
	char transB;

	if(fTransposeA){a_cols=A.rows;a_rows=A.cols;transA='T';}
	else {a_cols=A.cols;a_rows=A.rows;transA='N';}

	if(fTransposeB){b_cols=B.rows;b_rows=B.cols;transB='T';}
	else {b_cols=B.cols;b_rows=B.rows;transB='N';}

	if(a_cols!=b_rows) return C;						//Dimensional mismatch

	if((C.rows!=a_rows)||(C.cols!=b_cols)) C.ResizeMatrix(b_cols,a_rows);

	sgemm(&transA, &transB, &a_rows, &b_cols, &a_cols, &alpha,
		A.theArray.arr, &A.rows/*lda*/, B.theArray.arr, &B.rows/*ldb*/, &beta, C.theArray.arr, &C.rows/*ldc*/);

	return C;
}

template <class theType, class intType>
CMatrix<theType,intType>& CMatrix<theType,intType>::operator=(const CMatrix<theType,intType>& rhs)
{
	if(&rhs==this) return *this;

	//If both sizes match, just copy data
	if(cols==rhs.cols && rows==rhs.rows) theArray=rhs.theArray;
	else
	{
		cols=rhs.cols;
		rows=rhs.rows;
		theArray=rhs.theArray;
		SetColArrays();
	}

	return *this;
}

template <class theType, class intType>
CMatrix<theType,intType>::CMatrix(const CMatrix<theType,intType>& theMat):
theArray(theMat.theArray),
cols(theMat.cols),
rows(theMat.rows),
numRowsAdded(theMat.numRowsAdded)
{
	SetColArrays();
}

template <class theType, class intType>
void CMatrix<theType,intType>::GetRow(intType rowNum, CHArray<theType,intType>& result) const
{
	if(rowNum>(rows-1)) return;

	if(result.GetSize()<cols) result.ResizeArray(cols);

	result.SetNumPoints(cols);
	for(intType counter1=0;counter1<cols;counter1++)
	{
		result.arr[counter1]=theArray.arr[counter1*rows+rowNum];
	}

	return;
}

template <class theType, class intType>
void CMatrix<theType,intType>::GetColumn(intType colNum, CHArray<theType,intType>& result) const
{
	if(colNum>(cols-1)) return;

	if(result.GetSize()<rows) result.ResizeArray(rows);

	result.SetNumPoints(rows);
	for(intType counter1=0;counter1<rows;counter1++)
	{
		result.arr[counter1]=theArray.arr[colNum*rows+counter1];
	}

	return;
}

template <class theType, class intType>
void CMatrix<theType,intType>::SetRow(intType rowNum, const CHArray<theType,intType>& source)
{
	if(rowNum>(rows-1)) return;
	if(source.GetNumPoints()!=cols) return;

	for(intType counter1=0;counter1<cols;counter1++)
	{
		theArray.arr[counter1*rows+rowNum]=source.arr[counter1];
	}

	return;
}

template <class theType, class intType>
void CMatrix<theType,intType>::SetColumn(intType colNum, const CHArray<theType,intType>& source)
{
	if(colNum>(cols-1)) return;
	if(source.GetNumPoints()!=rows) return;

	for(intType counter1=0;counter1<rows;counter1++)
	{
		theArray.arr[colNum*rows+counter1]=source.arr[counter1];
	}
	
	return;
}

template <class theType, class intType>
CMatrix<theType,intType>::CMatrix(intType theCols,intType theRows):
theArray(theCols*theRows, true),
cols(theCols),
rows(theRows),
numRowsAdded(0)
{
	SetColArrays();
}

//Create a virtual matrix in an existing one that only includes a certain number of columns
template <class theType, class intType>
CMatrix<theType,intType>::CMatrix(const CMatrix<theType,intType>& otherMat, intType startCol, intType numColsToInclude):
theArray(otherMat.theArray.arr + otherMat.rows*startCol, otherMat.rows*numColsToInclude, true),	//virtual array
cols(numColsToInclude),
rows(otherMat.rows),
colArrays(otherMat.colArrays,startCol,numColsToInclude),
numRowsAdded(0)
{
	//Do not call SetColArrays() - they are already set!
}

template <class theType, class intType>
CMatrix<theType,intType>::CMatrix(const BString& fileName):
theArray(1, true),
cols(1),
rows(1),
numRowsAdded(0)
{
	Load(fileName);
	SetColArrays();
}

template <class theType, class intType>
void CMatrix<theType,intType>::Concatenate(const CMatrix<theType,intType>& rhs, bool fConcatenateCols)
{
	//Either columns or rows are concatenated, depending on fConcatenateCols

	if(fConcatenateCols)		//concatenate columns
	{
		if(rows!=rhs.rows) return;

		theArray.Concatenate(rhs.theArray);
		cols+=rhs.cols;
		SetColArrays();
	}
	else		//concatenate rows
	{
		if(cols!=rhs.cols) return;

		CMatrix<theType,intType> copy(*this);

		intType oldRows=rows;
		ResizeMatrix(cols,rows+rhs.rows);
		CopyFromMatrix(copy);

		for(intType colCounter=0;colCounter<cols;colCounter++)
		{
			for(intType rowCounter=0;rowCounter<rhs.rows;rowCounter++)
			{
				ElementAt(colCounter,oldRows+rowCounter)=rhs(colCounter,rowCounter);
			}
		}
	}
}

template <class theType, class intType>
bool CMatrix<theType,intType>::ReadStrings(const BString& fileName)		//tab and CR separated strings
{
	CHArray<char,int64> text;
	if(!text.ReadBinary(fileName)) return false;
	int64 textLength=text.GetNumPoints();
	if(textLength==0) {ResizeMatrix(0,0);return true;}

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
	if(textLength==0) {ResizeMatrix(0,0);return true;}

	//Remove terminating LF - there should be no LF at the end
	if(text[textLength-1]==10)
	{
		textLength--;
		text.SetNumPoints(textLength);
		if(textLength==0) {ResizeMatrix(0,0);return true;}
	}

	//Count the number of lines in the file
	intType numLines=1;
	for(int64 i=0;i<textLength;i++)
	{
		if(text.arr[i]==10) numLines++;
	}

	//Count the number of elements in the first row
	intType newNumCols=1;
	for(int64 i=0;i<textLength && text.arr[i]!=10;i++)
	{
		if(text.arr[i]=='\t') newNumCols++;
	}

	ResizeMatrix(newNumCols,numLines);

	//Maximum length of the element
	intType maxLength=0;
	intType curMaxLength=0;
	for(int64 i=0;i<=textLength;i++)
	{
		if(i<textLength && text.arr[i]!=10 && text.arr[i]!='\t') curMaxLength++;
		else
		{
			if(curMaxLength>maxLength) {maxLength=curMaxLength;}
			curMaxLength=0;
		}
	}
	if(curMaxLength>maxLength) maxLength=curMaxLength;

	//Read strings
	char* curString=new char[maxLength+1];

	int64 curStringPos=0;
	int64 curTextPos=0;
	for(intType rowCounter=0;rowCounter<rows;rowCounter++)
	{
		for(intType colCounter=0;colCounter<cols;colCounter++)
		{
			while(curTextPos<textLength && text.arr[curTextPos]!='\t' && text.arr[curTextPos]!=10)
			{
				curString[curStringPos++]=text[curTextPos++];
			}
			curString[curStringPos]=NULL;
			ElementAt(colCounter,rowCounter)=curString;
			curStringPos=0;
			curTextPos++;
		}
	}

	delete[] curString;
	return true;
}

template <class theType, class intType>
bool CMatrix<theType,intType>::WriteStrings(const BString& fileName, bool fOnlyAddedRows)
{
	int theRows;
	if(fOnlyAddedRows) theRows = numRowsAdded;
	else theRows = rows;

	//double pass - first count the number of symbols, then write to a char array
	int64 numSymbols=0;
	for(intType i=0;i<theRows;i++)
	{
		for(intType j=0;j<cols;j++)	{numSymbols+=ElementAt(j,i).GetLength();}
	}
	numSymbols+=(theRows-1);			//for "\n" after every line, except the last one
	numSymbols+=theRows*(cols-1);		//for "\t" after every element, except the last one in each row

	CHArray<char,int64> charArr(numSymbols,true);
	int64 curPos=0;
	for(intType i=0;i<theRows;i++)
	{
		for(intType j=0;j<cols;j++)
		{
			theType& curElement=ElementAt(j,i);
			int64 curLength=curElement.GetLength();

			for(int k=0;k<curLength;k++){charArr[curPos++]=curElement[k];}

			if(j<(cols-1)) charArr[curPos++]='\t';
		}
		if(i<(theRows-1)) charArr[curPos++]='\n';
	}

	return charArr.WriteBinary(fileName);
}
