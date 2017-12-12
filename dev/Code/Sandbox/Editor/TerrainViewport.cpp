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

#include "StdAfx.h"
#include "TerrainViewport.h"
#include "CryEditDoc.h"

#include "Terrain/TerrainManager.h"

/////////////////////////////////////////////////////////////////////////////
// CTerrainViewport

// Used to give each static object type a different color
static DWORD StatObjColorArray[] =
{
    0x00FF0000,
    0x0000FF00,
    0x000000FF,
    0x00FFFFFF,
    0x00FF00FF,
    0x00FFFF00,
    0x0000FFFF,
    0x007F00FF,
    0x007FFF7F,
    0x00FF7F00,
    0x0000FF7F,
    0x007F7F7F,
    0x00000000
};

CTerrainViewport::CTerrainViewport()
{
    m_heightmapSize.cx =    GetIEditor()->GetHeightmap()->GetWidth();
    m_heightmapSize.cy =    GetIEditor()->GetHeightmap()->GetHeight();

    VERIFY(m_dcView.CreateCompatibleDC(NULL));

    // Create a DC and a bitmap
    VERIFY(m_dcTerrain.CreateCompatibleDC(NULL));
    VERIFY(m_bmpTerrain.CreateBitmap(m_heightmapSize.cx, m_heightmapSize.cy, 1, 32, NULL));
    m_dcTerrain.SelectObject(&m_bmpTerrain);

    m_iBrushSize = 50;
}

CTerrainViewport::~CTerrainViewport()
{
}


BEGIN_MESSAGE_MAP(CTerrainViewport, MFCViewport)
//{{AFX_MSG_MAP(CTerrainViewport)
ON_WM_PAINT()
ON_WM_MOUSEWHEEL()
ON_WM_MOUSEMOVE()
ON_WM_ERASEBKGND()
ON_WM_SIZE()
ON_WM_RBUTTONDOWN()
ON_WM_RBUTTONUP()
ON_WM_LBUTTONDOWN()
ON_WM_LBUTTONUP()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTerrainViewport message handlers

//////////////////////////////////////////////////////////////////////////
POINT CTerrainViewport::WorldToView(const Vec3& wp) const
{
    CPoint p;
    float x = wp.x / 2.0f;
    float y = wp.y / 2.0f;

    float lTemp;

    // Swap the axis
    lTemp = x;
    x = y;
    y = lTemp;

    x = (x * GetZoomFactor());
    y = (y * GetZoomFactor());

    // Add the offset
    x += GetScrollOffset().x;
    y += GetScrollOffset().y;
    p.x = (int)x;
    p.y = (int)y;

    return p;
}

//////////////////////////////////////////////////////////////////////////
Vec3    CTerrainViewport::ViewToWorld(CPoint vp, bool* collideWithTerrain, bool onlyTerrain, bool bSkipVegetation, bool bTestRenderMesh) const
{
    vp.x += abs(GetScrollOffset().x);
    vp.y += abs(GetScrollOffset().y);

    // Swap the axis
    int lTemp = vp.x;
    vp.x = vp.y;
    vp.y = lTemp;

    // Scale with the zoom factor
    float x = (float)vp.x / GetZoomFactor();
    float y = (float)vp.y / GetZoomFactor();

    Vec3 wp;
    wp.x = x * 2.0f;
    wp.y = y * 2.0f;
    wp.z = GetIEditor()->GetTerrainElevation(wp.x, wp.y);

    if (collideWithTerrain)
    {
        *collideWithTerrain = true;
    }

    /*
    CEngineSingleton::GetGameInterface()->GetInterface()->
            MovePlayer(point.x * 2.0f, point.y * 2.0f, CEngineSingleton::
            GetGameInterface()->GetInterface()->GetTerrainElevation(point.x * 2.0f, point.y * 2.0f));
    */

    return wp;
}

void    CTerrainViewport::ViewToWorldRay(CPoint vp, Vec3& raySrc, Vec3& rayDir) const
{
    raySrc = ViewToWorld(vp);
    raySrc.z = 1000;
    rayDir(0, 0, -1);
}

bool CTerrainViewport::SetScrollOffset(long iX, long iY)
{
    ////////////////////////////////////////////////////////////////////////
    // Set the scrolling offset. If the passed values are invalid, the
    // nearest valid values will be set and false is returned
    ////////////////////////////////////////////////////////////////////////

    bool bRangeValid = true;
    RECT rcWndPos;
    long iScaledTexWidth = (long) (m_heightmapSize.cx * GetZoomFactor());

    // Obtain the size of the window
    GetClientRect(&rcWndPos);

    // Don't allow to scroll beyond the lower right corner
    if (abs(iX) > iScaledTexWidth - rcWndPos.right)
    {
        iX = -(iScaledTexWidth - rcWndPos.right);
        bRangeValid = false;
    }

    if (abs(iY) > iScaledTexWidth - rcWndPos.bottom)
    {
        iY = -(iScaledTexWidth - rcWndPos.bottom);
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

    RedrawWindow();

    return bRangeValid;
}


void CTerrainViewport::ResetContent()
{
    ////////////////////////////////////////////////////////////////////////
    // Create a terrain texture that consists entirely of water
    ////////////////////////////////////////////////////////////////////////
    DWORD* pWaterTexData = NULL;
    DWORD* pSurfaceTextureData = NULL;
    DWORD* pPixData = NULL, * pPixDataEnd = NULL;
    CBitmap bmpLoad;
    bool bReturn;

    // Load the water texture out of the ressource
    bReturn = bmpLoad.LoadBitmap(MAKEINTRESOURCE(IDB_WATER));
    ASSERT(bReturn);

    // Allocate new memory to hold the bitmap data
    pWaterTexData = new DWORD[128 * 128];
    ASSERT(pWaterTexData);

    // Retrieve the bits from the bitmap
    VERIFY(bmpLoad.GetBitmapBits(128 * 128 * sizeof(DWORD), pWaterTexData));


    // Allocate memory for the terrain texture
    pSurfaceTextureData = new DWORD[m_heightmapSize.cx * m_heightmapSize.cy];
    ASSERT(pSurfaceTextureData);


    // Fill the terrain texture with the water texture, tile as needed
    for (int j = 0; j < m_heightmapSize.cy; j++)
    {
        for (int i = 0; i < m_heightmapSize.cx; i++)
        {
            pSurfaceTextureData[i + j * m_heightmapSize.cx] =
                pWaterTexData[(i % 128) +  (j % 128) * 128];
        }
    }


    // Set the loop pointers
    pPixData = pSurfaceTextureData;
    pPixDataEnd = &pSurfaceTextureData[m_heightmapSize.cx * m_heightmapSize.cy];

    /*
    // Switch R and B
    while (pPixData != pPixDataEnd)
    {
        // Extract the bits, shift them, put them back and advance to the next pixel
        *pPixData++ = ((* pPixData & 0x00FF0000) >> 16) |
            (* pPixData & 0x0000FF00) | ((* pPixData & 0x000000FF) << 16);
    }
    */

    // Load it into the bitmap
    m_bmpTerrain.SetBitmapBits(m_heightmapSize.cx * m_heightmapSize.cy * 4, pSurfaceTextureData);

    if (::IsWindow(m_hWnd))
    {
        RedrawWindow();
    }

    // Free surface mem
    delete [] pSurfaceTextureData;
    pSurfaceTextureData = 0;

    // Free the water texture data
    delete [] pWaterTexData;
    pWaterTexData = 0;
}

void CTerrainViewport::UpdateContent(int flags)
{
    ////////////////////////////////////////////////////////////////////////
    // Generate a new terrain texture
    ////////////////////////////////////////////////////////////////////////
    DWORD* pSurfaceData = NULL;
    bool bReturn;

    // Allocate memory
    pSurfaceData = new DWORD[m_heightmapSize.cx * m_heightmapSize.cy];
    ASSERT(pSurfaceData);

    bReturn = GetIEditor()->GetHeightmap()->GetPreviewBitmap((DWORD*)pSurfaceData, m_heightmapSize.cx, false, false);
    /*
    // Fill in the surface data into the array. Apply lighting and water, use
    // the settings from the document
    bReturn = cSurfaceTexture.GenerateSurface(pSurfaceData, SURFACE_TEXTURE_WIDTH,
        SURFACE_TEXTURE_WIDTH, true, true, GLOBAL_GET_DOC->GetWaterLevel(), true);
        */

    if (bReturn)
    {
        m_bmpTerrain.SetBitmapBits(m_heightmapSize.cx * m_heightmapSize.cy * 4, pSurfaceData);
        if (IsWindow(GetSafeHwnd()))
        {
            RedrawWindow();
        }
    }

    // Free the surface data array
    delete [] pSurfaceData;
    pSurfaceData = 0;

    SetZoomFactor(GetZoomFactor());
}

void CTerrainViewport::SetZoomFactor(float fZoomFactor)
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
    // Obtain the dimensions of the window
    GetClientRect(&rcWndPos);

    // Is the new zoom factor smaller than the last one ?
    if (fZoomFactor < m_fZoomFactor)
    {
        // We might run into the problem that the map is smaller than the
        // the window when displayed with the specified zoom factor

        if (m_heightmapSize.cx * fZoomFactor < rcWndPos.right)
        {
            // Calculate the new size
            fZoomFactor = (float) rcWndPos.right / m_heightmapSize.cx;

            bRangeValid = false;
        }

        if (m_heightmapSize.cx * fZoomFactor < rcWndPos.bottom)
        {
            // Calculate the new size
            fNewSizeTemp = (float) rcWndPos.bottom / m_heightmapSize.cx;

            // Only set the new zoom if it is larger than the previous
            fZoomFactor = (fZoomFactor > fNewSizeTemp) ? fZoomFactor : fNewSizeTemp;

            bRangeValid = false;
        }
    }

    // Calculate how much larger the new zoom is compared to the old one
    fEnlargementFactor = fZoomFactor / m_fZoomFactor;

    // Save the new zoom factor
    m_fZoomFactor = fZoomFactor;

    if (m_cScrollOffset.x + m_heightmapSize.cx * fZoomFactor < rcWndPos.right)
    {
        m_cScrollOffset.x = rcWndPos.right - m_heightmapSize.cx * fZoomFactor;
    }

    if (m_cScrollOffset.y + m_heightmapSize.cy * fZoomFactor < rcWndPos.bottom)
    {
        m_cScrollOffset.y = rcWndPos.bottom - m_heightmapSize.cy * fZoomFactor;
    }

    if (m_cScrollOffset.x > 0)
    {
        m_cScrollOffset.x = 0;
    }
    if (m_cScrollOffset.y > 0)
    {
        m_cScrollOffset.y = 0;
    }

    RedrawWindow();
}

void CTerrainViewport::OnPaint()
{
    CPaintDC dc(this); // device context for painting

    if (!GetIEditor()->GetDocument())
    {
        return;
    }

    int iX, iY, iWidth, iHeight;

    /* OLD
    // Blit the image
    dc.SetStretchBltMode(HALFTONE);
    dc.StretchBlt(iX, iY, iWidth, iHeight, &m_dcTerrain, 0, 0, m_heightmapSize.cx, m_heightmapSize.cy, SRCCOPY);
    // Reset brush origin after strech blit.
    dc.SetBrushOrg( 0,0 );
    */

    // Free any old bitmap first

    if (m_bmpView.GetSafeHandle())
    {
        //m_dcView.SelectObject(NULL);
        ::SelectObject(m_dcView.m_hDC, NULL);
        m_bmpView.DeleteObject();
    }

    CRect rc(dc.m_ps.rcPaint);

    // Calculate the position and dimension of the image
    iX = m_cScrollOffset.x;
    iY = m_cScrollOffset.y;
    iWidth = (m_heightmapSize.cx * m_fZoomFactor);
    iHeight = (m_heightmapSize.cy * m_fZoomFactor);

    CPoint hp1 = ViewToHeightmap(CPoint(rc.left, rc.top));
    CPoint hp2 = ViewToHeightmap(CPoint(rc.right, rc.bottom));

    // Create bitmap
    BITMAPINFO BmpInfo;
    BmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    BmpInfo.bmiHeader.biWidth = hp2.x - hp1.x;
    BmpInfo.bmiHeader.biHeight = -(hp2.y - hp1.y);
    BmpInfo.bmiHeader.biPlanes = 1;
    BmpInfo.bmiHeader.biBitCount = 32;
    BmpInfo.bmiHeader.biCompression = BI_RGB;
    BmpInfo.bmiHeader.biSizeImage = 0;
    BmpInfo.bmiHeader.biXPelsPerMeter = 0;
    BmpInfo.bmiHeader.biYPelsPerMeter = 0;
    BmpInfo.bmiHeader.biClrUsed = 0;
    BmpInfo.bmiHeader.biClrImportant = 0;

    DWORD* pBits = NULL;
    // Create the DIB section to draw into
    HBITMAP bmpView = ::CreateDIBSection(m_dcView.m_hDC, &BmpInfo, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
    ASSERT(bmpView);
    m_bmpView.Attach(bmpView);

    // Select the DIB into the DC
    m_dcView.SelectObject(m_bmpView);

    ////////////////////////////////////////////////////////////////////////
    // Draw the heightmap into the final image
    ////////////////////////////////////////////////////////////////////////
    //m_dcView.BitBlt( 0,0,m_heightmapSize.cx,m_heightmapSize.cy,&m_dcTerrain, 0, 0, SRCCOPY);
    m_dcView.BitBlt(0, 0, hp2.x - hp1.x, hp2.y - hp1.y, &m_dcTerrain, hp1.x, hp1.y, SRCCOPY);
    //m_dcView.SetBrushOrg( 0,0 );

    DrawStaticObjects(pBits, hp2.x - hp1.x, hp2.y - hp1.y, CRect(hp1, hp2));

    //dc.BitBlt( 0,0,rc.right,rc.bottom,&m_dcView, 0, 0, SRCCOPY);

    dc.SetStretchBltMode(COLORONCOLOR);
    dc.StretchBlt(iX, iY, iWidth, iHeight, &m_dcView, 0, 0, hp2.x - hp1.x, hp2.y - hp1.y, SRCCOPY);
    //dc.BitBlt(0,0,rc.right,rc.bottom, &m_dcView, 0, 0, SRCCOPY);
    // Reset brush origin after strech blit.
    dc.SetBrushOrg(0, 0);

    /*
    CPoint p;
    p.x = m_viewerPos.x;
    p.y = m_viewerPos.y;
    HMCoordToWndCoord( &p );
    dc.Ellipse( p.x-3,p.y-3,p.x+3,p.y+3 );
    */
    // Do not call CViewport::OnPaint() for painting messages
}

void CTerrainViewport::DrawStaticObjects(DWORD* pBits, int trgWidth, int trgHeight, CRect rc)
{
}

BOOL CTerrainViewport::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    float prevz = GetZoomFactor();

    float z = GetZoomFactor() + (zDelta / 120.0f) * 2.0f;

    // Zoom to mouse position.
    float x = (float)(m_cMousePos.x - m_cScrollOffset.x) / prevz;
    float y = (float)(m_cMousePos.y - m_cScrollOffset.y) / prevz;
    m_cScrollOffset.x = m_cMousePos.x - z * x;
    m_cScrollOffset.y = m_cMousePos.y - z * y;

    SetZoomFactor(z);

    return TRUE;
}

void CTerrainViewport::OnMouseMove(UINT nFlags, CPoint point)
{
    m_cMousePos = point;

    if (GetViewMode() == ScrollZoomMode)
    {
        CRect rc;
        // You can only scroll while the middle or right mouse button is down
        if (nFlags & MK_RBUTTON || nFlags & MK_MBUTTON)
        {
            if (nFlags & MK_SHIFT)
            {
                // Get the dimensions of the window
                GetClientRect(&rc);

                CRect rc;
                GetClientRect(rc);
                int w = rc.right;
                int h = rc.bottom;

                // Zoom to mouse position.
                float z = m_prevZoomFactor + (point.y - m_cMouseDownPos.y) * 0.1f;
                float x = (float)(m_cMouseDownPos.x - m_prevScrollOffset.cx) / m_prevZoomFactor;
                float y = (float)(m_cMouseDownPos.y - m_prevScrollOffset.cy) / m_prevZoomFactor;
                m_cScrollOffset.x = m_cMouseDownPos.x - z * x;
                m_cScrollOffset.y = m_cMouseDownPos.y - z * y;
                SetZoomFactor(z);
            }
            else
            {
                // Set the new scrolled coordinates
                SetScrollOffset(GetScrollOffset() + (point - m_cPrevMousePos));
            }
            m_cPrevMousePos = point;
        }
        return;
    }

    // Only proceed when the mouse is over the drawing area
    if (nFlags & MK_LBUTTON)
    {
        // Calculate the coordinates in heightmap coordinates

        // Translate into drawing window coordinates
        CPoint hp = ViewToHeightmap(point);

        /*
        if (!(nFlags & MK_CONTROL))
        {
            // Add new static objects
            GetIEditor()->GetDocument()->GetStatObjMap()->DistributeObjects( hp, m_iBrushSize );
        }
        else
        {
            // Remove static objects
            GetIEditor()->GetDocument()->GetStatObjMap()->ClearObjects( hp, m_iBrushSize );
        }
        */
        /*
                // Redraw the modified area
                rcRedraw.left = point.x - m_scaledBrushSize;
                rcRedraw.top = point.y - m_scaledBrushSize;
                rcRedraw.right = point.x + m_scaledBrushSize;
                rcRedraw.bottom = point.y + m_scaledBrushSize;
                RedrawWindow(&rcRedraw, NULL, RDW_NOERASE | RDW_INVALIDATE);
                */
        RedrawWindow();
    }

    MFCViewport::OnMouseMove(nFlags, point);
}

BOOL CTerrainViewport::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

void CTerrainViewport::OnSize(UINT nType, int cx, int cy)
{
    MFCViewport::OnSize(nType, cx, cy);

    ////////////////////////////////////////////////////////////////////////
    // Re-evaluate the zoom / scroll offset values
    // TODO: Restore the zoom rectangle instead of resetting it
    ////////////////////////////////////////////////////////////////////////

    float fZoomFac1, fZoomFac2;

    fZoomFac1 = (float) cx / m_heightmapSize.cx;
    fZoomFac2 = (float) cx / m_heightmapSize.cy;

    SetZoomFactor((fZoomFac1 < fZoomFac2) ? fZoomFac1 : fZoomFac2);
}

void CTerrainViewport::OnRButtonDown(UINT nFlags, CPoint point)
{
    if (GetViewMode() != NothingMode)
    {
        return;
    }
    /*
        if (GetIEditor()->GetEditMode() == eEditModeCreate)
        {
            // Stop creation.
            GetIEditor()->GetObjectManager()->MouseCreateCallback( this,MOUSECREATE_RPOINT,point,0 );
        }
    */

    // Save the mouse down position
    m_cMouseDownPos = point;
    m_cPrevMousePos = point;

    m_prevZoomFactor = GetZoomFactor();
    m_prevScrollOffset = m_cScrollOffset;

    SetCapture();
    SetViewMode(ScrollZoomMode);

    MFCViewport::OnRButtonDown(nFlags, point);
}

void CTerrainViewport::OnRButtonUp(UINT nFlags, CPoint point)
{
    ReleaseCapture();
    SetViewMode(NothingMode);

    MFCViewport::OnRButtonUp(nFlags, point);
}

void CTerrainViewport::OnLButtonDown(UINT nFlags, CPoint point)
{
    // TODO: Add your message handler code here and/or call default

    MFCViewport::OnLButtonDown(nFlags, point);
}

void CTerrainViewport::OnLButtonUp(UINT nFlags, CPoint point)
{
    // TODO: Add your message handler code here and/or call default

    MFCViewport::OnLButtonUp(nFlags, point);
}

CPoint CTerrainViewport::ViewToHeightmap(CPoint hp)
{
    hp.x -= m_cScrollOffset.x;
    hp.y -= m_cScrollOffset.y;

    // Scale with the zoom factor
    float z1 = 1.0f / GetZoomFactor();
    int x = FloatToIntRet((float)hp.x * z1);
    int y = FloatToIntRet((float)hp.y * z1);
    if (x < 0)
    {
        x = 0;
    }
    if (y < 0)
    {
        y = 0;
    }
    if (x >= m_heightmapSize.cx)
    {
        x = m_heightmapSize.cx - 1;
    }
    if (y >= m_heightmapSize.cx)
    {
        y = m_heightmapSize.cx - 1;
    }

    return CPoint(x, y);
}

CPoint CTerrainViewport::HeightmapToView(CPoint hp)
{
    int x = FloatToIntRet(hp.x * GetZoomFactor());
    int y = FloatToIntRet(hp.y * GetZoomFactor());

    // Add the offset
    x += m_cScrollOffset.x;
    y += m_cScrollOffset.y;

    return CPoint(x, y);
}