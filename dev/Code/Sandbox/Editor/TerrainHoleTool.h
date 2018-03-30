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

#ifndef CRYINCLUDE_EDITOR_TERRAINHOLETOOL_H
#define CRYINCLUDE_EDITOR_TERRAINHOLETOOL_H

#pragma once

//////////////////////////////////////////////////////////////////////////
class CTerrainHoleTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CTerrainHoleTool();
    virtual ~CTerrainHoleTool();

    virtual void BeginEditParams(IEditor* ie, int flags) override;
    virtual void EndEditParams() override;

    virtual void Display(DisplayContext& dc) override;

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags) override;
    bool TabletCallback(CViewport* view, ETabletEvent event, const QPoint& point, const STabletContext& tabletContext, int flags) override;

    // Key down.
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
    bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;

    // Delete itself.
    void DeleteThis() override { delete this; };

    void SetBrushRadius(float r)
    {
        if (r < 0.1f)
        {
            r = 0.1f;
        }
        m_brushRadius = r;
    };
    float GetBrushRadius() const { return m_brushRadius; };

    void SetPressureRadius(bool value) { m_bPressureRadius = value; }
    bool GetPressureRadius() const { return m_bPressureRadius; }

    void SetMakeHole(bool bEnable) { m_bMakeHole = bEnable; }
    bool GetMakeHole() { return m_bMakeHole; }

    void Modify(float pressure = 1.0f);

private:
    bool CalculateHoleArea(float pressure, Vec2i& min, Vec2i& max) const;

    Vec3 m_pointerPos;
    static float m_brushRadius;
    static bool m_bMakeHole;

    bool m_bInTabletMode;
    bool m_bPressureRadius;
    float m_activePressure;

    int m_panelId;
    class CTerrainHolePanel* m_panel;
};


#endif // CRYINCLUDE_EDITOR_TERRAINHOLETOOL_H
