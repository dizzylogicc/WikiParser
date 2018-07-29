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

#include "QtUtils.h"
#include "BString.h"
#include <QMessageBox>
#include <QString>

void QtUtils::Show(int val, const BString& description)
{
	BString str;
	str.Format("%s%i", description.c_str(), val);
	InfoBox(str);
}

void QtUtils::Show(bool val, const BString& description)
{
	BString str;

	BString bStr;
	if (val) bStr = "true";
	else bStr = "false";

	str.Format("%s%s", description.c_str(), bStr.c_str());
	InfoBox(str);
}

void QtUtils::Show(double val, const BString& description)
{
	BString str;
	str.Format("%s%.4f", description.c_str() , val);
	InfoBox(str);
}

void QtUtils::Show(const BString& string)
{
	InfoBox(string);
}

void QtUtils::WarningBox(const BString& string)
{
	QMessageBox::warning(
		nullptr,
		QString("Warning"),
		QString((const char*)string));
}

void QtUtils::InfoBox(const BString& string)
{
	QMessageBox::information(
		nullptr,
		QString("Info"),
		QString((const char*)string));
}

void QtUtils::ErrorBox(const BString& string)
{
	QMessageBox::critical(
		nullptr,
		QString("Error"),
		QString((const char*)string));
}

//Enables or disables all widgets in the array
void QtUtils::EnableWidgets(CHArray<QWidget*>& widgets, bool fEnable)
{
	for (auto& x : widgets) x->setEnabled(fEnable);
}

//Sets all widgets visible or invisible
void QtUtils::SetWidgetsVisible(CHArray<QWidget*>& widgets, bool fVisible)
{
	for (auto& x : widgets) x->setVisible(fVisible);
}

