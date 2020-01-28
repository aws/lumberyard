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

// Description : Terrain modification tool.


#ifndef CRYINCLUDE_EDITOR_TERRAINMODIFYTOOL_H
#define CRYINCLUDE_EDITOR_TERRAINMODIFYTOOL_H

#pragma once

enum BrushType
{
    eBrushFlatten = 1,
    eBrushSmooth,
    eBrushNotSmooth,
    eBrushRiseLower,
    eBrushPickHeight,
    eBrushTypeLast
};

struct CTerrainBrush
{
    // Type of this brush.
    BrushType type;
    //! Outside Radius of brush.
    float radius;
    //! Inside Radius of brush.
    float radiusInside;
    //! Height where to paint.
    float height;
    //! Min and Max height of the brush.
    Vec2 heightRange;
    //! How hard this brush.
    float hardness;
    //! Is this brush have noise.
    bool bNoise;
    //! Scale of applied noise.
    float noiseScale;
    //! Frequency of applied noise.
    float noiseFreq;
    //! True if objects should be repositioned on modified terrain.
    bool bRepositionObjects;
    //! True if vegetation should be repositioned on modified terrain.
    bool bRepositionVegetation;


    CTerrainBrush()
    {
        type = eBrushFlatten;
        radius  = 2;
        radiusInside = 0;               // default 0 is preferred by designers
        height = 1;
        bNoise = false;
        hardness = 0.2f;
        noiseScale = 5;
        noiseFreq = 100;
        bRepositionObjects = false;
        bRepositionVegetation = true;
        heightRange = Vec2(0, 0);
    }
};

//////////////////////////////////////////////////////////////////////////
class CTerrainModifyTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CTerrainModifyTool();
    virtual ~CTerrainModifyTool();

    static const GUID& GetClassID();

    //! Register this tool to editor system.
    static void RegisterTool(CRegistrationContext& rc);

    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams();

    virtual void Display(DisplayContext& dc);

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    // Key down.
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    bool OnSetCursor(CViewport* vp);

    // Delete itself.
    void DeleteThis() { delete this; };

    void SetCurBrushType(BrushType type);
    BrushType GetCurBrushType() const { return m_currentBrushType; }
    void SetCurBrushParams(const CTerrainBrush& brush);
    void GetCurBrushParams(CTerrainBrush& brush) const { brush = m_brush[m_currentBrushType]; };
    void EnableBrushNoise(bool enable) { m_brush[m_currentBrushType].bNoise = enable; };
    void AdjustBrushValues();
    void SyncBrushRadiuses(bool bSync);

    void Paint();

    void SetExternalUIPanel(class CTerrainModifyPanel* pPanel);
    void ClearCtrlPressedState() { m_isCtrlPressed = false; }

    //////////////////////////////////////////////////////////////////////////
    // Commands.
    static void Command_Activate();
    static void Command_Flatten();
    static void Command_Smooth();
    static void Command_RiseLower();
    static int Debug_GetCurrentBrushType();
    //////////////////////////////////////////////////////////////////////////

private:
    void UpdateUI();

    Vec3 m_pointerPos;

    int m_panelId;
    class CTerrainModifyPanel* m_panel;

    static BrushType m_currentBrushType;
    static CTerrainBrush m_brush[eBrushTypeLast];
    CTerrainBrush* m_pBrush;
    CTerrainBrush* m_prevBrush;

    bool m_bSmoothOverride;
    bool m_bQueryHeightMode;
    int m_nPaintingMode;

    static bool m_bSyncBrushRadiuses;

    QPoint m_MButtonDownPos;
    float m_prevRadius;
    float m_prevRadiusInside;
    float m_prevHeight;

    bool m_isCtrlPressed;
    bool m_isAltPressed;

    QCursor m_hPickCursor;
    QCursor m_hPaintCursor;
    QCursor m_hFlattenCursor;
    QCursor m_hSmoothCursor;
};

#include <AzCore/Component/Component.h>

namespace AzToolsFramework
{
    //! A component to reflect scriptable commands for the Editor
    class TerrainModifyPythonFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(TerrainModifyPythonFuncsHandler, "{50AD3A1B-9431-498F-889F-A9C323741730}")

        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

} // namespace AzToolsFramework

#endif // CRYINCLUDE_EDITOR_TERRAINMODIFYTOOL_H
