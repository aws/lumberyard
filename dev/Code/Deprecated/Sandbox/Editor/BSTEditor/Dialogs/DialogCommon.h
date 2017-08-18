/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_DIALOGCOMMON_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_DIALOGCOMMON_H
#pragma once


#include "StringDlg.h"

class CDialogPositionHelper
{
public:
	static void SetToMouseCursor(CDialog& diag, bool center = false)
	{
		CRect formRect;
		diag.GetWindowRect(&formRect);
		const int width = formRect.Width();
		const int height = formRect.Height();
		CPoint clientPoint;
		::GetCursorPos( &clientPoint );
		if (center)
		{
			clientPoint.x -= width / 2;
			clientPoint.y -= height / 2;
		}
		formRect.TopLeft() = clientPoint;
		formRect.BottomRight() = CPoint(formRect.TopLeft().x + width, formRect.TopLeft().y + height);
		diag.MoveWindow(formRect, TRUE); 
	}
};

class CBSTStringDlg : public CStringDlg
{
public:
	CBSTStringDlg( const char *title = NULL,CWnd* pParent = NULL) : CStringDlg(title, pParent) {}
protected:
	virtual BOOL OnInitDialog()
	{
		BOOL res = CStringDlg::OnInitDialog();
		CDialogPositionHelper::SetToMouseCursor(*this, true);
		return res;
	}
};


#endif // CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_DIALOGCOMMON_H
