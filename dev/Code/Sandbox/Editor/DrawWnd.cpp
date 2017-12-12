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

// Description : implementation file


#include "stdafx.h"
#include "DrawWnd.h"
#include "./Terrain/Heightmap.h"

#pragma warning (disable : 4800)

/////////////////////////////////////////////////////////////////////////////
// CDrawWnd

CDrawWnd::CDrawWnd()
{
    ////////////////////////////////////////////////////////////////////////
    // Load the brushes for drawing and create a DC for them
    ////////////////////////////////////////////////////////////////////////

    CBitmap bmpLoad;

    // Load the ressource
    VERIFY(m_bmpBrushes.LoadBitmap(IDB_BRUSH));

    // Make pink (255, 0, 255) pixels transparent
    MakeAlpha(m_bmpBrushes);

    // Create the DC
    m_dcBrushes.CreateCompatibleDC(NULL);
    m_dcBrushes.SelectObject(&m_bmpBrushes);

    // Allocate new memory to hold the water bitmap data
    m_pWaterTexData = new DWORD[128 * 128];
    ASSERT(m_pWaterTexData);

    // Load the water texture out of the ressource
    VERIFY(bmpLoad.Attach(::LoadBitmap(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDB_WATER))));

    // Retrieve the bits from the bitmap
    VERIFY(bmpLoad.GetBitmapBits(128 * 128 * sizeof(DWORD), m_pWaterTexData));

    int HX = GetIEditor()->GetHeightmap()->GetWidth();
    // Init other members
    m_iCurBrush = 2;
    m_fZoomFactor = 512.0f / HX;
    m_cScrollOffset = CPoint(0, 0);
    m_fOpacity = 16;
    m_pCoordWnd = NULL;
    m_bShowWater = true;
    m_bShowMapObj = true;
    m_fSetToHeight = -1.0f;
    m_bUseNoiseBrush = false;
    m_ptMarker = CPoint(0, 0);

    m_heightmap = GetIEditor()->GetHeightmap();
}

CDrawWnd::~CDrawWnd()
{
    if (m_pWaterTexData)
    {
        delete [] m_pWaterTexData;
        m_pWaterTexData = NULL;
    }

    if (m_pCoordWnd)
    {
        m_pCoordWnd->Detach();
    }
}

BEGIN_MESSAGE_MAP(CDrawWnd, CWnd)
//{{AFX_MSG_MAP(CDrawWnd)
ON_WM_PAINT()
ON_WM_MOUSEMOVE()
ON_WM_MOUSEWHEEL()
ON_WM_LBUTTONDOWN()
ON_WM_LBUTTONUP()
ON_WM_RBUTTONDOWN()
ON_WM_MBUTTONDOWN()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDrawWnd message handlers

void CDrawWnd::OnPaint()
{
    ////////////////////////////////////////////////////////////////////////
    // Paint the window with the heightmap
    ////////////////////////////////////////////////////////////////////////

    CPaintDC dc(this); // device context for painting
    CRect rect;
    CDC dcHeightmap;
    CBitmap bmpHeightmap;

    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
    DWORD* pImageData = NULL;
    float fScaleX, fScaleY;
    int i, j;
    float fWaterLevel = pHeightmap->GetWaterLevel();
    long iYPreCalc;
    CPoint cTransfPt;
    CPoint ptTransfMarker = m_ptMarker;


    // Get the rect of the client window
    GetClientRect(&rect);

    // Allocate memory for the bitmap data
    pImageData = new DWORD[rect.right * rect.bottom];
    ASSERT(pImageData);

    // Calculate the scaling
    fScaleX = 1.0f / m_fZoomFactor;
    fScaleY = 1.0f / m_fZoomFactor;

    int maxHX = pHeightmap->GetWidth();
    int maxHY = pHeightmap->GetHeight();

    float fHeightPrecisionScale = pHeightmap->GetBytePrecisionScale();

    // Copy the heightmap into the bitmap data
    for (j = 0; j < rect.bottom; j++)
    {
        // Precalculate for speed reasons
        iYPreCalc = ftoi((j - m_cScrollOffset.y) * fScaleY);
        if (iYPreCalc >= maxHY)
        {
            continue;
        }

        for (i = 0; i < rect.right; i++)
        {
            int iXPreCalc = ftoi((i - m_cScrollOffset.x) * fScaleX);
            if (iYPreCalc >= maxHX)
            {
                continue;
            }

            // Get the grayscale value from the heightmap array
            float h = pHeightmap->GetXY(iXPreCalc, iYPreCalc);

            // Only draw the water if the flag has been set
            if (h < fWaterLevel && m_bShowWater)
            {
                // Water, fetch a tiled texel from the water texture
                pImageData[i + j * rect.right] = m_pWaterTexData[i % 128 + ((j % 128) << 7)];
            }
            else
            {
                int iColor = ftoi(h * fHeightPrecisionScale);
                if (iColor > 255)
                {
                    iColor = 255;
                }
                // Create a ABGR color and store it in the bitmap array
                pImageData[i + j * rect.right] = (iColor << 16) | (iColor << 8) | iColor;
            }
        }
    }

    // Create DC and bitmap for the heightmap
    VERIFY(dcHeightmap.CreateCompatibleDC(&dc));
    VERIFY(bmpHeightmap.CreateBitmap(rect.right, rect.bottom, 1, 32, pImageData));
    dcHeightmap.SelectObject(&bmpHeightmap);

    // Free the temporary image data
    if (pImageData)
    {
        delete [] pImageData;
        pImageData = NULL;
    }

    /*
    if (m_bShowMapObj)
    {
        // Draw the map objects over the preview
        for (i=0; i<(long) GLOBAL_GET_DOC->m_sMapObjects.GetSize(); i++)
        {
            // Convert the position to map coordinates
            cTransfPt.x = (long) (GLOBAL_GET_DOC->m_sMapObjects[i].fX / 2.0f);
            cTransfPt.y = (long) (GLOBAL_GET_DOC->m_sMapObjects[i].fY / 2.0f);

            // Convert it to window coordinates
            HMCoordToWndCoord(&cTransfPt);

            // Calculate the rectangle that is used to draw the position of the map object
            ::SetRect(&rcObject, cTransfPt.x - 1, cTransfPt.y - 1,
                cTransfPt.x + 1, cTransfPt.y + 1);

            // Get the color of the object
            switch (GLOBAL_GET_DOC->m_sMapObjects[i].eType)
            {
                case MOEntity:
                    // Entities are red
                    dwMapObjColor = 0x000000FF;
                    break;

                case MOTagPoint:
                    // Tag points are white
                    dwMapObjColor = 0x00FFFFFF;
                    break;

                case MOBuilding:
                    // Buldings are yellow
                    dwMapObjColor = 0x0000FFFF;
                    break;

                default:
                    // Use white as default color
                    dwMapObjColor = 0x00FFFFFF;
                    break;
            }

            // Draw the rectangle
            dcHeightmap.FillRect(&rcObject, &CBrush(dwMapObjColor));
        }
    }
    */

    // Draw the current position marker
    HMCoordToWndCoord(&ptTransfMarker);
    dcHeightmap.Ellipse(ptTransfMarker.x - 3, ptTransfMarker.y - 3,
        ptTransfMarker.x + 3, ptTransfMarker.y + 3);

    // Blit the heightmap to the window
    dc.BitBlt(0, 0, rect.right, rect.bottom, &dcHeightmap, 0, 0, SRCCOPY);

    // Do not call CWnd::OnPaint() for painting messages
}

bool CDrawWnd::Create(CWnd* pwndParent)
{
    ////////////////////////////////////////////////////////////////////////
    // Use this function instead of the base class versions to create the
    // window
    ////////////////////////////////////////////////////////////////////////

    RECT rcDefault;

    // Create the window with a custom window class
    return CreateEx(NULL, AfxRegisterWndClass(CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
            AfxGetApp()->LoadCursor(IDC_HAND_INTERNAL), NULL, NULL), "DrawWindow",
        WS_CHILD | WS_VISIBLE, rcDefault, pwndParent, NULL);
}

void CDrawWnd::MakeAlpha(CBitmap& ioBM)
{
    ////////////////////////////////////////////////////////////////////////
    // Make pink pixels transparent
    ////////////////////////////////////////////////////////////////////////

    struct RGBAQUAD
    {
        BYTE rgbatBlue;
        BYTE rgbatGreen;
        BYTE rgbatRed;
        BYTE rgbatAlpha;
    };

    RGBAQUAD* pixels;

    // Figure out how many pixels there are in the bitmap
    BITMAP bmInfo;

    VERIFY(ioBM.GetBitmap (&bmInfo));

    // Add support for additional bit depths if you choose
    VERIFY(bmInfo.bmBitsPixel == 32);
    VERIFY(bmInfo.bmWidthBytes == (bmInfo.bmWidth * 4));

    if (bmInfo.bmBitsPixel != 32)
    {
        CLogFile::WriteLine("Drawing Error: Dektop is set to 16 bit, need 32 bit");
        QMessageBox:::warning(nullptr, QString(), "Please set your desktop color depth to 32 bits !");
    }

    const UINT numPixels (bmInfo.bmHeight * bmInfo.bmWidth);

    // Allocate space to receive the pixel data
    pixels = new RGBAQUAD[bmInfo.bmHeight * bmInfo.bmWidth];

    // Retrieve the pixels
    VERIFY(ioBM.GetBitmapBits(bmInfo.bmHeight * bmInfo.bmWidth * sizeof(RGBAQUAD), pixels) != 0);

    // Loop trough all pixels
    for (UINT i = 0; i < numPixels; ++i)
    {
        if (pixels[i].rgbatBlue == 255
            && pixels[i].rgbatGreen == 0
            && pixels[i].rgbatRed == 255)
        {
            pixels[i].rgbatGreen = 0;
            pixels[i].rgbatRed = 0;
            pixels[i].rgbatBlue = 0;

            // Skip pixels that have the background color
            continue;
        }
    }

    // Write the bitmap data back
    VERIFY(ioBM.SetBitmapBits(bmInfo.bmHeight * bmInfo.bmWidth * sizeof(RGBAQUAD), pixels) != 0);

    // Free the pixels
    delete [] pixels;
    pixels = 0;
}

BOOL CDrawWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    ////////////////////////////////////////////////////////////////////////
    // Mousewheel is used to control zooming
    ////////////////////////////////////////////////////////////////////////

    // Scroll in / out relative to the mouse wheel direction
    SetZoomFactor(GetZoomFactor() + (zDelta / 120.0f) * 0.15f);

    // Redraw the window
    RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

    return 0;
}

bool CDrawWnd::SetScrollOffset(long iX, long iY)
{
    ////////////////////////////////////////////////////////////////////////
    // Set the scrolling offset. If the passed values are invalid, the
    // nearest valid values will be set and false is returned
    ////////////////////////////////////////////////////////////////////////

    bool bRangeValid = true;
    RECT rcWndPos;
    long iScaledWidth = (long) (m_heightmap->GetWidth() * GetZoomFactor());

    // Obtain the size of the window
    GetClientRect(&rcWndPos);

    // Don't allow to scroll beyond the lower right corner
    if (abs(iX) > iScaledWidth - rcWndPos.right)
    {
        iX = -(iScaledWidth - rcWndPos.right);
        bRangeValid = false;
    }

    if (abs(iY) > iScaledWidth - rcWndPos.bottom)
    {
        iY = -(iScaledWidth - rcWndPos.bottom);
        bRangeValid = false;
    }

    // Don't allow to scroll beyond the upper left corner
    if (iX > 0)
    {
        iX = 0;
        bRangeValid = false;
    }

    if (iY > 0)
    {
        iY = 0;
        bRangeValid = false;
    }

    // Save the (eventually corrected) scroll offset
    m_cScrollOffset = CPoint(iX, iY);

    // Redraw the window
    RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

    return bRangeValid;
}

bool CDrawWnd::SetZoomFactor(float fZoomFactor)
{
    ////////////////////////////////////////////////////////////////////////
    // Adjust the zoom factor of the view. If the passed values are invalid,
    // the nearest valid values will be set and false is returned
    ////////////////////////////////////////////////////////////////////////

    RECT rcWndPos;
    bool bRangeValid = true;
    float fNewSizeTemp = 0;
    float fEnlargementFactor;

    // Zero can produce artifacts and errors
    if (fZoomFactor == 0.0f)
    {
        fZoomFactor = m_fZoomFactor;
        bRangeValid = false;
    }

    // Is the new zoom factor smaller than the last one ?
    if (fZoomFactor < m_fZoomFactor)
    {
        // We might run into the problem that the map is smaller than the
        // the window when displayed with the specified zoom factor

        // Obtain the dimensions of the window
        GetClientRect(&rcWndPos);

        if (m_heightmap->GetWidth() * fZoomFactor < rcWndPos.right)
        {
            // Calculate the new size
            fZoomFactor = (float) rcWndPos.right / m_heightmap->GetWidth();

            bRangeValid = false;
        }

        if (m_heightmap->GetWidth() * fZoomFactor < rcWndPos.bottom)
        {
            // Calculate the new size
            fNewSizeTemp = (float) rcWndPos.bottom / m_heightmap->GetWidth();

            // Only set the new zoom if it is larger than the previous
            fZoomFactor = (fZoomFactor > fNewSizeTemp) ? fZoomFactor : fNewSizeTemp;

            bRangeValid = false;
        }
    }

    // Calculate how much larger the new zoom is compared to the old one
    fEnlargementFactor = fZoomFactor / m_fZoomFactor;

    // Save the new zoom factor
    m_fZoomFactor = fZoomFactor;

    // Let SetScrollOffset() find a valid scroll offset
    // TODO: Scroll centering is broken
    SetScrollOffset((long) (GetScrollOffset().x * fEnlargementFactor),
        (long) (GetScrollOffset().y * fEnlargementFactor));

    return bRangeValid;
}

void CDrawWnd::OnMButtonDown(UINT nFlags, CPoint point)
{
    ////////////////////////////////////////////////////////////////////////
    // New marker position
    ////////////////////////////////////////////////////////////////////////

    m_ptMarker = point;
    WndCoordToHMCoord(&m_ptMarker);

    RedrawWindow();

    CWnd::OnMButtonDown(nFlags, point);
}

afx_msg void CDrawWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    ////////////////////////////////////////////////////////////////////////
    // User pressed the left mouse button
    ////////////////////////////////////////////////////////////////////////

    RECT rcClient;

    GetClientRect(&rcClient);

    // Save the mouse down position
    m_cMouseDownPos = point;

    // Capture mouse input for this window, we need it for the scrolling
    SetCapture();

    // Call the mouse move function to immediatly draw a spot
    OnMouseMove(nFlags, point);
}

afx_msg void CDrawWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
    ////////////////////////////////////////////////////////////////////////
    // User pressed the right mouse button
    ////////////////////////////////////////////////////////////////////////

    RECT rcClient;

    GetClientRect(&rcClient);

    // Save the mouse down position
    m_cMouseDownPos = point;

    // Capture mouse input for this window, we need it for the scrolling
    SetCapture();

    // Call the mouse move function to immediatly draw a spot
    OnMouseMove(nFlags, point);
}

void CDrawWnd::OnMouseMove(UINT nFlags, CPoint point)
{
    RECT rcWin;
    char szCoordText[256];

    if (nFlags & MK_LBUTTON || nFlags & MK_MBUTTON)
    {
        // CTRL or SHIFT indicate scrolling and zooming
        if (nFlags & MK_CONTROL || nFlags & MK_MBUTTON)
        {
            // You can only scroll while the left mouse button is down and you hold down CTRL

            // Set the new scrolled coordinates
            SetScrollOffset(GetScrollOffset() + (point - m_cMouseDownPos));
        }
        else if (nFlags & MK_SHIFT)
        {
            // You can only zoom while the left mouse button is down and you hold down SHIFT

            // Get the dimensions of the window
            GetClientRect(&rcWin);

            // Zoom
            SetZoomFactor(GetZoomFactor() + ((point.y - m_cMouseDownPos.y) / (float) rcWin.bottom));
        }
        else
        {
            // None of the modificator keys pressed, draw

            // Get the heightmap coordinates of the clicked point
            WndCoordToHMCoord(&point);

            // Draw a spot
            m_heightmap->DrawSpot(point.y, point.x,
                (uint8) (10.0f + m_iCurBrush * 25.0f), m_fOpacity,
                m_fSetToHeight, m_bUseNoiseBrush);
        }

        // Redraw the window
        RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    else if (nFlags & MK_RBUTTON)
    {
        // Get the heightmap coordinates of the clicked point
        WndCoordToHMCoord(&point);

        // Draw a spot
        m_heightmap->DrawSpot(point.y, point.x,
            (uint8) (10.0f + m_iCurBrush * 25.0f), -m_fOpacity,
            m_fSetToHeight);

        // Redraw the window
        RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }

    // Save the new mouse down position
    m_cMouseDownPos = point;

    // Display the current coordinates in the coordinate window
    if (m_pCoordWnd)
    {
        // Convert to meters
        WndCoordToHMCoord(&point);
        point.x *= 2;
        point.y *= 2;

        // Create and set the string
        sprintf_s(szCoordText, "X: %i Y: %i", point.x, point.y);
        m_pCoordWnd->SetWindowText(szCoordText);
    }

    CWnd::OnMouseMove(nFlags, point);
}

afx_msg void CDrawWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
    ////////////////////////////////////////////////////////////////////////
    // User released the left mouse button
    ////////////////////////////////////////////////////////////////////////

    // Release the restriction of the cursor
    ReleaseCapture();

    CWnd::OnLButtonUp(nFlags, point);
}

void CDrawWnd::WndCoordToHMCoord(CPoint* pWndPt)
{
    ////////////////////////////////////////////////////////////////////////
    // Transform a point from window space into map space
    ////////////////////////////////////////////////////////////////////////

    long lTemp;

    ASSERT(pWndPt);

    // Add the offset
    pWndPt->x += abs(GetScrollOffset().x);
    pWndPt->y += abs(GetScrollOffset().y);

    // Swap the axis
    lTemp = pWndPt->x;
    pWndPt->x = pWndPt->y;
    pWndPt->y = lTemp;

    // Scale with the zoom factor
    pWndPt->x = (long) (pWndPt->x / GetZoomFactor());
    pWndPt->y = (long) (pWndPt->y / GetZoomFactor());

    // Scale from map texture coordinates to heightmap coordinates
    pWndPt->x = (long) (pWndPt->x / (float) m_heightmap->GetWidth() * m_heightmap->GetWidth());
    pWndPt->y = (long) (pWndPt->y / (float) m_heightmap->GetHeight() * m_heightmap->GetHeight());
}

void CDrawWnd::HMCoordToWndCoord(CPoint* pWndPt)
{
    ////////////////////////////////////////////////////////////////////////
    // Transform a point from map space into window space
    ////////////////////////////////////////////////////////////////////////

    RECT rcWndPos;
    long lTemp;

    // Obtain the window dimensions
    GetClientRect(&rcWndPos);

    // Swap the axis
    lTemp = pWndPt->x;
    pWndPt->x = pWndPt->y;
    pWndPt->y = lTemp;

    pWndPt->x = (long) (pWndPt->x / (float) m_heightmap->GetWidth() * (m_heightmap->GetWidth() * GetZoomFactor()));
    pWndPt->y = (long) (pWndPt->y / (float) m_heightmap->GetHeight() * (m_heightmap->GetWidth() * GetZoomFactor()));

    // Add the offset
    pWndPt->x += GetScrollOffset().x;
    pWndPt->y += GetScrollOffset().y;
}

/*
void CDrawWnd::OnMouseMove(UINT nFlags, CPoint point)
{
    ////////////////////////////////////////////////////////////////////////
    // Use the current brush / rubber to draw on the heightmap
    ////////////////////////////////////////////////////////////////////////

    CPoint TransformedPoint;
    UINT iScaledBrushWidth, iScaledBrushHeight;
    BLENDFUNCTION bfBrushBlend;
    typedef BOOL (CALLBACK * ALPHABLEND) (HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
    ALPHABLEND pfnAlphaBlend = NULL;
    BOOL bSuccess;
    HINSTANCE hLib;
    RECT rcInvalid;

    // Only draw when the left mouse button is pressed
    if (!nFlags & MK_LBUTTON)
        return;

    // Transform mouse coordinates into heightmap coordinates
    TransformedPoint.x = (LONG) ((point.x / 350.0f) * GLOBAL_GET_DOC->m_cHeightmap.GetWidth());
    TransformedPoint.y = (LONG) ((point.y / 350.0f) * GLOBAL_GET_DOC->m_cHeightmap.GetHeight());

    // Calculate the scaled brush size
    iScaledBrushWidth = (UINT) ((350.0f / GLOBAL_GET_DOC->m_cHeightmap.GetWidth()) * 32);
    iScaledBrushHeight = (UINT) ((350.0f / GLOBAL_GET_DOC->m_cHeightmap.GetHeight()) * 32);

    // Scale the brush (TODO: Make this not hardcoded but selectable)
    iScaledBrushWidth *= 4;
    iScaledBrushHeight *= 4;

    // Center brush
    TransformedPoint.x -= iScaledBrushWidth / 2;
    TransformedPoint.y -= iScaledBrushHeight / 2;

    // Random offset
    TransformedPoint.x -= cry_random(-2, 2);
    TransformedPoint.y -= cry_random(-2, 2);

    // Are we in rubber mode ?
    if (m_bRubberMode)
    {
        // Just erase a rectangular area
        GLOBAL_GET_DOC->m_dcHeightmap.BitBlt(TransformedPoint.x, TransformedPoint.y, iScaledBrushWidth,
            iScaledBrushHeight, &GLOBAL_GET_DOC->m_dcHeightmap, 0, 0, BLACKNESS);

        // Update the painted part of the window
        SetRect(&rcInvalid, point.x - iScaledBrushWidth / 2,
            point.y - iScaledBrushHeight / 2, point.x + iScaledBrushWidth / 2,
            point.y + iScaledBrushHeight / 2);
        InvalidateRect(&rcInvalid);
    }
    else
    {
        // Load the imaging library
        hLib = LoadLibrary("msimg32.dll");
        assert(hLib);

        // Query the function pointer for AlphaBlend
        pfnAlphaBlend = (ALPHABLEND) GetProcAddress(hLib, "AlphaBlend");

        if (!pfnAlphaBlend)
        {
            CLogFile::WriteLine("Drawing Error: Can't find entry point for AlphaBlend() or can't load msimg32.dll at all");
            QMessageBox("Can't find entry point for AlphaBlend() or can't load msimg32.dll at all. " \
                "Upgrade to Win98 / WinME / Win2K / WinXP or install the needed DLL manually");
            assert(pfnAlphaBlend);
            return;
        }

        // Setup the blend function
        bfBrushBlend.BlendOp = AC_SRC_OVER;
        bfBrushBlend.BlendFlags = 0;
        bfBrushBlend.SourceConstantAlpha = 32;
        bfBrushBlend.AlphaFormat = AC_SRC_NO_PREMULT_ALPHA;

        // Paint the brush
        bSuccess = (* pfnAlphaBlend) (GLOBAL_GET_DOC->m_dcHeightmap.m_hDC, TransformedPoint.x,
            TransformedPoint.y, iScaledBrushWidth, iScaledBrushHeight, m_dcBrushes.m_hDC,
            m_iCurBrush * 32, 0, 32, 32, bfBrushBlend);
        assert(bSuccess);

        // Update the painted part of the window
        SetRect(&rcInvalid, point.x - iScaledBrushWidth / 2,
            point.y - iScaledBrushHeight / 2, point.x + iScaledBrushWidth / 2,
            point.y + iScaledBrushHeight / 2);
        InvalidateRect(&rcInvalid);

        // We dont need the imaging library anymore
        FreeLibrary(hLib);
    }

    // We modified the document
    GLOBAL_GET_DOC->SetModifiedFlag();

    // Need to generate all layers from scratch
    GLOBAL_GET_DOC->InvalidateLayers();

    CWnd::OnMouseMove(nFlags, point);
}
*/
