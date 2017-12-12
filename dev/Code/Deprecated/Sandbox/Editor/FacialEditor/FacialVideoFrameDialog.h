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

#ifndef CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALVIDEOFRAMEDIALOG_H
#define CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALVIDEOFRAMEDIALOG_H
#pragma once


#include "FacialEdContext.h"
#include "ToolbarDialog.h"
//#include <atlimage.h>

class CImage2;

class CFacialVideoFrameDialog : public CToolbarDialog
{
	DECLARE_DYNAMIC(CFacialVideoFrameDialog)
public:
	enum { IDD = IDD_DATABASE };

	CFacialVideoFrameDialog();
	~CFacialVideoFrameDialog();

	void SetContext(CFacialEdContext* pContext);
	void SetResolution(int width, int height, int bpp);
	void* GetBits();
	int GetPitch();	

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK() {};
	virtual void OnCancel() {};
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnSize(UINT nType, int cx, int cy);
	virtual void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnPaint();
	CBitmap m_offscreenBitmap;

	CFacialEdContext* m_pContext;
	HACCEL m_hAccelerators;
	//ATL::CImage m_image;
	CImage2	*m_image;
	//CStatic m_static;
};

#endif // CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALVIDEOFRAMEDIALOG_H
