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
#include "TerrainTexturePainter.h"
#include "Viewport.h"
#include "Objects/DisplayContext.h"

#include "TerrainPainterPanel.h"
#include "MainWindow.h"

#include "CryEditDoc.h"
#include "Terrain/Layer.h"

#include "Util/ImagePainter.h"

#include <I3DEngine.h>
#include <ITerrain.h>

#include "Terrain/TerrainManager.h"
#include "Terrain/SurfaceType.h"
#include "Terrain/Heightmap.h"

#include "Util/BoostPythonHelpers.h"

#ifdef AZ_PLATFORM_WINDOWS
#include <InitGuid.h>
#endif

#include <AzToolsFramework/Commands/LegacyCommand.h>

// {C3EE67BE-F167-4E48-93B6-47D184DC06F8}
DEFINE_GUID(TERRAIN_PAINTER_TOOL_GUID, 0xc3ee67be, 0xf167, 0x4e48, 0x93, 0xb6, 0x47, 0xd1, 0x84, 0xdc, 0x6, 0xf8);



struct CUndoTPSector
{
    CUndoTPSector()
    {
        m_img = 0;
        m_imgRedo = 0;
    }

    QPoint tile;
    int x, y;
    int w;
    uint32 dwSize;
    CImageEx* m_img;
    CImageEx* m_imgRedo;
};


struct CUndoTPLayerIdSector
{
    CUndoTPLayerIdSector()
    {
        m_img = 0;
        m_imgRedo = 0;
    }

    int x, y;
    Weightmap* m_img;
    Weightmap* m_imgRedo;
};

struct CUndoTPElement
{
    std::vector <CUndoTPSector> sects;
    std::vector <CUndoTPLayerIdSector> layerIds;

    ~CUndoTPElement()
    {
        Clear();
    }

    void AddSector(float fpx, float fpy, float radius)
    {
        CHeightmap* heightmap = GetIEditor()->GetHeightmap();

        CRGBLayer* rgbLayer = GetIEditor()->GetTerrainManager()->GetRGBLayer();
        uint32 dwMaxRes = rgbLayer->CalcMaxLocalResolution(0, 0, 1, 1);

        uint32 dwTileCountX = rgbLayer->GetTileCountX();
        uint32 dwTileCountY = rgbLayer->GetTileCountY();

        float   gx1 = fpx - radius - 2.0f / dwMaxRes;
        float   gx2 = fpx + radius + 2.0f / dwMaxRes;
        float   gy1 = fpy - radius - 2.0f / dwMaxRes;
        float gy2 = fpy + radius + 2.0f / dwMaxRes;

        // Make sure we stay within valid ranges.
        gx1 = clamp_tpl(gx1, 0.0f, 1.0f);
        gy1 = clamp_tpl(gy1, 0.0f, 1.0f);
        gx2 = clamp_tpl(gx2, 0.0f, 1.0f);
        gy2 = clamp_tpl(gy2, 0.0f, 1.0f);

        QRect recTiles = QRect(
                QPoint((uint32)floor(gx1 * dwTileCountX),
                (uint32)floor(gy1 * dwTileCountY)),
                QPoint((uint32)ceil(gx2 * dwTileCountX),
                (uint32)ceil(gy2 * dwTileCountY)));

        for (uint32 dwTileY = recTiles.top(); dwTileY < recTiles.bottom(); ++dwTileY)
        {
            for (uint32 dwTileX = recTiles.left(); dwTileX < recTiles.right(); ++dwTileX)
            {
                uint32 dwLocalSize = rgbLayer->GetTileResolution(dwTileX, dwTileY);
                uint32 dwSize = rgbLayer->GetTileResolution(dwTileX, dwTileY) * rgbLayer->GetTileCountX();

                uint32 gwid = dwSize * radius * 4;
                if (gwid < 32)
                {
                    gwid = 32;
                }
                else if (gwid < 64)
                {
                    gwid = 64;
                }
                else if (gwid < 128)
                {
                    gwid = 128;
                }
                else
                {
                    gwid = 256;
                }

                float   x1 = gx1;
                float   x2 = gx2;
                float   y1 = gy1;
                float y2 = gy2;

                if (x1 < (float)dwTileX / dwTileCountX)
                {
                    x1 = (float)dwTileX / dwTileCountX;
                }
                if (y1 < (float)dwTileY / dwTileCountY)
                {
                    y1 = (float)dwTileY / dwTileCountY;
                }
                if (x2 > (float)(dwTileX + 1) / dwTileCountX)
                {
                    x2 = (float)(dwTileX + 1) / dwTileCountX;
                }
                if (y2 > (float)(dwTileY + 1) / dwTileCountY)
                {
                    y2 = (float)(dwTileY + 1) / dwTileCountY;
                }

                uint32 wid = gwid;

                if (wid > dwLocalSize)
                {
                    wid = dwLocalSize;
                }

                QRect recSects = QRect(
                        QPoint(((int)floor(x1 * dwSize / wid)) * wid,
                        ((int)floor(y1 * dwSize / wid)) * wid),
                        QPoint(((int)ceil(x2 * dwSize / wid)) * wid,
                        ((int)ceil(y2 * dwSize / wid)) * wid));


                for (uint32 sy = recSects.top(); sy < recSects.bottom(); sy += wid)
                {
                    for (uint32 sx = recSects.left(); sx < recSects.right(); sx += wid)
                    {
                        bool bFind = false;
                        for (int i = sects.size() - 1; i >= 0; i--)
                        {
                            CUndoTPSector* sect = &sects[i];
                            if (sect->tile.x() == dwTileX && sect->tile.y() == dwTileY)
                            {
                                if (sect->x == sx &&  sect->y == sy)
                                {
                                    bFind = true;
                                    break;
                                }
                            }
                        }
                        if (!bFind)
                        {
                            CUndoTPSector newSect;
                            newSect.x = sx;
                            newSect.y = sy;
                            newSect.w = wid;
                            newSect.tile.rx() = dwTileX;
                            newSect.tile.ry() = dwTileY;
                            newSect.dwSize = dwSize;

                            newSect.m_img = new CImageEx;
                            newSect.m_img->Allocate(newSect.w, newSect.w);

                            CUndoTPSector* pSect = &newSect;

                            rgbLayer->GetSubImageStretched((float)pSect->x / pSect->dwSize, (float)pSect->y / pSect->dwSize,
                                (float)(pSect->x + pSect->w) / pSect->dwSize, (float)(pSect->y + pSect->w) / pSect->dwSize, *pSect->m_img);

                            sects.push_back(newSect);
                        }
                    }
                }
            }
        }

        // Store LayerIDs
        const uint32 layerSize = 64;

        QRect reclids = QRect(
                QPoint((uint32)floor(gx1 * heightmap->GetWidth() / layerSize),
                (uint32)floor(gy1 * heightmap->GetHeight() / layerSize)),
                QPoint((uint32)ceil(gx2 * heightmap->GetWidth() / layerSize),
                (uint32)ceil(gy2 * heightmap->GetHeight() / layerSize)));

        for (uint32 ly = reclids.top(); ly < reclids.bottom(); ly++)
        {
            for (uint32 lx = reclids.left(); lx < reclids.right(); lx++)
            {
                bool bFind = false;
                for (int i = layerIds.size() - 1; i >= 0; i--)
                {
                    CUndoTPLayerIdSector* sect = &layerIds[i];
                    if (sect->x == lx &&  sect->y == ly)
                    {
                        bFind = true;
                        break;
                    }
                }
                if (!bFind)
                {
                    CUndoTPLayerIdSector newSect;
                    CUndoTPLayerIdSector* sect = &newSect;

                    sect->m_img = new Weightmap();
                    sect->x = lx;
                    sect->y = ly;
                    heightmap->GetWeightmapBlock(sect->x * layerSize, sect->y * layerSize, layerSize, layerSize, *sect->m_img);

                    layerIds.push_back(newSect);
                }
            }
        }
    }

    void Paste(bool bIsRedo = false)
    {
        CHeightmap* heightmap = GetIEditor()->GetHeightmap();
        CRGBLayer* rgbLayer = GetIEditor()->GetTerrainManager()->GetRGBLayer();

        bool bFirst = true;
        QPoint gminp;
        QPoint gmaxp;

        AABB aabb(AABB::RESET);

        for (int i = sects.size() - 1; i >= 0; i--)
        {
            CUndoTPSector* sect = &sects[i];
            rgbLayer->SetSubImageStretched((float)sect->x / sect->dwSize, (float)sect->y / sect->dwSize,
                (float)(sect->x + sect->w) / sect->dwSize, (float)(sect->y + sect->w) / sect->dwSize, *(bIsRedo ? sect->m_imgRedo : sect->m_img));

            aabb.Add(Vec3((float)sect->x / sect->dwSize, (float)sect->y / sect->dwSize, 0));
            aabb.Add(Vec3((float)(sect->x + sect->w) / sect->dwSize, (float)(sect->y + sect->w) / sect->dwSize, 0));
        }

        // LayerIDs
        for (int i = layerIds.size() - 1; i >= 0; i--)
        {
            CUndoTPLayerIdSector* sect = &layerIds[i];
            heightmap->SetWeightmapBlock(sect->x * sect->m_img->GetWidth(), sect->y * sect->m_img->GetHeight(), *(bIsRedo ? sect->m_imgRedo : sect->m_img));
            heightmap->UpdateEngineTerrain(sect->x * sect->m_img->GetWidth(), sect->y * sect->m_img->GetHeight(), sect->m_img->GetWidth(), sect->m_img->GetHeight(), true, false);
        }

        if (!aabb.IsReset())
        {
            QRect rc(
                QPoint(aabb.min.x* heightmap->GetWidth(), aabb.min.y* heightmap->GetHeight()),
                 QPoint(aabb.max.x* heightmap->GetWidth() - 1, aabb.max.y* heightmap->GetHeight() - 1)
            );
            heightmap->UpdateLayerTexture(rc);
        }
    }

    void StoreRedo()
    {
        CHeightmap* heightmap = GetIEditor()->GetHeightmap();

        for (int i = sects.size() - 1; i >= 0; i--)
        {
            CUndoTPSector* sect = &sects[i];
            if (!sect->m_imgRedo)
            {
                sect->m_imgRedo = new CImageEx();
                sect->m_imgRedo->Allocate(sect->w, sect->w);

                CRGBLayer* rgbLayer = GetIEditor()->GetTerrainManager()->GetRGBLayer();
                rgbLayer->GetSubImageStretched((float)sect->x / sect->dwSize, (float)sect->y / sect->dwSize,
                    (float)(sect->x + sect->w) / sect->dwSize, (float)(sect->y + sect->w) / sect->dwSize, *sect->m_imgRedo);
            }
        }

        // LayerIds
        for (int i = layerIds.size() - 1; i >= 0; i--)
        {
            CUndoTPLayerIdSector* sect = &layerIds[i];
            if (!sect->m_imgRedo)
            {
                sect->m_imgRedo = new Weightmap();
                heightmap->GetWeightmapBlock(sect->x * sect->m_img->GetWidth(), sect->y * sect->m_img->GetHeight(), sect->m_img->GetWidth(), sect->m_img->GetHeight(), *sect->m_imgRedo);
            }
        }
    }

    void Clear()
    {
        for (int i = 0; i < sects.size(); i++)
        {
            CUndoTPSector* sect = &sects[i];
            delete sect->m_img;
            sect->m_img = 0;

            delete sect->m_imgRedo;
            sect->m_imgRedo = 0;
        }

        for (int i = 0; i < layerIds.size(); i++)
        {
            CUndoTPLayerIdSector* sect = &layerIds[i];
            delete sect->m_img;
            sect->m_img = 0;

            delete sect->m_imgRedo;
            sect->m_imgRedo = 0;
        }
    }

    int GetSize()
    {
        int size = 0;
        for (int i = 0; i < sects.size(); i++)
        {
            CUndoTPSector* sect = &sects[i];
            if (sect->m_img)
            {
                size += sect->m_img->GetSize();
            }
            if (sect->m_imgRedo)
            {
                size += sect->m_imgRedo->GetSize();
            }
        }
        for (int i = 0; i < layerIds.size(); i++)
        {
            CUndoTPLayerIdSector* sect = &layerIds[i];
            if (sect->m_img)
            {
                size += sect->m_img->GetSize();
            }
            if (sect->m_imgRedo)
            {
                size += sect->m_imgRedo->GetSize();
            }
        }
        return size;
    }
};

//////////////////////////////////////////////////////////////////////////
//! Undo object.
class CUndoTexturePainter
    : public IUndoObject
{
public:
    CUndoTexturePainter(CTerrainTexturePainter* tool)
    {
        m_undo = tool->m_tpElem;
        tool->m_tpElem = 0;
    }

protected:
    virtual void Release()
    {
        delete m_undo;
        delete this;
    };

    virtual int GetSize() { return sizeof(*this) + (m_undo ? m_undo->GetSize() : 0); };
    virtual QString GetDescription() { return "Terrain Painter Modify"; };

    virtual void Undo(bool bUndo)
    {
        if (bUndo)
        {
            m_undo->StoreRedo();
        }

        if (m_undo)
        {
            m_undo->Paste();
        }
    }
    virtual void Redo()
    {
        if (m_undo)
        {
            m_undo->Paste(true);
        }
    }

private:
    CUndoTPElement* m_undo;
};


CTextureBrush CTerrainTexturePainter::m_brush;

namespace {
    int s_toolPanelId = 0;
    CTerrainPainterPanel* s_toolPanel = 0;
};

//////////////////////////////////////////////////////////////////////////
CTerrainTexturePainter::CTerrainTexturePainter()
{
    m_brush.maxRadius = 1000.0f;

    SetStatusText(tr("Paint Texture Layers"));

    m_heightmap = GetIEditor()->GetHeightmap();
    assert(m_heightmap);

    m_3DEngine = GetIEditor()->Get3DEngine();
    assert(m_3DEngine);

    m_renderer = GetIEditor()->GetRenderer();
    assert(m_renderer);

    m_pointerPos(0, 0, 0);
    GetIEditor()->ClearSelection();

    //////////////////////////////////////////////////////////////////////////
    // Initialize sectors.
    //////////////////////////////////////////////////////////////////////////
    SSectorInfo sectorInfo;
    m_heightmap->GetSectorsInfo(sectorInfo);

    m_isPainting = false;
    m_tpElem = 0;
}

//////////////////////////////////////////////////////////////////////////
CTerrainTexturePainter::~CTerrainTexturePainter()
{
    m_pointerPos(0, 0, 0);

    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->CancelUndo();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::BeginEditParams(IEditor* ie, int flags)
{
    if (!s_toolPanelId)
    {
        s_toolPanel = new CTerrainPainterPanel(*this);
        s_toolPanelId = GetIEditor()->AddRollUpPage(ROLLUP_TERRAIN, tr("Layer Painter"), s_toolPanel);
        MainWindow::instance()->setFocus();

        s_toolPanel->SetBrush(m_brush);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::EndEditParams()
{
    if (s_toolPanelId)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_TERRAIN, s_toolPanelId);
        s_toolPanel = 0;
        s_toolPanelId = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainTexturePainter::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    bool painting = false;
    bool hitTerrain = false;
    m_pointerPos = view->ViewToWorld(point, &hitTerrain, true);

    m_brush.erase = false;

    if (m_isPainting)
    {
        if (event == eMouseLDown || event == eMouseLUp)
        {
            Action_StopUndo();
        }
    }

    if ((flags & MK_CONTROL) != 0)                                                         // pick layerid
    {
        if (event == eMouseLDown)
        {
            Action_PickLayerId();
        }
    }
    else if (event == eMouseLDown || (event == eMouseMove && (flags & MK_LBUTTON)))       // paint
    {
        // Terrain can only exist in the positive quadrant, so don't even try to paint when 
        // we're outside those bounds.
        if ((m_pointerPos.x >= 0.0f) && (m_pointerPos.y >= 0.0f) && hitTerrain)
        {
            Action_Paint();
        }
    }

#if AZ_TRAIT_OS_PLATFORM_APPLE
    GetIEditor()->SetStatusText(tr("L-Mouse:Paint   [ ]: Change Brush Radius  Shift+[ ]:Change Brush Hardness   ⌘L-Mouse:Pick LayerId"));
#else
    GetIEditor()->SetStatusText(tr("L-Mouse:Paint   [ ]: Change Brush Radius  Shift+[ ]:Change Brush Hardness   CTRL+L-Mouse:Pick LayerId"));
#endif

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::Display(DisplayContext& dc)
{
    dc.SetColor(0, 1, 0, 1);

    if (m_pointerPos.x == 0 && m_pointerPos.y == 0 && m_pointerPos.z == 0)
    {
        return;     // mouse cursor not in window
    }
    dc.DepthTestOff();
    dc.DrawTerrainCircle(m_pointerPos, m_brush.radius, 0.2f);
    dc.DepthTestOn();
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainTexturePainter::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    bool processed = false;

    bool shiftKeyPressed = Qt::ShiftModifier & QApplication::queryKeyboardModifiers();
    bool controlKeyPressed = Qt::ControlModifier & QApplication::queryKeyboardModifiers();

    if (nChar == VK_OEM_6)
    {
        if (!shiftKeyPressed && !controlKeyPressed)
        {
            m_brush.radius = clamp_tpl(m_brush.radius + 1, m_brush.minRadius, m_brush.maxRadius);
            processed = true;
        }

        // If you press shift & control together, you can adjust both sliders at the same time.
        if (shiftKeyPressed)
        {
            m_brush.colorHardness = clamp_tpl(m_brush.colorHardness + 0.01f, 0.0f, 1.0f);
            processed = true;
        }
        if (controlKeyPressed)
        {
            m_brush.detailHardness = clamp_tpl(m_brush.detailHardness + 0.01f, 0.0f, 1.0f);
            processed = true;
        }
    }
    else if (nChar == VK_OEM_4)
    {
        if (!shiftKeyPressed && !controlKeyPressed)
        {
            m_brush.radius = clamp_tpl(m_brush.radius - 1, m_brush.minRadius, m_brush.maxRadius);
            processed = true;
        }

        // If you press shift & control together, you can adjust both sliders at the same time.
        if (shiftKeyPressed)
        {
            m_brush.colorHardness = clamp_tpl(m_brush.colorHardness - 0.01f, 0.0f, 1.0f);
            processed = true;
        }
        if (controlKeyPressed)
        {
            m_brush.detailHardness = clamp_tpl(m_brush.detailHardness - 0.01f, 0.0f, 1.0f);
            processed = true;
        }
    }

    if (processed && s_toolPanel)
    {
        s_toolPanel->SetBrush(m_brush);
    }

    return processed;
}
//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::Action_PickLayerId()
{
    int iTerrainSize = m_3DEngine->GetTerrainSize();                                                                            // in m
    int hmapWidth = m_heightmap->GetWidth();
    int hmapHeight = m_heightmap->GetHeight();

    int iX = (int)((m_pointerPos.y * hmapWidth) / iTerrainSize);            // maybe +0.5f is needed
    int iY = (int)((m_pointerPos.x * hmapHeight) / iTerrainSize);           // maybe +0.5f is needed

    if (iX >= 0 && iX < iTerrainSize)
    {
        if (iY >= 0 && iY < iTerrainSize)
        {
            LayerWeight weight = m_heightmap->GetLayerWeightAt(iX, iY);

            CLayer* layer = GetIEditor()->GetTerrainManager()->FindLayerByLayerId(weight.GetLayerId(0));

            s_toolPanel->SelectLayer(layer);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::Action_Paint()
{
    //////////////////////////////////////////////////////////////////////////
    // Paint spot on selected layer.
    //////////////////////////////////////////////////////////////////////////
    CLayer* layer = GetSelectedLayer();
    if (!layer)
    {
        return;
    }

    Vec3 center(m_pointerPos.x, m_pointerPos.y, 0);

    static bool paintLock = false;
    if (paintLock)
    {
        return;
    }

    paintLock = true;

    PaintLayer(layer, center, false);

    GetIEditor()->GetDocument()->SetModifiedFlag(TRUE);
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
    GetIEditor()->UpdateViews(eUpdateHeightmap);

    paintLock = false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::Action_Flood()
{
    if (m_3DEngine->GetTerrainSize() <= 0)
    {
        return;
    }

    //////////////////////////////////////////////////////////////////////////
    // Paint spot on selected layer.
    //////////////////////////////////////////////////////////////////////////
    CLayer* layer = GetSelectedLayer();
    if (!layer)
    {
        return;
    }

    static bool paintLock = false;
    if (paintLock)
    {
        return;
    }

    paintLock = true;

    PaintLayer(layer, Vec3(0.0f), true);

    GetIEditor()->GetDocument()->SetModifiedFlag(TRUE);
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
    GetIEditor()->UpdateViews(eUpdateHeightmap);

    paintLock = false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::PaintLayer(CLayer* pLayer, const Vec3& center, bool bFlood)
{
    float fTerrainSize = (float)m_3DEngine->GetTerrainSize();                                                                           // in m

    // If we're doing a flood fill, set our paint brush to the center of the terrain with a radius that covers the entire terrain
    // regardless of what's been passed in.
    float radius = bFlood ? m_3DEngine->GetTerrainSize() * 0.5f : m_brush.radius;
    Vec3 brushCenter = bFlood ? Vec3(radius, radius, 0.0f) : center;

    SEditorPaintBrush br(*GetIEditor()->GetHeightmap(), *pLayer, m_brush.maskByLayerSettings, m_brush.m_dwMaskLayerId, bFlood);

    br.m_cFilterColor = m_brush.filterColor * m_brush.brightness;
    br.m_cFilterColor.rgb2srgb();
    br.fRadius = radius / fTerrainSize;
    br.color = m_brush.value;
    if (m_brush.erase)
    {
        br.color = 0;
    }

    // Paint spot on layer mask.
    {
        float fX = brushCenter.y / fTerrainSize;                         // 0..1
        float fY = brushCenter.x / fTerrainSize;                         // 0..1

        // change terrain texture
        if (m_brush.colorHardness > 0.0f)
        {
            br.hardness = m_brush.colorHardness;

            CRGBLayer* pRGBLayer = GetIEditor()->GetTerrainManager()->GetRGBLayer();

            assert(pRGBLayer->CalcMinRequiredTextureExtend());

            // load m_texture is needed/possible
            pLayer->PrecacheTexture();

            if (pLayer->m_texture.IsValid())
            {
                Action_CollectUndo(fX, fY, br.fRadius);
                pRGBLayer->PaintBrushWithPatternTiled(fX, fY, br, pLayer->m_texture);
            }
        }

        if (m_brush.detailHardness > 0.0f)        // we also want to change the detail layer data
        {
            br.hardness = m_brush.detailHardness;

            // get unique layerId
            uint32 dwLayerId = pLayer->GetOrRequestLayerId();

            m_heightmap->PaintLayerId(fX, fY, br, dwLayerId);
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // Update Terrain textures.
    //////////////////////////////////////////////////////////////////////////
    {
        // For updating our runtime terrain textures, we adjust our minimum "affected" pixel values to 
        // be one up and to the left from where we've painted.  This is because our runtime textures
        // use bilinear filtering, which puts a copy of the first pixel in a sector into the last pixel 
        // in the previous sector.  So if we modified the first pixel of a sector, subtracting
        // one will cause us to properly update the runtime texture for the previous sector as well.
        const Vec3 bilinearOffset = Vec3(1.0f, 1.0f, 0.0f);
        Vec3 vMin = brushCenter - Vec3(radius, radius, 0.0f) - bilinearOffset;
        Vec3 vMax = brushCenter + Vec3(radius, radius, 0.0f);

        int iTerrainSize = m_3DEngine->GetTerrainSize();                                                // in meters
        int iTexSectorSize = m_3DEngine->GetTerrainTextureNodeSizeMeters();         // in meters

        if (!iTexSectorSize)
        {
            assert(0);                      // maybe we should visualized this to the user
            return;                             // you need to calculated the surface texture first
        }

        // Get the range of sectors that have been modified so that we can update the runtime textures
        // currently in use.
        int iMinSecX = (int)floor(vMin.x / (float)iTexSectorSize);
        int iMinSecY = (int)floor(vMin.y / (float)iTexSectorSize);
        int iMaxSecX = (int)ceil (vMax.x / (float)iTexSectorSize);
        int iMaxSecY = (int)ceil (vMax.y / (float)iTexSectorSize);

        iMinSecX = max(iMinSecX, 0);
        iMinSecY = max(iMinSecY, 0);
        iMaxSecX = min(iMaxSecX, iTerrainSize / iTexSectorSize);
        iMaxSecY = min(iMaxSecY, iTerrainSize / iTexSectorSize);

        float fTerrainSize = (float)m_3DEngine->GetTerrainSize();                                                                           // in m

        // update rectangle in 0..1 range
        float fGlobalMinX = max(vMin.x / fTerrainSize, 0.0f), fGlobalMinY = max(vMin.y / fTerrainSize, 0.0f);
        float fGlobalMaxX = vMax.x / fTerrainSize, fGlobalMaxY = vMax.y / fTerrainSize;

        for (int iY = iMinSecY; iY < iMaxSecY; ++iY)
        {
            for (int iX = iMinSecX; iX < iMaxSecX; ++iX)
            {
                m_heightmap->UpdateSectorTexture(QPoint(iY, iX), fGlobalMinY, fGlobalMinX, fGlobalMaxY, fGlobalMaxX);
            }
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // Update surface types.
    //////////////////////////////////////////////////////////////////////////
    // Build rectangle in heightmap coordinates.
    {
        float fTerrainSize = (float)m_3DEngine->GetTerrainSize();                                                                           // in m
        int hmapWidth = m_heightmap->GetWidth();
        int hmapHeight = m_heightmap->GetHeight();

        float fX = brushCenter.y * hmapWidth / fTerrainSize;
        float fY = brushCenter.x * hmapHeight / fTerrainSize;

        int unitSize = m_heightmap->GetUnitSize();
        float fHMRadius = radius / unitSize;

        // clip against heightmap borders
        int left = max((int)floor(fX - fHMRadius), 0);
        int top = max((int)floor(fY - fHMRadius), 0);
        int width = min((int)ceil(fX + fHMRadius), hmapWidth);
        int height = min((int)ceil(fY + fHMRadius), hmapHeight);

        // Update surface types at 3d engine terrain.
        m_heightmap->UpdateEngineTerrain(left, top, width, height, false, true);
    }
}

//////////////////////////////////////////////////////////////////////////
CLayer* CTerrainTexturePainter::GetSelectedLayer() const
{
    //CString selLayer = s_toolPanel->GetSelectedLayer();
    CLayer* pSelLayer = s_toolPanel->GetSelectedLayer();
    for (int i = 0; i < GetIEditor()->GetTerrainManager()->GetLayerCount(); i++)
    {
        CLayer* pLayer = GetIEditor()->GetTerrainManager()->GetLayer(i);
        if (pSelLayer == pLayer)
        {
            return pLayer;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::Action_StartUndo()
{
    if (m_isPainting)
    {
        return;
    }

    m_tpElem = new CUndoTPElement;

    m_isPainting = true;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::Action_CollectUndo(float x, float y, float radius)
{
    if (!m_isPainting)
    {
        Action_StartUndo();
    }

    m_tpElem->AddSector(x, y, radius);
}


//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::Action_StopUndo()
{
    if (!m_isPainting)
    {
        return;
    }

    AzToolsFramework::UndoSystem::URSequencePoint* undoOperation = nullptr;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(undoOperation, &AzToolsFramework::ToolsApplicationRequests::BeginUndoBatch, "Texture Layer Painting");

    if (undoOperation)
    {
        auto undoCommand = aznew AzToolsFramework::LegacyCommand<IUndoObject>("Texture Layer Painting Command", AZStd::make_unique<CUndoTexturePainter>(this));
        undoCommand->SetParent(undoOperation); // This transfers ownership to undoOperation object who will delete undoCommand.
    }

    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::EndUndoBatch);

    m_isPainting = false;
}

void CTerrainTexturePainter::SaveLayer(CLayer* layer)
{
    if (layer)
    {
        layer->m_cLayerFilterColor = m_brush.filterColor;
        layer->m_fLayerBrightness = m_brush.brightness;
        GetIEditor()->Notify(eNotify_OnInvalidateControls);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::Command_Activate()
{
    CEditTool* tool = GetIEditor()->GetEditTool();
    if (tool && qobject_cast<CTerrainTexturePainter*>(tool))
    {
        // Already active.
        return;
    }

    GetIEditor()->SelectRollUpBar(ROLLUP_TERRAIN);

    // This needs to be done after the terrain tab is selected, because in
    // Cry-Free mode the terrain tool could be closed, whereas in legacy
    // mode the rollupbar is never deleted, it's only hidden
    tool = new CTerrainTexturePainter();
    GetIEditor()->SetEditTool(tool);

    MainWindow::instance()->update();
}

const GUID& CTerrainTexturePainter::GetClassID()
{
    return TERRAIN_PAINTER_TOOL_GUID;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::RegisterTool(CRegistrationContext& rc)
{
    rc.pClassFactory->RegisterClass(new CQtViewClass<CTerrainTexturePainter>("EditTool.TerrainPainter", "Terrain", ESYSTEM_CLASS_EDITTOOL));

    CommandManagerHelper::RegisterCommand(rc.pCommandManager, "edit_tool", "terrain_painter_activate", "", "", functor(CTerrainTexturePainter::Command_Activate));
}

//////////////////////////////////////////////////////////////////////////
class CTerrainTexturePainterBindings
{
public:
    //////////////////////////////////////////////////////////////////////////
    static float PyGetBrushRadius()
    {
        return CTerrainTexturePainter::m_brush.radius;
    }

    //////////////////////////////////////////////////////////////////////////
    static void PySetBrushRadius(float radius)
    {
        CTerrainTexturePainter::m_brush.radius = radius;
        RefreshUI();
    }

    //////////////////////////////////////////////////////////////////////////
    static float PyGetBrushColorHardness()
    {
        return CTerrainTexturePainter::m_brush.colorHardness;
    }

    //////////////////////////////////////////////////////////////////////////
    static void PySetBrushColorHardness(float colorHardness)
    {
        CTerrainTexturePainter::m_brush.colorHardness = colorHardness;
        RefreshUI();
    }

    //////////////////////////////////////////////////////////////////////////
    static float PyGetBrushDetailHardness()
    {
        return CTerrainTexturePainter::m_brush.detailHardness;
    }

    //////////////////////////////////////////////////////////////////////////
    static void PySetBrushDetailHardness(float detailHardness)
    {
        CTerrainTexturePainter::m_brush.detailHardness = detailHardness;
        RefreshUI();
    }

    //////////////////////////////////////////////////////////////////////////
    static bool PyGetBrushMaskByLayerSettings()
    {
        return CTerrainTexturePainter::m_brush.maskByLayerSettings;
    }

    //////////////////////////////////////////////////////////////////////////
    static void PySetBrushMaskByLayerSettings(bool maskByLayerSettings)
    {
        CTerrainTexturePainter::m_brush.maskByLayerSettings = maskByLayerSettings;
        RefreshUI();
    }

    //////////////////////////////////////////////////////////////////////////
    static QString PyGetBrushMaskLayer()
    {
        IEditor* editor = GetIEditor();
        AZ_Assert(editor, "Editor instance doesn't exist!");

        if (editor)
        {
            CTerrainManager* terrainManager = editor->GetTerrainManager();
            AZ_Assert(terrainManager, "Terrain Manager instance doesn't exist!");

            if (terrainManager)
            {
                CLayer* layer = terrainManager->FindLayerByLayerId(CTerrainTexturePainter::m_brush.m_dwMaskLayerId);
                return layer ? layer->GetLayerName() : QString{};
            }
        }

        return {};
    }

    //////////////////////////////////////////////////////////////////////////
    static void PySetBrushMaskLayer(const char* layerName)
    {
        CLayer* layer = FindLayer(layerName);
        CTerrainTexturePainter::m_brush.m_dwMaskLayerId = layer ? layer->GetCurrentLayerId() : CTextureBrush::sInvalidMaskId;
        RefreshUI();
    }

    //////////////////////////////////////////////////////////////////////////
    static boost::python::tuple PyGetLayerBrushColor(const char* layerName)
    {
        CLayer* layer = FindLayer(layerName);

        if (layer)
        {
            ColorF color = layer->GetLayerFilterColor();
            return boost::python::make_tuple(color.r, color.g, color.b);
        }

        return boost::python::make_tuple(0.0f, 0.0f, 0.0f);
    }

    //////////////////////////////////////////////////////////////////////////
    static void PySetLayerBrushColor(const char* layerName, float red, float green, float blue)
    {
        CLayer* layer = FindLayer(layerName);

        if (layer)
        {
            CTerrainTexturePainter::m_brush.filterColor = ColorF(red, green, blue);
            layer->SetLayerFilterColor(CTerrainTexturePainter::m_brush.filterColor);
            RefreshUI();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    static float PyGetLayerBrushColorBrightness(const char* layerName)
    {
        CLayer* layer = FindLayer(layerName);

        if (layer)
        {
            return layer->GetLayerBrightness();
        }
        else
        {
            return 0.0f;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    static void PySetLayerBrushColorBrightness(const char* layerName, float colorBrightness)
    {
        CLayer* layer = FindLayer(layerName);

        if (layer)
        {
            CTerrainTexturePainter::m_brush.brightness = colorBrightness;
            layer->SetLayerBrightness(colorBrightness);
            RefreshUI();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    static void PyPaintLayer(const char* layerName, float centerX, float centerY, float centerZ, bool floodFill)
    {
        IEditor* editor = GetIEditor();
        AZ_Assert(editor, "Editor instance doesn't exist!");
        if (!editor)
        {
            return;
        }

        CTerrainManager* terrainManager = editor->GetTerrainManager();
        AZ_Assert(terrainManager, "Terrain Manager instance doesn't exist!");
        if (!terrainManager)
        {
            return;
        }

        // Select the given layer, deselect the rest.  If more than one layer has the same name,
        // the first one will be selected.
        CLayer* brushLayer = nullptr;
        for (int i = 0; i < terrainManager->GetLayerCount(); i++)
        {
            CLayer* layer = terrainManager->GetLayer(i);
            if ((!brushLayer) && (QString::compare(layer->GetLayerName(), layerName) == 0))
            {
                layer->SetSelected(true);
                brushLayer = layer;
            }
            else
            {
                layer->SetSelected(false);
            }
        }

        if (!brushLayer)
        {
            return;
        }

        CTerrainTexturePainter::Command_Activate();
        RefreshUI();

        CEditTool* editTool = editor->GetEditTool();
        if (editTool && qobject_cast<CTerrainTexturePainter*>(editTool))
        {
            CTerrainTexturePainter* paintTool = reinterpret_cast<CTerrainTexturePainter*>(editTool);

            paintTool->PaintLayer(brushLayer, Vec3(centerX, centerY, centerZ), floodFill);

            editor->GetDocument()->SetModifiedFlag(TRUE);
            editor->SetModifiedModule(eModifiedTerrain);
            editor->UpdateViews(eUpdateHeightmap);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    static float PyGetMinAltitude(const char* layerName)
    {
        if (!layerName)
        {
            return 0.0f;
        }

        CLayer* layer = FindLayer(layerName);

        if (!layer)
        {
            return 0.0f;
        }

        return layer->GetLayerStart();
    }

    //////////////////////////////////////////////////////////////////////////
    static float PyGetMaxAltitude(const char* layerName)
    {
        if (!layerName)
        {
            return 0.0f;
        }

        CLayer* layer = FindLayer(layerName);

        if (!layer)
        {
            return 0.0f;
        }

        return layer->GetLayerEnd();
    }

    //////////////////////////////////////////////////////////////////////////
    static void PySetMinAltitude(const char* layerName, float minAltitude)
    {
        if (!layerName)
        {
            return;
        }

        CLayer* layer = FindLayer(layerName);

        if (!layer)
        {
            return;
        }

        layer->SetLayerStart(minAltitude);
        RefreshUI();
    }

    //////////////////////////////////////////////////////////////////////////
    static void PySetMaxAltitude(const char* layerName, float maxAltitude)
    {
        if (!layerName)
        {
            return;
        }

        CLayer* layer = FindLayer(layerName);

        if (!layer)
        {
            return;
        }

        layer->SetLayerEnd(maxAltitude);
        RefreshUI();
    }

    //////////////////////////////////////////////////////////////////////////
    static float PyGetMinSlope(const char* layerName)
    {
        if (!layerName)
        {
            return 0.0f;
        }

        CLayer* layer = FindLayer(layerName);

        if (!layer)
        {
            return 0.0f;
        }

        return layer->GetLayerMinSlopeAngle();
    }

    //////////////////////////////////////////////////////////////////////////
    static float PyGetMaxSlope(const char* layerName)
    {
        if (!layerName)
        {
            return 0.0f;
        }

        CLayer* layer = FindLayer(layerName);

        if (!layer)
        {
            return 0.0f;
        }

        return layer->GetLayerMaxSlopeAngle();
    }

    //////////////////////////////////////////////////////////////////////////
    static void PySetMinSlope(const char* layerName, float minSlope)
    {
        if (!layerName)
        {
            return;
        }

        CLayer* layer = FindLayer(layerName);

        if (!layer)
        {
            return;
        }

        layer->SetLayerMinSlopeAngle(minSlope);
        RefreshUI();
    }

    //////////////////////////////////////////////////////////////////////////
    static void PySetMaxSlope(const char* layerName, float maxSlope)
    {
        if (!layerName)
        {
            return;
        }

        CLayer* layer = FindLayer(layerName);

        if (!layer)
        {
            return;
        }

        layer->SetLayerMaxSlopeAngle(maxSlope);
        RefreshUI();
    }

private:
    //////////////////////////////////////////////////////////////////////////
    static CLayer* FindLayer(const char* layerName)
    {
        IEditor* editor = GetIEditor();
        AZ_Assert(editor, "Editor instance doesn't exist!");
        if (editor)
        {
            AZ_Assert(editor->GetTerrainManager(), "Terrain Mananger doesn't exist!");
            if (editor->GetTerrainManager())
            {
                return editor->GetTerrainManager()->FindLayer(layerName);
            }
        }

        return nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    static void RefreshUI()
    {
        if (s_toolPanel)
        {
            s_toolPanel->SetBrush(CTerrainTexturePainter::m_brush);
            IEditor* editor = GetIEditor();
            AZ_Assert(editor, "Editor instance doesn't exist!");
            if (editor)
            {
                editor->Notify(eNotify_OnInvalidateControls);
            }
        }
    }
};

//////////////////////////////////////////////////////////////////////////

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PyGetBrushRadius, terrain, get_layer_painter_brush_radius, 
    "Get the terrain layer painter brush radius.", "terrain.get_layer_painter_brush_radius()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PySetBrushRadius, terrain, set_layer_painter_brush_radius,
    "Set the terrain layer painter brush radius.", "terrain.set_layer_painter_brush_radius(float radius)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PyGetBrushColorHardness, terrain, get_layer_painter_brush_color_opacity,
    "Get the terrain layer painter brush color opacity.", "terrain.get_layer_painter_brush_color_opacity()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PySetBrushColorHardness, terrain, set_layer_painter_brush_color_opacity,
    "Set the terrain layer painter brush color opacity.", "terrain.set_layer_painter_brush_color_opacity(float opacity)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PyGetBrushDetailHardness, terrain, get_layer_painter_brush_detail_intensity,
    "Get the terrain layer painter brush detail intensity.", "terrain.get_layer_painter_brush_detail_intensity()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PySetBrushDetailHardness, terrain, set_layer_painter_brush_detail_intensity,
    "Set the terrain layer painter brush detail intensity.", "terrain.set_layer_painter_brush_detail_intensity(float intensity)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PyGetBrushMaskByLayerSettings, terrain, get_layer_painter_brush_mask_by_layer_settings,
    "Get the terrain layer painter brush setting for masking by layer settings (altitude, slope).", "terrain.get_layer_painter_brush_mask_by_layer_settings()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PySetBrushMaskByLayerSettings, terrain, set_layer_painter_brush_mask_by_layer_settings,
    "Set the terrain layer painter brush setting for masking by layer settings (altitude, slope).", "terrain.set_layer_painter_brush_mask_by_layer_settings(bool enable)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PyGetBrushMaskLayer, terrain, get_layer_painter_brush_mask_layer_name,
    "Get the terrain layer painter brush 'mask by layer' layer name.", "terrain.get_layer_painter_brush_mask_layer_name()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PySetBrushMaskLayer, terrain, set_layer_painter_brush_mask_layer_name,
    "Set the terrain layer painter brush 'mask by layer' layer name.", "terrain.set_layer_painter_brush_mask_layer_name(string layer)");

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PyGetLayerBrushColor, terrain, get_layer_brush_color,
    "Get the specific terrain layer's brush color.", "terrain.get_layer_brush_color(string layer)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PySetLayerBrushColor, terrain, set_layer_brush_color,
    "Set the specific terrain layer's brush color.", "terrain.set_layer_brush_color(string layer, float red, float green, float blue)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PyGetLayerBrushColorBrightness, terrain, get_layer_brush_color_brightness,
    "Get the specific terrain layer's brush color brightness setting.", "terrain.get_layer_brush_color_brightness(string layer)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PySetLayerBrushColorBrightness, terrain, set_layer_brush_color_brightness,
    "Set the specific terrain layer's brush color brightness setting.", "terrain.set_layer_brush_color_brightness(string layer, float brightness)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PyPaintLayer, terrain, paint_layer,
    "Paint the terrain using the brush settings from the given layer and the terrain layer painter.", 
    "terrain.paint_layer(string layer, float center_x, float center_y, float center_z, bool flood_fill)");

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PyGetMinAltitude, terrain, get_layer_min_altitude,
    "Returns the min altitude.", "terrain.get_layer_min_altitude(string layer_name)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PyGetMaxAltitude, terrain, get_layer_max_altitude,
    "Returns the max altitude.", "terrain.get_layer_max_altitude(string layer_name)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PySetMinAltitude, terrain, set_layer_min_altitude,
    "Sets the min altitude.", "terrain.set_layer_min_altitude(string layer_name, float min_altitude)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PySetMaxAltitude, terrain, set_layer_max_altitude,
    "Sets the max altitude.", "terrain.set_layer_max_altitude(string layer_name, float max_altitude)");

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PyGetMinSlope, terrain, get_layer_min_slope,
    "Returns the min slope.", "terrain.get_layer_min_slope(string layer_name)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PyGetMaxSlope, terrain, get_layer_max_slope,
    "Returns the max slope.", "terrain.get_layer_max_slope(string layer_name)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PySetMinSlope, terrain, set_layer_min_slope,
    "Sets the min slope.", "terrain.set_layer_min_slope(string layer_name, float min_slope)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(CTerrainTexturePainterBindings::PySetMaxSlope, terrain, set_layer_max_slope,
    "Sets the max slope.", "terrain.set_layer_max_slope(string layer_name, float max_slope)");

#include <TerrainTexturePainter.moc>