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

#ifndef CRYINCLUDE_EDITOR_TERRAINTEXTUREPAINTER_H
#define CRYINCLUDE_EDITOR_TERRAINTEXTUREPAINTER_H

#pragma once

class CHeightmap;
class CLayer;

/** Terrain Texture brush types.
*/
enum ETextureBrushType
{
    ET_BRUSH_PAINT = 1,
};

/** Terrain texture brush.
*/
struct CTextureBrush
{
    ETextureBrushType                   type;                   // Type of this brush.
    float                               radius;                 // Radius of brush in meters
    float                               colorHardness;          // Opacity of layer color in brush painting
    float                               detailHardness;         // Opacity of detail texture in brush painting
    float                               value;                  //
    bool                                erase;                  //

    bool                                maskByLayerSettings;    //

    float                               minRadius;              //
    float                               maxRadius;              //
    ColorF                              filterColor;            //
    float                               brightness;             // used together with filterColor
    uint32                              m_dwMaskLayerId;        // 0xffffffff if not used

    CTextureBrush()
    {
        type = ET_BRUSH_PAINT;
        radius = 4;
        colorHardness = 1.0f;
        detailHardness = 1.0f;
        value = 255;
        erase = false;
        minRadius = 0.01f;
        maxRadius = 256.0f;
        maskByLayerSettings = true;
        m_dwMaskLayerId = sInvalidMaskId;
        filterColor = ColorF(1, 1, 1);
        brightness = 1.0f;
    }

    static constexpr uint32 sInvalidMaskId = 0xffffffff;
};

//////////////////////////////////////////////////////////////////////////
class CTerrainTexturePainter
    : public CEditTool
{
    Q_OBJECT

private:
    friend class CTerrainTexturePainterBindings;

public:
    Q_INVOKABLE CTerrainTexturePainter();
    virtual ~CTerrainTexturePainter();

    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams();

    virtual void Display(DisplayContext& dc);

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    // Key down.
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) { return false; };

    // Delete itself.
    void DeleteThis() { delete this; };

    void SetBrush(const CTextureBrush& brush) { m_brush = brush; };
    void GetBrush(CTextureBrush& brush) const { brush = m_brush; };

    void Action_Flood();
    void Action_Paint();
    void Action_PickLayerId();

    //Undo
    void Action_StartUndo();
    void Action_CollectUndo(float x, float y, float radius);
    void Action_StopUndo();

    static void SaveLayer(CLayer* pLayer);

    static const GUID& GetClassID();
    static void RegisterTool(CRegistrationContext& rc);

private:
    void PaintLayer(CLayer* pLayer, const Vec3& center, bool bFlood);
    CLayer* GetSelectedLayer() const;
    static void Command_Activate();

private:
    Vec3 m_pointerPos;

    //! Flag is true if painting mode. Used for Undo.
    bool m_isPainting;

    struct CUndoTPElement* m_tpElem;

    // Cache often used interfaces.
    I3DEngine* m_3DEngine;
    IRenderer* m_renderer;
    CHeightmap* m_heightmap;

    static CTextureBrush m_brush;

    friend class CUndoTexturePainter;
};

#endif // CRYINCLUDE_EDITOR_TERRAINTEXTUREPAINTER_H
