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
#include "TopRendererWnd.h"
#include "./Terrain/Heightmap.h"
#include "VegetationMap.h"
#include "DisplaySettings.h"
#include "ViewManager.h"

#include "./Terrain/TerrainTexGen.h"
#include "TerrainModifyTool.h"

// Size of the surface texture
#define SURFACE_TEXTURE_WIDTH 512

#define MARKER_SIZE 6.0f
#define MARKER_DIR_SIZE 10.0f
#define SELECTION_RADIUS 30.0f

#define GL_RGBA 0x1908
#define GL_BGRA 0x80E1

// Used to give each static object type a different color
static uint32 sVegetationColors[16] =
{
    0xFFFF0000,
    0xFF00FF00,
    0xFF0000FF,
    0xFFFFFFFF,
    0xFFFF00FF,
    0xFFFFFF00,
    0xFF00FFFF,
    0xFF7F00FF,
    0xFF7FFF7F,
    0xFFFF7F00,
    0xFF00FF7F,
    0xFF7F7F7F,
    0xFFFF0000,
    0xFF00FF00,
    0xFF0000FF,
    0xFFFFFFFF,
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

QTopRendererWnd::QTopRendererWnd(QWidget* parent)
    : Q2DViewport(parent)
{
    ////////////////////////////////////////////////////////////////////////
    // Set the window type member of the base class to the correct type and
    // create the initial surface texture
    ////////////////////////////////////////////////////////////////////////

    if (gSettings.viewports.bTopMapSwapXY)
    {
        SetAxis(VPA_YX);
    }
    else
    {
        SetAxis(VPA_XY);
    }

    m_bShowHeightmap = false;
    m_bShowStatObjects = false;
    m_bLastShowHeightmapState = false;

    ////////////////////////////////////////////////////////////////////////
    // Surface texture
    ////////////////////////////////////////////////////////////////////////

    m_textureSize.setWidth(gSettings.viewports.nTopMapTextureResolution);
    m_textureSize.setHeight(gSettings.viewports.nTopMapTextureResolution);

    m_heightmapSize = QSize(1, 1);

    m_terrainTextureId = 0;

    m_vegetationTextureId = 0;
    m_bFirstTerrainUpdate = true;
    m_bShowWater = false;

    // Create a new surface texture image
    ResetSurfaceTexture();

    m_bContentsUpdated = false;

    m_gridAlpha = 0.3f;
    m_colorGridText = QColor(255, 255, 255);
    m_colorAxisText = QColor(255, 255, 255);
    m_colorBackground = QColor(128, 128, 128);

    //For this viewport 250 is a better max zoom. 
    //Anything more than that and the viewport is too small to actually 
    //paint a heightmap outside of a very high res 4K+ monitor.
    m_maxZoom = 250.0f;
}

//////////////////////////////////////////////////////////////////////////
QTopRendererWnd::~QTopRendererWnd()
{
    ////////////////////////////////////////////////////////////////////////
    // Destroy the attached render and free the surface texture
    ////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////////////////////
void QTopRendererWnd::SetType(EViewportType type)
{
    m_viewType = type;
    m_axis = VPA_YX;
    SetAxis(m_axis);
}

//////////////////////////////////////////////////////////////////////////
void QTopRendererWnd::ResetContent()
{
    Q2DViewport::ResetContent();
    ResetSurfaceTexture();

    // Reset texture ids.
    m_terrainTextureId = 0;
    m_vegetationTextureId = 0;
}

//////////////////////////////////////////////////////////////////////////
void QTopRendererWnd::UpdateContent(int flags)
{
    if (gSettings.viewports.bTopMapSwapXY)
    {
        SetAxis(VPA_YX);
    }
    else
    {
        SetAxis(VPA_XY);
    }

    Q2DViewport::UpdateContent(flags);
    if (!GetIEditor()->GetDocument())
    {
        return;
    }

    CHeightmap* heightmap = GetIEditor()->GetHeightmap();
    if (!heightmap)
    {
        return;
    }

    if (!isVisible())
    {
        return;
    }

    m_heightmapSize.setWidth(heightmap->GetWidth() * heightmap->GetUnitSize());
    m_heightmapSize.setHeight(heightmap->GetHeight() * heightmap->GetUnitSize());

    UpdateSurfaceTexture(flags);
    m_bContentsUpdated = true;

    // if first update.
    if (m_bFirstTerrainUpdate)
    {
        InitHeightmapAlignment();
    }
    m_bFirstTerrainUpdate = false;
}

//////////////////////////////////////////////////////////////////////////
void QTopRendererWnd::InitHeightmapAlignment()
{
    CHeightmap* heightmap = GetIEditor()->GetHeightmap();
    if (heightmap)
    {
        SSectorInfo si;
        heightmap->GetSectorsInfo(si);
        float sizeMeters = si.numSectors * si.sectorSize;
        float mid = sizeMeters / 2;
        if (sizeMeters != 0.f)
        {
            SetZoom((0.95f / sizeMeters) * m_rcClient.width(), QPoint(m_rcClient.width() / 2, m_rcClient.height() / 2));
            SetScrollOffset(-10, -10);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void QTopRendererWnd::UpdateSurfaceTexture(int flags)
{
    ////////////////////////////////////////////////////////////////////////
    // Generate a new surface texture
    ////////////////////////////////////////////////////////////////////////
    if (flags & eUpdateHeightmap)
    {
        bool bShowHeightmap = m_bShowHeightmap;

        CEditTool* pTool = GetEditTool();
        if (pTool && qobject_cast<CTerrainModifyTool*>(pTool))
        {
            bShowHeightmap = true;
        }

        bool bRedrawFullTexture = m_bLastShowHeightmapState != bShowHeightmap;

        m_bLastShowHeightmapState = bShowHeightmap;
        if (!bShowHeightmap)
        {
            m_textureSize.setWidth(gSettings.viewports.nTopMapTextureResolution);
            m_textureSize.setHeight(gSettings.viewports.nTopMapTextureResolution);

            m_terrainTexture.Allocate(m_textureSize.width(), m_textureSize.height());

            int flags = ETTG_LIGHTING | ETTG_FAST_LLIGHTING | ETTG_BAKELIGHTING;
            if (m_bShowWater)
            {
                flags |= ETTG_SHOW_WATER;
            }
            // Fill in the surface data into the array. Apply lighting and water, use
            // the settings from the document
            CTerrainTexGen texGen;
            texGen.GenerateSurfaceTexture(flags, m_terrainTexture);
        }
        else
        {
            int cx = gSettings.viewports.nTopMapTextureResolution;
            int cy = gSettings.viewports.nTopMapTextureResolution;
            if (m_textureSize != QSize(cx, cy) || !m_terrainTexture.GetData())
            {
                m_textureSize = QSize(cx, cy);
                m_terrainTexture.Allocate(m_textureSize.width(), m_textureSize.height());
            }

            AABB box = GetIEditor()->GetViewManager()->GetUpdateRegion();
            QRect updateRect = GetIEditor()->GetHeightmap()->WorldBoundsToRect(box);
            QRect* pUpdateRect = &updateRect;
            if (bRedrawFullTexture)
            {
                pUpdateRect = 0;
            }

            GetIEditor()->GetHeightmap()->GetPreviewBitmap((DWORD*)m_terrainTexture.GetData(), m_textureSize.width(), false, false, pUpdateRect, m_bShowWater, m_bAutoScaleGreyRange);
        }
    }

    if (flags == eUpdateStatObj)
    {
        // If The only update flag is Update of static objects, display them.
        m_bShowStatObjects = true;
    }

    if (flags & eUpdateStatObj)
    {
        if (m_bShowStatObjects)
        {
            DrawStaticObjects();
        }
    }
    m_bContentValid = false;
}

//////////////////////////////////////////////////////////////////////////
void QTopRendererWnd::DrawStaticObjects()
{
    if (!m_bShowStatObjects)
    {
        return;
    }

    CVegetationMap* vegetationMap = GetIEditor()->GetVegetationMap();
    int srcW = vegetationMap->GetNumSectors();
    int srcH = vegetationMap->GetNumSectors();

    QRect rc = QRect(0, 0, srcW, srcH);
    AABB updateRegion = GetIEditor()->GetViewManager()->GetUpdateRegion();
    if (updateRegion.min.x > -10000)
    {
        // Update region valid.
        const QPoint p1 = QPoint(vegetationMap->WorldToSector(updateRegion.min.y), vegetationMap->WorldToSector(updateRegion.min.x));
        const QPoint p2 = QPoint(vegetationMap->WorldToSector(updateRegion.max.y), vegetationMap->WorldToSector(updateRegion.max.x));
        const QRect urc = QRect(p1, p2).adjusted(-1, -1, 1, 1);
        rc &= urc;
    }

    int trgW = rc.right() - rc.left();
    int trgH = rc.bottom() - rc.top();

    if (trgW <= 0 || trgH <= 0)
    {
        return;
    }

    m_vegetationTexturePos = rc.topLeft();
    m_vegetationTextureSize = QSize(srcW, srcH);
    m_vegetationTexture.Allocate(trgW, trgH);

    uint32* trg = m_vegetationTexture.GetData();
    vegetationMap->DrawToTexture(trg, trgW, trgH, rc.left(), rc.top());
}

//////////////////////////////////////////////////////////////////////////
void QTopRendererWnd::ResetSurfaceTexture()
{
    ////////////////////////////////////////////////////////////////////////
    // Create a surface texture that consists entirely of water
    ////////////////////////////////////////////////////////////////////////

    unsigned int i, j;

    // Load the water texture out of the ressource
    QImage waterImage(":/water.png");
    assert(!waterImage.isNull());

    // Allocate memory for the surface texture
    m_terrainTexture.Allocate(m_textureSize.width(), m_textureSize.height());

    // Fill the surface texture with the water texture, tile as needed
    for (j = 0; j < m_textureSize.height(); j++)
    {
        for (i = 0; i < m_textureSize.width(); i++)
        {
            m_terrainTexture.ValueAt(i, j) = waterImage.pixel(i & 127, j & 127) & 0x00ffffff; // no alpha-channel wanted
        }
    }

    m_bContentValid = false;
}

//////////////////////////////////////////////////////////////////////////
void QTopRendererWnd::Draw(DisplayContext& dc)
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    ////////////////////////////////////////////////////////////////////////
    // Perform the rendering for this window
    ////////////////////////////////////////////////////////////////////////
    if (!m_bContentsUpdated)
    {
        UpdateContent(0xFFFFFFFF);
    }

    ////////////////////////////////////////////////////////////////////////
    // Render the 2D map
    ////////////////////////////////////////////////////////////////////////
    if (!m_terrainTextureId)
    {
        //GL_BGRA_EXT
        if (m_terrainTexture.IsValid())
        {
            m_terrainTextureId = m_renderer->DownLoadToVideoMemory((unsigned char*)m_terrainTexture.GetData(), m_textureSize.width(), m_textureSize.height(), eTF_R8G8B8A8, eTF_R8G8B8A8, 0, 0, 0);
        }
    }

    if (m_terrainTextureId && m_terrainTexture.IsValid())
    {
        m_renderer->UpdateTextureInVideoMemory(m_terrainTextureId, (unsigned char*)m_terrainTexture.GetData(), 0, 0, m_textureSize.width(), m_textureSize.height(), eTF_R8G8B8A8);
    }

    if (m_bShowStatObjects)
    {
        if (m_vegetationTexture.IsValid())
        {
            int w = m_vegetationTexture.GetWidth();
            int h = m_vegetationTexture.GetHeight();
            uint32* tex = m_vegetationTexture.GetData();
            if (!m_vegetationTextureId)
            {
                m_vegetationTextureId = m_renderer->DownLoadToVideoMemory((unsigned char*)tex, w, h, eTF_R8G8B8A8, eTF_R8G8B8A8, 0, 0, FILTER_NONE);
            }
            else
            {
                int px = m_vegetationTexturePos.x();
                int py = m_vegetationTexturePos.y();
                m_renderer->UpdateTextureInVideoMemory(m_vegetationTextureId, (unsigned char*)tex, px, py, w, h, eTF_R8G8B8A8);
            }
            m_vegetationTexture.Release();
        }
    }


    // Reset states
    m_renderer->ResetToDefault();

    dc.DepthTestOff();

    Matrix34 tm = GetScreenTM();

    if (m_axis == VPA_YX)
    {
        float s[4], t[4];

        s[0] = 0;
        t[0] = 0;
        s[1] = 1;
        t[1] = 0;
        s[2] = 1;
        t[2] = 1;
        s[3] = 0;
        t[3] = 1;
        m_renderer->DrawImageWithUV(tm.m03, tm.m13, 0, tm.m01 * m_heightmapSize.width(), tm.m10 * m_heightmapSize.height(), m_terrainTextureId, s, t);
    }
    else
    {
        float s[4], t[4];
        s[0] = 0;
        t[0] = 0;
        s[1] = 0;
        t[1] = 1;
        s[2] = 1;
        t[2] = 1;
        s[3] = 1;
        t[3] = 0;
        m_renderer->DrawImageWithUV(tm.m03, tm.m13, 0, tm.m00 * m_heightmapSize.width(), tm.m11 * m_heightmapSize.height(), m_terrainTextureId, s, t);
    }

    dc.DepthTestOn();

    Q2DViewport::Draw(dc);
}

//////////////////////////////////////////////////////////////////////////
Vec3    QTopRendererWnd::ViewToWorld(const QPoint& vp, bool* collideWithTerrain, bool onlyTerrain, bool bSkipVegetation, bool bTestRenderMesh, bool* collideWithObject) const
{
    Vec3 wp = Q2DViewport::ViewToWorld(vp, collideWithTerrain, onlyTerrain, bSkipVegetation, bTestRenderMesh, collideWithObject);
    wp.z = GetIEditor()->GetTerrainElevation(wp.x, wp.y);
    return wp;
}

#include <TopRendererWnd.moc>
