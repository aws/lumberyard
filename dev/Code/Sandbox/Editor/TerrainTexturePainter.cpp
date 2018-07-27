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

#ifdef AZ_PLATFORM_WINDOWS
#include <InitGuid.h>
#endif

// {C3EE67BE-F167-4E48-93B6-47D184DC06F8}
DEFINE_GUID(TERRAIN_PAINTER_TOOL_GUID, 0xc3ee67be, 0xf167, 0x4e48, 0x93, 0xb6, 0x47, 0xd1, 0x84, 0xdc, 0x6, 0xf8);



struct CUndoTPSector
{
    CUndoTPSector()
    {
        m_pImg = 0;
        m_pImgRedo = 0;
    }

    QPoint tile;
    int x, y;
    int w;
    uint32 dwSize;
    CImageEx* m_pImg;
    CImageEx* m_pImgRedo;
};


struct CUndoTPLayerIdSector
{
    CUndoTPLayerIdSector()
    {
        m_pImg = 0;
        m_pImgRedo = 0;
    }

    int x, y;
    Weightmap* m_pImg;
    Weightmap* m_pImgRedo;
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

        CRGBLayer* pRGBLayer = GetIEditor()->GetTerrainManager()->GetRGBLayer();
        uint32 dwMaxRes = pRGBLayer->CalcMaxLocalResolution(0, 0, 1, 1);

        uint32 dwTileCountX = pRGBLayer->GetTileCountX();
        uint32 dwTileCountY = pRGBLayer->GetTileCountY();

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
                uint32 dwLocalSize = pRGBLayer->GetTileResolution(dwTileX, dwTileY);
                uint32 dwSize = pRGBLayer->GetTileResolution(dwTileX, dwTileY) * pRGBLayer->GetTileCountX();

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
                            CUndoTPSector* pSect = &sects[i];
                            if (pSect->tile.x() == dwTileX && pSect->tile.y() == dwTileY)
                            {
                                if (pSect->x == sx &&  pSect->y == sy)
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

                            newSect.m_pImg = new CImageEx;
                            newSect.m_pImg->Allocate(newSect.w, newSect.w);

                            CUndoTPSector* pSect = &newSect;

                            pRGBLayer->GetSubImageStretched((float)pSect->x / pSect->dwSize, (float)pSect->y / pSect->dwSize,
                                (float)(pSect->x + pSect->w) / pSect->dwSize, (float)(pSect->y + pSect->w) / pSect->dwSize, *pSect->m_pImg);

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
                    CUndoTPLayerIdSector* pSect = &layerIds[i];
                    if (pSect->x == lx &&  pSect->y == ly)
                    {
                        bFind = true;
                        break;
                    }
                }
                if (!bFind)
                {
                    CUndoTPLayerIdSector newSect;
                    CUndoTPLayerIdSector* pSect = &newSect;

                    pSect->m_pImg = new Weightmap();
                    pSect->x = lx;
                    pSect->y = ly;
                    heightmap->GetWeightmapBlock(pSect->x * layerSize, pSect->y * layerSize, layerSize, layerSize, *pSect->m_pImg);

                    layerIds.push_back(newSect);
                }
            }
        }
    }

    void Paste(bool bIsRedo = false)
    {
        CHeightmap* heightmap = GetIEditor()->GetHeightmap();
        CRGBLayer* pRGBLayer = GetIEditor()->GetTerrainManager()->GetRGBLayer();

        bool bFirst = true;
        QPoint gminp;
        QPoint gmaxp;

        AABB aabb(AABB::RESET);

        for (int i = sects.size() - 1; i >= 0; i--)
        {
            CUndoTPSector* pSect = &sects[i];
            pRGBLayer->SetSubImageStretched((float)pSect->x / pSect->dwSize, (float)pSect->y / pSect->dwSize,
                (float)(pSect->x + pSect->w) / pSect->dwSize, (float)(pSect->y + pSect->w) / pSect->dwSize, *(bIsRedo ? pSect->m_pImgRedo : pSect->m_pImg));

            aabb.Add(Vec3((float)pSect->x / pSect->dwSize, (float)pSect->y / pSect->dwSize, 0));
            aabb.Add(Vec3((float)(pSect->x + pSect->w) / pSect->dwSize, (float)(pSect->y + pSect->w) / pSect->dwSize, 0));
        }

        // LayerIDs
        for (int i = layerIds.size() - 1; i >= 0; i--)
        {
            CUndoTPLayerIdSector* pSect = &layerIds[i];
            heightmap->SetWeightmapBlock(pSect->x * pSect->m_pImg->GetWidth(), pSect->y * pSect->m_pImg->GetHeight(), *(bIsRedo ? pSect->m_pImgRedo : pSect->m_pImg));
            heightmap->UpdateEngineTerrain(pSect->x * pSect->m_pImg->GetWidth(), pSect->y * pSect->m_pImg->GetHeight(), pSect->m_pImg->GetWidth(), pSect->m_pImg->GetHeight(), true, false);
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
            CUndoTPSector* pSect = &sects[i];
            if (!pSect->m_pImgRedo)
            {
                pSect->m_pImgRedo = new CImageEx();
                pSect->m_pImgRedo->Allocate(pSect->w, pSect->w);

                CRGBLayer* pRGBLayer = GetIEditor()->GetTerrainManager()->GetRGBLayer();
                pRGBLayer->GetSubImageStretched((float)pSect->x / pSect->dwSize, (float)pSect->y / pSect->dwSize,
                    (float)(pSect->x + pSect->w) / pSect->dwSize, (float)(pSect->y + pSect->w) / pSect->dwSize, *pSect->m_pImgRedo);
            }
        }

        // LayerIds
        for (int i = layerIds.size() - 1; i >= 0; i--)
        {
            CUndoTPLayerIdSector* pSect = &layerIds[i];
            if (!pSect->m_pImgRedo)
            {
                pSect->m_pImgRedo = new Weightmap();
                heightmap->GetWeightmapBlock(pSect->x * pSect->m_pImg->GetWidth(), pSect->y * pSect->m_pImg->GetHeight(), pSect->m_pImg->GetWidth(), pSect->m_pImg->GetHeight(), *pSect->m_pImgRedo);
            }
        }
    }

    void Clear()
    {
        for (int i = 0; i < sects.size(); i++)
        {
            CUndoTPSector* pSect = &sects[i];
            delete pSect->m_pImg;
            pSect->m_pImg = 0;

            delete pSect->m_pImgRedo;
            pSect->m_pImgRedo = 0;
        }

        for (int i = 0; i < layerIds.size(); i++)
        {
            CUndoTPLayerIdSector* pSect = &layerIds[i];
            delete pSect->m_pImg;
            pSect->m_pImg = 0;

            delete pSect->m_pImgRedo;
            pSect->m_pImgRedo = 0;
        }
    }

    int GetSize()
    {
        int size = 0;
        for (int i = 0; i < sects.size(); i++)
        {
            CUndoTPSector* pSect = &sects[i];
            if (pSect->m_pImg)
            {
                size += pSect->m_pImg->GetSize();
            }
            if (pSect->m_pImgRedo)
            {
                size += pSect->m_pImgRedo->GetSize();
            }
        }
        for (int i = 0; i < layerIds.size(); i++)
        {
            CUndoTPLayerIdSector* pSect = &layerIds[i];
            if (pSect->m_pImg)
            {
                size += pSect->m_pImg->GetSize();
            }
            if (pSect->m_pImgRedo)
            {
                size += pSect->m_pImgRedo->GetSize();
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
    CUndoTexturePainter(CTerrainTexturePainter* pTool)
    {
        m_pUndo = pTool->m_pTPElem;
        pTool->m_pTPElem = 0;
    }

protected:
    virtual void Release()
    {
        delete m_pUndo;
        delete this;
    };

    virtual int GetSize() { return sizeof(*this) + (m_pUndo ? m_pUndo->GetSize() : 0); };
    virtual QString GetDescription() { return "Terrain Painter Modify"; };

    virtual void Undo(bool bUndo)
    {
        if (bUndo)
        {
            m_pUndo->StoreRedo();
        }

        if (m_pUndo)
        {
            m_pUndo->Paste();
        }
    }
    virtual void Redo()
    {
        if (m_pUndo)
        {
            m_pUndo->Paste(true);
        }
    }

private:
    CUndoTPElement* m_pUndo;
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

    m_bIsPainting = false;
    m_pTPElem = 0;
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
    bool bPainting = false;
    bool hitTerrain = false;
    m_pointerPos = view->ViewToWorld(point, &hitTerrain, true);

    m_brush.bErase = false;

    if (m_bIsPainting)
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

#ifdef AZ_PLATFORM_APPLE
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
    bool bProcessed = false;

    bool shiftKeyPressed = Qt::ShiftModifier & QApplication::queryKeyboardModifiers();
    bool controlKeyPressed = Qt::ControlModifier & QApplication::queryKeyboardModifiers();

    if (nChar == VK_OEM_6)
    {
        if (!shiftKeyPressed && !controlKeyPressed)
        {
            m_brush.radius = clamp_tpl(m_brush.radius + 1, m_brush.minRadius, m_brush.maxRadius);
            bProcessed = true;
        }

        // If you press shift & control together, you can adjust both sliders at the same time.
        if (shiftKeyPressed)
        {
            m_brush.colorHardness = clamp_tpl(m_brush.colorHardness + 0.01f, 0.0f, 1.0f);
            bProcessed = true;
        }
        if (controlKeyPressed)
        {
            m_brush.detailHardness = clamp_tpl(m_brush.detailHardness + 0.01f, 0.0f, 1.0f);
            bProcessed = true;
        }
    }
    else if (nChar == VK_OEM_4)
    {
        if (!shiftKeyPressed && !controlKeyPressed)
        {
            m_brush.radius = clamp_tpl(m_brush.radius - 1, m_brush.minRadius, m_brush.maxRadius);
            bProcessed = true;
        }

        // If you press shift & control together, you can adjust both sliders at the same time.
        if (shiftKeyPressed)
        {
            m_brush.colorHardness = clamp_tpl(m_brush.colorHardness - 0.01f, 0.0f, 1.0f);
            bProcessed = true;
        }
        if (controlKeyPressed)
        {
            m_brush.detailHardness = clamp_tpl(m_brush.detailHardness - 0.01f, 0.0f, 1.0f);
            bProcessed = true;
        }
    }

    if (bProcessed && s_toolPanel)
    {
        s_toolPanel->SetBrush(m_brush);
    }

    return bProcessed;
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

            CLayer* pLayer = GetIEditor()->GetTerrainManager()->FindLayerByLayerId(weight.GetLayerId(0));

            s_toolPanel->SelectLayer(pLayer);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::Action_Paint()
{
    //////////////////////////////////////////////////////////////////////////
    // Paint spot on selected layer.
    //////////////////////////////////////////////////////////////////////////
    CLayer* pLayer = GetSelectedLayer();
    if (!pLayer)
    {
        return;
    }

    Vec3 center(m_pointerPos.x, m_pointerPos.y, 0);

    static bool bPaintLock = false;
    if (bPaintLock)
    {
        return;
    }

    bPaintLock = true;

    PaintLayer(pLayer, center, false);

    GetIEditor()->GetDocument()->SetModifiedFlag(TRUE);
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
    GetIEditor()->UpdateViews(eUpdateHeightmap);

    bPaintLock = false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::Action_Flood()
{
    //////////////////////////////////////////////////////////////////////////
    // Paint spot on selected layer.
    //////////////////////////////////////////////////////////////////////////
    CLayer* pLayer = GetSelectedLayer();
    if (!pLayer)
    {
        return;
    }

    float oldRadius = m_brush.radius;
    float fRadius = m_3DEngine->GetTerrainSize() * 0.5f;
    m_brush.radius = fRadius;
    Vec3 center(fRadius, fRadius, 0);

    static bool bPaintLock = false;
    if (bPaintLock)
    {
        return;
    }

    bPaintLock = true;

    PaintLayer(pLayer, center, true);

    GetIEditor()->GetDocument()->SetModifiedFlag(TRUE);
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
    GetIEditor()->UpdateViews(eUpdateHeightmap);

    m_brush.radius = oldRadius;
    bPaintLock = false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::PaintLayer(CLayer* pLayer, const Vec3& center, bool bFlood)
{
    float fTerrainSize = (float)m_3DEngine->GetTerrainSize();                                                                           // in m

    SEditorPaintBrush br(*GetIEditor()->GetHeightmap(), *pLayer, m_brush.bMaskByLayerSettings, m_brush.m_dwMaskLayerId, bFlood);

    br.m_cFilterColor = m_brush.m_cFilterColor * m_brush.m_fBrightness;
    br.m_cFilterColor.rgb2srgb();
    br.fRadius = m_brush.radius / fTerrainSize;
    br.color = m_brush.value;
    if (m_brush.bErase)
    {
        br.color = 0;
    }

    // Paint spot on layer mask.
    {
        float fX = center.y / fTerrainSize;                         // 0..1
        float fY = center.x / fTerrainSize;                         // 0..1

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


    Vec3 vMin = center - Vec3(m_brush.radius, m_brush.radius, 0);
    Vec3 vMax = center + Vec3(m_brush.radius, m_brush.radius, 0);

    //////////////////////////////////////////////////////////////////////////
    // Update Terrain textures.
    //////////////////////////////////////////////////////////////////////////
    {
        int iTerrainSize = m_3DEngine->GetTerrainSize();                                                // in meters
        int iTexSectorSize = m_3DEngine->GetTerrainTextureNodeSizeMeters();         // in meters

        if (!iTexSectorSize)
        {
            assert(0);                      // maybe we should visualized this to the user
            return;                             // you need to calculated the surface texture first
        }

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
        float fGlobalMinX = vMin.x / fTerrainSize, fGlobalMinY = vMin.y / fTerrainSize;
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

        float fX = center.y * hmapWidth / fTerrainSize;
        float fY = center.x * hmapHeight / fTerrainSize;

        int unitSize = m_heightmap->GetUnitSize();
        float fHMRadius = m_brush.radius / unitSize;
        QRect hmaprc;

        // clip against heightmap borders
        hmaprc.setLeft(max((int)floor(fX - fHMRadius), 0));
        hmaprc.setTop(max((int)floor(fY - fHMRadius), 0));
        hmaprc.setRight(min((int)ceil(fX + fHMRadius), hmapWidth));
        hmaprc.setBottom(min((int)ceil(fY + fHMRadius), hmapHeight));

        // Update surface types at 3d engine terrain.
        m_heightmap->UpdateEngineTerrain(hmaprc.left(), hmaprc.top(), hmaprc.width(), hmaprc.height(), false, true);
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
    if (m_bIsPainting)
    {
        return;
    }

    m_pTPElem = new CUndoTPElement;

    m_bIsPainting = true;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::Action_CollectUndo(float x, float y, float radius)
{
    if (!m_bIsPainting)
    {
        Action_StartUndo();
    }

    m_pTPElem->AddSector(x, y, radius);
}


//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::Action_StopUndo()
{
    if (!m_bIsPainting)
    {
        return;
    }

    GetIEditor()->BeginUndo();

    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoTexturePainter(this));
    }

    GetIEditor()->AcceptUndo("Texture Layer Painting");

    m_bIsPainting = false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTexturePainter::Command_Activate()
{
    CEditTool* pTool = GetIEditor()->GetEditTool();
    if (pTool && qobject_cast<CTerrainTexturePainter*>(pTool))
    {
        // Already active.
        return;
    }

    GetIEditor()->SelectRollUpBar(ROLLUP_TERRAIN);

    // This needs to be done after the terrain tab is selected, because in
    // Cry-Free mode the terrain tool could be closed, whereas in legacy
    // mode the rollupbar is never deleted, it's only hidden
    pTool = new CTerrainTexturePainter();
    GetIEditor()->SetEditTool(pTool);

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

#include <TerrainTexturePainter.moc>