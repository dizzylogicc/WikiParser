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

#include "Common.h"
#include <fstream>

#ifdef _MFC_VER
	#include "stdafx.h"
#endif

#pragma warning(disable:4996 4244)

BString CCommon::ByteArrayToHexString(const CHArray<char>& theArr)
{
	BString result;
	BString buffer;
	int numPoints=theArr.GetNumPoints();
	for(int i=0;i<numPoints;i++)
	{
		buffer.Format("%02x",theArr[i]);
		result+=buffer;
	}

	return result;
}

BString CCommon::GetExtension(const BString& fileName)
{
	BString extension="";
	int length=fileName.GetLength();

	int i;
	for(i=length-1;i>=0 && fileName[i]!='.';i--)
	{
		extension=fileName[i]+extension;
	}

	if(i<0) return "";
	else return extension;
}

//Snippet must be null-terminated
int64 CCommon::FindInString(const char* string, const char* snippet, int64 startPos, int64 stringSize)
{
	for(int64 i=startPos;i<stringSize;i++)
	{
		int64 posInSnippet=0;

		while((i+posInSnippet)<stringSize &&
				snippet[posInSnippet]==string[i+posInSnippet] &&
				string[i+posInSnippet]!=0)
		{
			posInSnippet++;
		}

		if(snippet[posInSnippet]==0) return i;	//Reached the end of the snippet - match found
	}

	return -1;	//no matches found
}

//converts a BString containing an IP address into UINT
unsigned int CCommon::IPStringToUINT(const BString& IP)
{
	unsigned int result=0;
	int pos=0;

	BString curToken=IP.Tokenize(".", pos);
	while(curToken!="")
	{
		result=result*256+atoi(curToken);

		curToken=IP.Tokenize(".", pos);
	}

	return result;
}

//converts a decimal string to long long integer
long long CCommon::atoll(const char* string)
{
	long long retval;
	retval=0;
	long long sign=1;

	for (int i=0;string[i]!=NULL;i++)
	{
		char curChar=string[i];
		if(curChar<'0' || curChar>'9')
		{
			if(curChar=='-') {sign*=-1;}
			continue;
		}

		retval = 10*retval + (curChar - '0');
	}

	retval*=sign;
	return retval;
}

BString CCommon::MakeNounPlural(BString noun)
{
	int length=noun.GetLength();
	if(length==0) return "";
	if(length==1) return noun+"s";

	char last=noun[length-1];
	char nextToLast=noun[length-2];
	
	BString plural;
	bool selected=false;

	if((!selected)&&(noun.Right(3)=="man"))
	{
		plural=noun.Left(length-2)+"en";
		selected=true;
	}

	if((!selected)&&((noun.Right(3)=="sis")||(noun.Right(3)=="xis")))
	{
		plural=noun.Left(length-2)+"es";
		selected=true;
	}

	if((!selected)&&(last=='y')&& IsConsonant(nextToLast))
	{
		plural=noun.Left(length-1);
		plural+="ies";
		selected=true;
	}

	if((!selected)&&(last=='o'))
	{
		plural=noun+"s";
		selected=true;
	}

	if((!selected)&& EndsInSibilant(noun))
	{
		plural=noun+"es";
		selected=true;
	}

	if(!selected)
	{
		plural=noun+"s";
		selected=true;
	}

	return plural;
}

BString CCommon::MakeAdverbFromAdj(BString adj)
{
	int length=adj.GetLength();
	if(length==0) return "";
	if(length==1) return adj+"ly";

	char last=adj[length-1];
	char nextToLast=adj[length-2];

	BString adverb="";
	bool selected=false;

	//final y
	if((!selected)&&(last=='y')&& IsConsonant(nextToLast))
	{
		adverb=adj.Left(length-1);
		adverb+="ily";
		selected=true;
	}

	//final -ic
	if(!selected && adj.Right(2)=="ic")
	{
		adverb=adj;
		adverb+="ally";
		selected=true;
	}

	if(!selected && adj.Right(3)=="ble")
	{
		adverb=adj.Left(length-1);
		adverb+="y";
		selected=true;
	}

	//default
	if(!selected)
	{
		adverb=adj+"ly";
		selected=true;
	}

	return adverb;
}

BString CCommon::MakeErAdj(BString adj)
{
	int length=adj.GetLength();
	if(length==0) return "";
	if(length<3) return adj+"er";

	char last=adj[length-1];
	char last2=adj[length-2];
	char last3=adj[length-3];

	BString result="";
	bool selected=false;

	//y at the end
	if((!selected)&&(last=='y')&& IsConsonant(last2))
	{
		result=adj.Left(length-1);
		result+="ier";
		selected=true;
	}

	//consonant doubling
	if(!selected && IsConsonant(last) && IsVowel(last2) && IsConsonant(last3))
	{
		result=adj;
		result+=last;
		result+="er";
		selected=true;
	}

	if(!selected && last=='e')
	{
		result=adj+"r";
		selected=true;
	}

	//default
	if(!selected)
	{
		result=adj+"er";
		selected=true;
	}

	return result;
}

int CCommon::NumVowelsInWord(BString word)
{
	int result=0;

	int length=word.GetLength();
	for(int counter1=0;counter1<length;counter1++)
	{
		if(IsVowel(word[counter1])) result++;
	}

	return result;
}

BString CCommon::MakeNessNoun(BString word)
{
	//Make a -ness noun from an adjective
	int length=word.GetLength();
	if(length==0) return "";
	if(length==1) return word+"ness";

	char last=word[length-1];
	char nextToLast=word[length-2];

	BString result="";
	bool selected=false;

	//final y
	if((!selected)&&(last=='y')&& IsConsonant(nextToLast))
	{
		result=word.Left(length-1);
		result+="iness";
		selected=true;
	}

	//default
	if(!selected)
	{
		result=word+"ness";
		selected=true;
	}

	return result;
}

BString CCommon::MakeLessAdj(BString word)
{
	//Make a -less adjective from a noun
	int length=word.GetLength();
	if(length==0) return "";
	if(length==1) return word+"less";

	char last=word[length-1];
	char nextToLast=word[length-2];

	BString result="";
	bool selected=false;

	//final y
	if((!selected)&&(last=='y')&& IsConsonant(nextToLast))
	{
		result=word.Left(length-1);
		result+="iless";
		selected=true;
	}

	//default
	if(!selected)
	{
		result=word+"less";
		selected=true;
	}

	return result;
}

BString CCommon::MakeEstAdj(BString adj)
{
	int length=adj.GetLength();
	if(length==0) return "";
	if(length<3) return adj+"est";

	char last=adj[length-1];
	char last2=adj[length-2];
	char last3=adj[length-3];

	BString result="";
	bool selected=false;

	//final y
	if((!selected)&&(last=='y')&& IsConsonant(last2))
	{
		result=adj.Left(length-1);
		result+="iest";
		selected=true;
	}

	//consonant doubling
	if(!selected && IsConsonant(last) && IsVowel(last2) && IsConsonant(last3))
	{
		result=adj;
		result+=last;
		result+="est";
		selected=true;
	}

	//final e
	if(!selected && last=='e')
	{
		result=adj+"st";
		selected=true;
	}

	if(!selected)
	{
		result=adj+"est";
		selected=true;
	}

	return result;
}



int CCommon::AddStringToCharPointer(char* pointer, int startFrom, BString string)
{
	int stringLength=string.GetLength();

	for(int counter1=0;counter1<stringLength;counter1++)
	{
		pointer[startFrom+counter1]=string[counter1];
	}

	return startFrom+stringLength;
}

void CCommon::ShowInt(long long val, BString comment)
{
#ifdef _MFC_VER
	CString buffer;
	buffer.Format(_T("%s%lli"),CString(comment), val);
	AfxMessageBox(buffer);
#else
	std::cout << comment << val << "\n";
#endif
}

void CCommon::ShowFloat(double val, BString comment)
{
#ifdef _MFC_VER
	CString buffer;
	buffer.Format(_T("%s%.6e"),CString(comment),val);
	AfxMessageBox(buffer);
#else
	std::cout << comment << val << "\n";
#endif
}

void CCommon::ShowString(BString string, BString comment)
{
#ifdef _MFC_VER
	CString buffer=CString(comment)+CString(string);
	AfxMessageBox(buffer);
#else
	std::cout << comment << string << "\n";
#endif
}

BString CCommon::RemoveAfterAndIncluding(BString label, BString string)
{
	int pos=string.Find(label,0);
	if(pos!=-1)
	{
		string=string.Left(pos);
	}
	return string;
}

bool CCommon::IsVowel(char a)
{
	if(
		(a=='a')||
		(a=='e')||
		(a=='i')||
		(a=='o')||
		(a=='u')||
		(a=='y')
		)
		return true;

	else return false;
}

bool CCommon::IsHardConsonant(char a)
{
	if(
		(a=='b')||
		(a=='d')||
		(a=='f')||
		(a=='g')||
		(a=='k')||
		(a=='l')||
		(a=='m')||
		(a=='n')||
		(a=='p')||
		(a=='q')||
		(a=='r')||
		(a=='s')||
		(a=='t')||
		(a=='v')||
		(a=='z')
		)
		return true;

	else return false;
}

bool CCommon::EndsInSibilant(BString word)
{
	int length=word.GetLength();
	char last=word[length-1];

	if(
		(last=='s')||
		(last=='z')||
		(last=='x')
		)

		return true;

	BString lastTwo=word.Right(2);

	if(
		(lastTwo=="ch")||
		(lastTwo=="sh")
		)

		return true;

	else return false;
}

BString CCommon::GetBracketed(BString& string, BString leftStr, BString rightStr)
{
	int rightLength=rightStr.GetLength();
	int leftLength=leftStr.GetLength();

	int rightPos=-rightLength;
	int leftPos=0;

	BString result="";
	while((leftPos=string.Find(leftStr,rightPos+rightLength))!=(-1))
	{
		if((rightPos=string.Find(rightStr,leftPos+rightLength))==(-1)) break;
		result+=string.Mid(leftPos+leftLength, rightPos-leftPos-leftLength);
		result+=" | ";
	}

	return result;
}

void CCommon::RemoveBracketed(BString& string, char left, char right)
{
	int length=string.GetLength();
	char* temp=new char[length+1];

	for(int counter1=0;counter1<length;counter1++)
	{
		temp[counter1]=string[counter1];
	}

	temp[length]=NULL;

	RemoveBracketed(temp,left,right);

	string=temp;
	delete[] temp;
}

int CCommon::ProcessBracketedByStrings(char* oldString, char* newString, char* left, char* right, char* altRight, bool fKeepOrRemove)
{
	int rightLength; for(rightLength=0;right[rightLength]!=NULL;rightLength++);	//Calculating lengths
	int altRightLength; if(altRight!=NULL) for(altRightLength=0;altRight[altRightLength]!=NULL;altRightLength++);
	int leftLength; for(leftLength=0;left[leftLength]!=NULL;leftLength++);
	int oldLength; for(oldLength=0;oldString[oldLength]!=NULL;oldLength++);

	for(int curSymbol=0;curSymbol<oldLength;curSymbol++)
	{
		newString[curSymbol]=0;
		if(BoundedStringCompare(oldString+curSymbol,left)) {newString[curSymbol]=1;curSymbol+=(leftLength-1);continue;}
		if(BoundedStringCompare(oldString+curSymbol,right)) {newString[curSymbol]=2;curSymbol+=(rightLength-1);continue;}
		if(altRight!=NULL && BoundedStringCompare(oldString+curSymbol,altRight)) {newString[curSymbol]=2;curSymbol+=(altRightLength-1);continue;}
	}
	
	int curNewPos=0;
	char curState=0;
	for(int curOldPos=0;curOldPos<oldLength;curOldPos++)
	{
		if(newString[curOldPos]==1) {curState++;curOldPos+=(leftLength-1);continue;}
		if(newString[curOldPos]==2) {curState--;curOldPos+=(rightLength-1);continue;}
		
		if((fKeepOrRemove && curState>0) || (!fKeepOrRemove && curState==0))
		{
			newString[curNewPos]=oldString[curOldPos];
			curNewPos++;
		}
	}
	newString[curNewPos]=NULL;
	return curNewPos;
}

bool CCommon::BoundedStringCompare(char* str1, char* str2)
{
	for(int counter1=0;str1[counter1]!=NULL && str2[counter1]!=NULL;counter1++)
	{
		if(str1[counter1]!=str2[counter1]) return false;
	}
	return true;
}


void CCommon::RemoveBracketedByStrings(BString& string, BString left, BString right, BString altRight)
{
	int length=string.GetLength();
	int rightLength=right.GetLength();
	int altRightLength=altRight.GetLength();

	CHArray<char> flagArray(length, true);
	flagArray=0;

	int pos1=0;
	while((pos1=string.Find(left,pos1))!=-1)
	{
		flagArray.arr[pos1]=1;
		pos1++;
	}

	pos1=0;
	while((pos1=string.Find(right,pos1))!=-1)
	{
		flagArray.arr[pos1+rightLength-1]=-1;
		pos1+=rightLength;
	}

	if(altRight!="")
	{
		pos1=0;
		while((pos1=string.Find(altRight,pos1))!=-1)
		{
			flagArray.arr[pos1+altRightLength-1]=-1;
			pos1+=altRightLength;
		}
	}

	//Check correctness
	int integral=0;
	for(int i=0;i<length;i++)
	{
		integral+=flagArray[i];

		if( integral<0 ) return;
	}
	if(integral!=0) return;
	
	RemoveUsingMarkup(string, flagArray);
}

void CCommon::RemoveUsingMarkup(BString& string, CHArray<char>& markup)
{
	//Removes whenever curState > 1
	//increments curState when markup[counter1]==1
	//decrements it when markup[counter1]==-1

	int length=string.GetLength();

	int curState=0;
	char* newString=new char[length+1];
	int curPos=0;
	for(int counter1=0;counter1<length;counter1++)
	{
		if(markup.arr[counter1]==1) curState++;

		if(curState==0)
		{
			newString[curPos++]=string[counter1];
		}

		if(markup.arr[counter1]==-1 && curState>0) curState--;
	}
	newString[curPos]=NULL;

	string=newString;
	delete[] newString;
}

void CCommon::RemoveBracketed(char* string, char left, char right)
{
	int curNum=0;
	int curState=0;
	for(int counter1=0;string[counter1]!=0;counter1++)
	{
		if(string[counter1]==left) curState++;

		if(curState==0) {string[curNum++]=string[counter1];}

		if(string[counter1]==right) curState--;
	}
	string[curNum]=NULL;
}

void CCommon::Replace(BString& text, const BString& toReplace, const BString& replacement)
{
	int numReplacements=0;

	int pos=0;
	pos=text.Find(toReplace,pos);
	while(pos!=-1)
	{
		numReplacements++;
		pos=text.Find(toReplace,pos+1);
	}

	if(numReplacements==0) return;

	int newSize=text.GetLength()+numReplacements*(replacement.GetLength()-toReplace.GetLength());

	CHArray<char> newString(newSize+1);
	
	ReplaceInString(text, newString.arr, toReplace, replacement);

	newString[newSize]=NULL;
	text=newString.arr;

	return;
}

void CCommon::ReplaceInString(const char* oldString, char* newString, const char* toReplace, const char* replacement)
{
	//All strings must be NULL-terminated
	//Does not check bounds on newString!

	int oldNum=0;
	while(oldString[oldNum]!=0) oldNum++;

	int curNum=0;		//Number of chars in the new string
	for(int oldCounter=0;oldString[oldCounter]!=0;oldCounter++)
	{
		bool fMatch=true;
		int shift;
		for(shift=0;toReplace[shift]!=0;shift++)
		{
			if((oldCounter+shift)<oldNum)
			{
				if(oldString[oldCounter+shift]!=toReplace[shift]) {fMatch=false; break;}
			}
			else {fMatch=false; break;}
		}

		if(fMatch)
		{
			for(int rCounter=0;replacement[rCounter]!=0;rCounter++)
			{
				newString[curNum++]=replacement[rCounter];
			}
			oldCounter+=shift;
			oldCounter--;
		}
		else newString[curNum++]=oldString[oldCounter];
	}
	newString[curNum]=NULL;
}

bool CCommon::ReadString(BString& string, const BString& fileName)
{
	std::ifstream instream(fileName, std::ios::binary);
	if (!instream) return false;

	instream.seekg(0, instream.end);
	std::streampos length = instream.tellg();
	instream.seekg(0, instream.beg);

	char* buffer = new char[length + std::streampos(1)];
	instream.read(buffer, length);
	buffer[length] = 0;

	string = buffer;
	delete[] buffer;
	return true;
}

bool CCommon::WriteString(const BString& string, const BString& fileName)
{
	std::ofstream outstream(fileName,std::ofstream::binary);
	if(!outstream) return false;

	outstream.write((const char*)string, string.GetLength());
	return true;
}

int CCommon::ToInt(BString& string)
{
	int result;
	sscanf(string, "%i", &result);
	return result;
}

BString CCommon::ToString(int val)
{
	BString result;
	result.Format("%i", val);
	return result;
}