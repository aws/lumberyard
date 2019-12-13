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
private:
    friend class ScriptingBindings;

public:
    Q_INVOKABLE CTerrainHoleTool();
    virtual ~CTerrainHoleTool();

    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams();

    virtual void Display(DisplayContext& dc);

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    // Key down.
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);

    // Delete itself.
    void DeleteThis() { delete this; };

    void SetBrushRadius(float r)
    {
        if (r < 0.1f)
        {
            r = 0.1f;
        }
        m_brushRadius = r;
    };
    float GetBrushRadius() const { return m_brushRadius; };

    void SetMakeHole(bool bEnable) { m_bMakeHole = bEnable; }
    bool GetMakeHole() { return m_bMakeHole; }

    AZStd::pair<Vec2i, Vec2i> Modify();

private:
    bool UpdatePointerPos(CViewport* view, const QPoint& pos);

    bool CalculateHoleArea(Vec2i& min, Vec2i& max) const;

    Vec3 m_pointerPos;
    static float m_brushRadius;
    static bool m_bMakeHole;

    int m_panelId;
    class CTerrainHolePanel* m_panel;
};


#endif // CRYINCLUDE_EDITOR_TERRAINHOLETOOL_H
