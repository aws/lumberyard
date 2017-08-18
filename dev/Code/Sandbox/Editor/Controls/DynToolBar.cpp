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

// Description : A tool bar which supports dynamic addition of buttons
// Notice      : Originally from http://code.google.com/p/cdyntoolbar/

#include "stdafx.h"
#include "DynToolBar.h"

/////////////////////////////////////////////////////////////////////////////
// CDynToolBar

CDynToolBar::CDynToolBar()
{
}

CDynToolBar::~CDynToolBar()
{
}


BEGIN_MESSAGE_MAP(CDynToolBar, CToolBar)
//{{AFX_MSG_MAP(CDynToolBar)
// NOTE - the ClassWizard will add and remove mapping macros here.
ON_NOTIFY_REFLECT(TBN_GETBUTTONINFO, OnToolBarGetButtonInfo)
ON_NOTIFY_REFLECT(TBN_BEGINADJUST, OnToolBarBeginAdjust)
ON_NOTIFY_REFLECT(TBN_ENDADJUST, OnToolBarEndAdjust)
ON_NOTIFY_REFLECT(TBN_QUERYDELETE, OnToolBarQueryDelete)
ON_NOTIFY_REFLECT(TBN_QUERYINSERT, OnToolBarQueryInsert)
ON_NOTIFY_REFLECT(TBN_TOOLBARCHANGE, OnToolBarChange)
ON_NOTIFY_REFLECT(TBN_RESET, OnToolBarReset)
ON_NOTIFY_REFLECT(TBN_INITCUSTOMIZE, OnInitCustomize)
ON_WM_CONTEXTMENU()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

bool CDynToolBar::AddSeparator()
{
    return AddButton(0, (HICON)NULL);
}

bool CDynToolBar::AddButton(int nIdCommand, HICON hIcon)
{
    TBBUTTON tbb = {0};
    tbb.idCommand = nIdCommand;
    tbb.iString = -1;   // Text drawn next to the button
    tbb.fsState = TBSTATE_ENABLED;
    tbb.fsStyle = nIdCommand == 0 ? TBSTYLE_SEP : TBSTYLE_BUTTON;
    return AddButton(tbb, hIcon);
}

bool CDynToolBar::AddButton(int nIdCommand, UINT nIDResource)
{
    //load the icon from the resources
    if (nIDResource != NULL)
    {
        HINSTANCE hInst =
            AfxFindResourceHandle(MAKEINTRESOURCE(nIDResource), RT_GROUP_ICON);
        HICON hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(nIDResource),
                IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        return AddButton(nIdCommand, hIcon);
    }
    else
    {
        return AddButton(nIdCommand, (HICON)NULL);
    }
}

bool CDynToolBar::AddButton(int nIdCommand, LPCTSTR lpszResourceName)
{
    //load the icon from the resources
    if (lpszResourceName != NULL)
    {
        HINSTANCE hInst =
            AfxFindResourceHandle(MAKEINTRESOURCE(lpszResourceName), RT_GROUP_ICON);
        HICON hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(lpszResourceName),
                IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        return AddButton(nIdCommand, hIcon);
    }
    else
    {
        return AddButton(nIdCommand, (HICON)NULL);
    }
}

namespace
{
    // http://www.codeproject.com/KB/graphics/Create_GrayscaleIcon.aspx
    HICON CreateGrayscaleIcon(HICON hIcon, COLORREF* pPalette)
    {
        if (hIcon == NULL)
        {
            return NULL;
        }

        HDC hdc = ::GetDC(NULL);

        HICON      hGrayIcon      = NULL;
        ICONINFO   icInfo         = { 0 };
        ICONINFO   icGrayInfo     = { 0 };
        LPDWORD    lpBits         = NULL;
        LPBYTE     lpBitsPtr      = NULL;
        SIZE sz;
        DWORD c1 = 0;
        BITMAPINFO bmpInfo        = { 0 };
        bmpInfo.bmiHeader.biSize  = sizeof(BITMAPINFOHEADER);

        if (::GetIconInfo(hIcon, &icInfo))
        {
            if (::GetDIBits(hdc, icInfo.hbmColor, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS) != 0)
            {
                bmpInfo.bmiHeader.biCompression = BI_RGB;
                bmpInfo.bmiHeader.biPlanes = 1;
                bmpInfo.bmiHeader.biBitCount = 32;
                bmpInfo.bmiHeader.biSizeImage = bmpInfo.bmiHeader.biWidth * 4 * bmpInfo.bmiHeader.biHeight;
                bmpInfo.bmiHeader.biClrUsed = 0;
                bmpInfo.bmiHeader.biClrImportant = 0;

                const int size = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * bmpInfo.bmiHeader.biClrUsed;
                BITMAPINFO* pBmpInfo = (BITMAPINFO*)new BYTE[size];
                memset (pBmpInfo, 0, size);
                pBmpInfo->bmiHeader = bmpInfo.bmiHeader;
                pBmpInfo->bmiHeader.biCompression = BI_RGB;

                sz.cx = bmpInfo.bmiHeader.biWidth;
                sz.cy = bmpInfo.bmiHeader.biHeight;
                c1 = sz.cx * sz.cy;

                lpBits = (LPDWORD)::GlobalAlloc(GMEM_FIXED, (c1) * 4);

                if (lpBits && ::GetDIBits(hdc, icInfo.hbmColor, 0, bmpInfo.bmiHeader.biHeight, lpBits, pBmpInfo, DIB_RGB_COLORS) != 0)
                {
                    lpBitsPtr     = (LPBYTE)lpBits;
                    UINT off      = 0;

                    for (UINT i = 0; i < c1; i++)
                    {
                        off = (UINT)(255 - ((lpBitsPtr[0] + lpBitsPtr[1] + lpBitsPtr[2]) / 3));

                        if (lpBitsPtr[3] != 0 || off != 255)
                        {
                            if (off == 0)
                            {
                                off = 1;
                            }

                            lpBits[i] = pPalette[off] | (lpBitsPtr[3] << 24);
                        }

                        lpBitsPtr += 4;
                    }

                    icGrayInfo.hbmColor = ::CreateCompatibleBitmap(hdc, sz.cx, sz.cy);

                    if (icGrayInfo.hbmColor != NULL)
                    {
                        ::SetDIBits(hdc, icGrayInfo.hbmColor, 0, sz.cy, lpBits, &bmpInfo, DIB_RGB_COLORS);

                        icGrayInfo.hbmMask = icInfo.hbmMask;
                        icGrayInfo.fIcon   = TRUE;

                        hGrayIcon = ::CreateIconIndirect(&icGrayInfo);

                        ::DeleteObject(icGrayInfo.hbmColor);
                    }

                    ::GlobalFree(lpBits);
                    lpBits = NULL;
                }
            }

            ::DeleteObject(icInfo.hbmColor);
            ::DeleteObject(icInfo.hbmMask);
        }

        ::ReleaseDC(NULL, hdc);

        return hGrayIcon;
    }

    HICON CreateGrayscaleIcon(HICON hIcon)
    {
        if (hIcon == NULL)
        {
            return NULL;
        }

        COLORREF defaultGrayPalette[256];
        BOOL bGrayPaletteSet = FALSE;
        if (!bGrayPaletteSet)
        {
            for (int i = 0; i < 256; i++)
            {
                defaultGrayPalette[i] = RGB(255 - i, 255 - i, 255 - i);
            }

            bGrayPaletteSet = TRUE;
        }

        return CreateGrayscaleIcon(hIcon, defaultGrayPalette);
    }
}

bool CDynToolBar::AddButton(TBBUTTON& tbb, HICON hIcon)
{
    const int iconSize = 16;
    if (hIcon != 0)
    {
        // Add the icon to the imagelist's
        if (!m_ImageList)
        {
            SetSizes(CSize(iconSize + 7, iconSize + 8), CSize(iconSize, iconSize));

            m_ImageList.Create(iconSize, iconSize, ILC_COLOR32 | ILC_MASK, 0, 0);
            GetToolBarCtrl().SetImageList(&m_ImageList);

            m_DisabledImageList.Create(iconSize, iconSize, ILC_COLOR32 | ILC_MASK, 0, 0);
            GetToolBarCtrl().SetDisabledImageList(&m_DisabledImageList);
        }

        tbb.iBitmap = m_ImageList.Add(hIcon);
        HICON hGSIcon = CreateGrayscaleIcon(hIcon);
        m_DisabledImageList.Add(hGSIcon);
        DestroyIcon(hGSIcon);
    }
    else
    {
        tbb.iBitmap = I_IMAGENONE;
    }

    return GetToolBarCtrl().AddButtons(1, &tbb) != 0;
}

/////////////////////////////////////////////////////////////////////////////
// CDynToolBar message handlers
#if defined(_WIN64)
INT_PTR CDynToolBar::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
#else
int CDynToolBar::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
#endif
{
    ASSERT_VALID(this);
    ASSERT(::IsWindow(m_hWnd));

    // check child windows first by calling CControlBar
#if defined(_WIN64)
    INT_PTR nHit = (int)CControlBar::OnToolHitTest(point, pTI);
#else
    int nHit = (int)CControlBar::OnToolHitTest(point, pTI);
#endif
    if (nHit != -1)
    {
        return nHit;
    }

    // now hit test against CToolBar buttons
    int nButtons = GetToolBarCtrl().GetButtonCount();
    for (int i = 0; i < nButtons; i++)
    {
        CRect rect;
        TBBUTTON button;
        if (GetToolBarCtrl().GetItemRect(i, &rect))
        {
            if (rect.PtInRect(point) &&
                GetToolBarCtrl().GetButton(i, &button) &&
                !(button.fsStyle & TBSTYLE_SEP))
            {
                int nHit = (int)GetItemID(i);
                if (pTI != NULL && pTI->cbSize >= 40 /*sizeof(AFX_OLDTOOLINFO)*/)
                {
                    pTI->hwnd = m_hWnd;
                    pTI->rect = rect;
                    pTI->uId = nHit;
                    pTI->lpszText = LPSTR_TEXTCALLBACK;
                }
                // found matching rect, return the ID of the button
                return nHit != 0 ? nHit : -1;
            }
        }
    }
    return -1;
}

void CDynToolBar::OnToolBarGetButtonInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
    TBNOTIFY* pTBntf = (TBNOTIFY*)pNMHDR;

    if ((pTBntf->iItem >= 0) && (pTBntf->iItem < GetToolBarCtrl().GetButtonCount()))
    {
        TBBUTTON button = {0};
        GetToolBarCtrl().GetButton(pTBntf->iItem, &button);
        pTBntf->tbButton = button;
        CString str;
        str.LoadString(GetItemID(pTBntf->iItem));
        strcpy(pTBntf->pszText, str);

        *pResult = TRUE;
    }
    else
    {
        *pResult = FALSE;
    }
}

void CDynToolBar::OnContextMenu(CWnd* pWnd, CPoint point)
{
    CMenu menu;
    VERIFY(menu.CreatePopupMenu());
    menu.AppendMenu(MF_STRING, 1, "Customize...");
    int nResult = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, this);
    if (nResult == 1)
    {
        GetToolBarCtrl().Customize();
    }
}

void CDynToolBar::OnCustomize()
{
    GetToolBarCtrl().Customize();
}

void CDynToolBar::OnToolBarQueryDelete(NMHDR* pNMHDR, LRESULT* pResult)
{
    TBNOTIFY* pTBntf = (TBNOTIFY*)pNMHDR;

    // do not allow hidden button to be deleted as they just do not go
    // to the Add listbox.
    if ((pTBntf->tbButton.idCommand) &&
        GetToolBarCtrl().IsButtonHidden(pTBntf->tbButton.idCommand))
    {
        *pResult = FALSE;
    }
    else
    {
        *pResult = TRUE;
    }
}

void CDynToolBar::OnToolBarQueryInsert(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = TRUE;
}

void CDynToolBar::OnToolBarChange(NMHDR* pNMHDR, LRESULT* pResult)
{
    GetParentFrame()->RecalcLayout();
}

void CDynToolBar::OnToolBarEndAdjust(NMHDR* pNMHDR, LRESULT* pResult)
{
}

void CDynToolBar::OnToolBarBeginAdjust(NMHDR* pNMHDR, LRESULT* pResult)
{
}

void CDynToolBar::OnToolBarReset(NMHDR* pNMHDR, LRESULT* pResult)
{
}

void CDynToolBar::OnInitCustomize(NMHDR* pNMHDR, LRESULT* pResult)
{
    // Hide the help button in the customize dialog
    *pResult = TBNRF_HIDEHELP;
}

void CDynToolBar::ClearButtons()
{
    while (GetToolBarCtrl().GetButtonCount() > 0)
    {
        GetToolBarCtrl().DeleteButton(0);
    }

    m_ImageList.DeleteImageList();
    m_DisabledImageList.DeleteImageList();
}