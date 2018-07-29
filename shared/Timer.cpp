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

// Timer.cpp: implementation of the CTimer class.

#if defined(__unix) || defined(__APPLE__)
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <sys/timeb.h>
#include <time.h>
#include <windows.h>
#endif

#include "Timer.h"

// Construction/Destruction

CTimer::CTimer()
{
	#ifdef _WIN32
		LARGE_INTEGER f;
	    QueryPerformanceFrequency(&f);

		frequency=(double)f.QuadPart;
	#endif
}

void CTimer::SetTimerZero(int timerNum)
{
	zeroTime[timerNum]=GetTime();
}

double CTimer::GetCurTime(int timerNum)
{
	return GetTime() - zeroTime[timerNum];
}

BString CTimer::GetCurTimerString(int timerNum, const BString& format /*= "%.4f"*/)
{
	BString result;
	result.Format(format, GetCurTime(timerNum));
	return result;
}

double CTimer::GetCurTimeAndZero(int timerNum)
{
	double res=GetCurTime(timerNum);
	SetTimerZero(timerNum);
	return res;
}

double CTimer::GetTime()
{
	#if defined(__unix) || defined(__APPLE__)
	    struct timeval tv;
	    struct timezone tz;
	    gettimeofday(&tv, &tz);
	    return tv.tv_sec + tv.tv_usec * 1e-6;
	#elif defined(_WIN32)
	    LARGE_INTEGER t;
	    QueryPerformanceCounter(&t);
	    return double(t.QuadPart)/frequency;
	#else
	    return 0;
	#endif
}

void CTimer::WaitUntil(double time, int timerNum)
{
	while(GetCurTime(timerNum)<time) {};
	return;
}
