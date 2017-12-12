// EditWithButton.cpp : implementation file
//

//DISCLAIMER:
//The code in this project is Copyright (C) 2006 by Gautam Jain. You have the right to
//use and distribute the code in any way you see fit as long as this paragraph is included
//with the distribution. No warranties or claims are made as to the validity of the
//information and code contained herein, so use it at your own risk.

#include "stdafx.h"
#include "EditWithButton.h"
#include "UserMessageDefines.h"

// CEditWithButton

IMPLEMENT_DYNAMIC(CEditWithButton, CEdit)

CEditWithButton::CEditWithButton()
{
	m_iButtonClickedMessageId = WM_USER_EDITWITHBUTTON_CLICKED;
	m_bButtonExistsAlways[0] = FALSE;
	m_bButtonExistsAlways[1] = FALSE;

	m_rcEditArea.SetRect(0,0,0,0);
}

CEditWithButton::~CEditWithButton()
{
	m_bmpEmptyEdit.DeleteObject();
	m_bmpFilledEdit.DeleteObject();
	
}


BEGIN_MESSAGE_MAP(CEditWithButton, CEdit)

	ON_MESSAGE(WM_SETFONT, OnSetFont)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	ON_WM_CREATE()
END_MESSAGE_MAP()



// CEditWithButton message handlers

void CEditWithButton::PreSubclassWindow( )
{	
	// We must have a multiline edit
	// to be able to set the edit rect
	ASSERT( GetStyle() & ES_MULTILINE );

	ResizeWindow();

}

BOOL CEditWithButton::PreTranslateMessage( MSG* pMsg )
{
	return CEdit::PreTranslateMessage(pMsg);
}

BOOL CEditWithButton::SetBitmaps(UINT iEmptyEdit, UINT iFilledEdit)
{
	BITMAP bmpInfo;

	//delete if already loaded.. just in case
	m_bmpEmptyEdit.DeleteObject();
	m_bmpFilledEdit.DeleteObject();

	m_bmpEmptyEdit.LoadBitmap(iEmptyEdit);
	m_bmpFilledEdit.LoadBitmap(iFilledEdit);
	
	m_bmpEmptyEdit.GetBitmap(&bmpInfo);
	m_sizeEmptyBitmap.SetSize(bmpInfo.bmWidth,bmpInfo.bmHeight);
	
	m_bmpFilledEdit.GetBitmap(&bmpInfo);
	m_sizeFilledBitmap.SetSize(bmpInfo.bmWidth,bmpInfo.bmHeight);

	
	return TRUE;

}

//client area
void CEditWithButton::SetFirstButtonArea(CRect rcButtonArea)
{
	m_rcButtonArea[0] = rcButtonArea;
}

void CEditWithButton::SetSecondButtonArea(CRect rcButtonArea)
{
	m_rcButtonArea[1] = rcButtonArea;
}

void CEditWithButton::ResizeWindow()
{
	if (!::IsWindow(m_hWnd)) return;

	//proceed only if edit area is set
	if (m_rcEditArea == CRect(0,0,0,0)) return;

	if (GetWindowTextLength() == 0)
	{
		SetWindowPos(&wndTop,0,0,m_sizeEmptyBitmap.cx,m_sizeEmptyBitmap.cy,SWP_NOMOVE|SWP_NOZORDER);
	}else
	{
		SetWindowPos(&wndTop,0,0,m_sizeFilledBitmap.cx,m_sizeFilledBitmap.cy,SWP_NOMOVE|SWP_NOZORDER);
	}

	SetRect(&m_rcEditArea);

}


//set edit area may be called before creating the edit control
//especially when using the CEdit::Create method
//or after creating the edit control in CEdit::DoDataExchange
//we call ResizeWindow once in SetEditArea and once in PreSubclassWindow
BOOL CEditWithButton::SetEditArea(CRect rcEditArea)
{
	m_rcEditArea = rcEditArea;

	ResizeWindow();

	return TRUE;
}

void CEditWithButton::SetButtonClickedMessageId(UINT iButtonClickedMessageId)
{
	m_iButtonClickedMessageId = iButtonClickedMessageId;

}

void CEditWithButton::SetFirstButtonExistsAlways(BOOL bButtonExistsAlways)
{
	m_bButtonExistsAlways[0] = bButtonExistsAlways;
}

void CEditWithButton::SetSecondButtonExistsAlways(BOOL bButtonExistsAlways)
{
	m_bButtonExistsAlways[1] = bButtonExistsAlways;
}

BOOL CEditWithButton::OnEraseBkgnd(CDC* pDC)
{
	if (!pDC)
	{
		return FALSE;
	}

	// Get the size of the bitmap
	CDC dcMemory;
    CSize sizeBitmap;
	CBitmap* pOldBitmap = NULL;
	int iTextLength = GetWindowTextLength();
    
	if (iTextLength == 0)
	{
		sizeBitmap = m_sizeEmptyBitmap;
	}else
	{
		sizeBitmap = m_sizeFilledBitmap;
	}

    // Create an in-memory DC compatible with the
    // display DC we're using to paint
    dcMemory.CreateCompatibleDC(pDC);

	if (iTextLength == 0)
	{
		// Select the bitmap into the in-memory DC
		pOldBitmap = dcMemory.SelectObject(&m_bmpEmptyEdit);
	}else
	{
		// Select the bitmap into the in-memory DC
		pOldBitmap = dcMemory.SelectObject(&m_bmpFilledEdit);
	}

    // Copy the bits from the in-memory DC into the on-
    // screen DC to actually do the painting. Use the centerpoint
    // we computed for the target offset.
    pDC->BitBlt(0,0, sizeBitmap.cx, sizeBitmap.cy, &dcMemory, 
        0, 0, SRCCOPY);

	if (pOldBitmap)
	{
		dcMemory.SelectObject(pOldBitmap);
	}

	return TRUE;
}

void CEditWithButton::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	//this will draw the background again
	//so that the button will be drawn if the text exists

	InvalidateRect(NULL);

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CEditWithButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	//if the button is clicked then send message to the
	//owner.. the owner need not be parent
	//you can set the owner using the CWnd::SetOwner method
	int button = 0;
	if (m_rcButtonArea[0].PtInRect(point))
		button = 1;
	else if (m_rcButtonArea[1].PtInRect(point))
		button = 2;
	if (button)
	{
		//it is assumed that when the text is not typed in the
		//edit control, the button will not be visible
		//but you can override this by setting 
		//the m_bButtonExistsAlways to TRUE
		if ( (GetWindowTextLength() > 0) || m_bButtonExistsAlways[button-1])
		{
			CWnd *pOwner = GetOwner();
			if (pOwner)
			{
				pOwner->SendMessage(m_iButtonClickedMessageId,button-1,0);
			}
		}
	}

	CEdit::OnLButtonUp(nFlags, point);
}


//by default, when the mouse moves over the edit control
//the system shows the I-beam cursor. However we want to
//show the arrow cursor when it is over the Non-Edit area
//where the button and icon is displayed
//here is the code to do this
BOOL CEditWithButton::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CPoint pntCursor;
	GetCursorPos(&pntCursor);
	ScreenToClient(&pntCursor);
	//if mouse is not in the edit area then
	//show arrow cursor
	if (!m_rcEditArea.PtInRect(pntCursor))
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(IDC_ARROW)));
		return TRUE;
	}

	return CEdit::OnSetCursor(pWnd, nHitTest, message);
}



int CEditWithButton::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;

	ResizeWindow();

	return 0;
}


LRESULT CEditWithButton::OnSetFont( WPARAM wParam, LPARAM lParam )
{
	DefWindowProc( WM_SETFONT, wParam, lParam );

	ResizeWindow();

	return 0;

}

void CEditWithButton::OnSize( UINT nType, int cx, int cy ) 
{
	CEdit::OnSize( nType, cx, cy );

	ResizeWindow();
}
