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

#pragma once

#include "Tools/BaseTool.h"
#include "Util/SpotManager.h"

namespace CD
{
    enum ECubeEditorMode
    {
        eCubeEditorMode_Add,
        eCubeEditorMode_Remove,
        eCubeEditorMode_Paint,
        eCubeEditorMode_Invalid
    };
}

class CubeEditor
    : public BaseTool
{
public:

    CubeEditor(CD::EDesignerTool tool)
        : BaseTool(tool)
    {
    }

    void Enter() override;

    void BeginEditParams() override;
    void EndEditParams() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseWheel(CViewport* view, UINT nFlags, const QPoint& point) override;

    void Display(DisplayContext& dc) override;

    void MaterialChanged() override;
    void SetSubMatID(int nSubMatID) override;

    void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event) override;

    CD::ECubeEditorMode GetEditMode() const;

private:

    BrushFloat GetCubeSize() const;
    bool IsAddMode() const;
    bool IsRemoveMode() const;
    bool IsPaintMode() const;
    void SelectPrevBrush() const;
    void SelectNextBrush() const;
    int  GetSubMatID() const;
    bool IsSideMerged() const;

    void DisplayBrush(DisplayContext& dc);
    std::vector<CD::PolygonPtr> GetBrushPolygons(const AABB& aabb) const;
    void AddCube(const AABB& brushAABB);
    void RemoveCube(const AABB& brushAABB);
    void PaintCube(const AABB& brushAABB);
    AABB GetBrushBox(CViewport* view, const QPoint& point);
    AABB GetBrushBox(const BrushVec3& vSnappedPos, const BrushVec3& vPickedPos, const BrushVec3& vNormal);

    bool GetBrushPos(CViewport* view, const QPoint& point, BrushVec3& outSnappedPos, BrushVec3& outPickedPos, BrushVec3* pOutNormal);
    BrushVec3 Snap(const BrushVec3& vPos) const;

    void AddBrush(const AABB& aabb);

    AABB m_BrushAABB;
    std::vector<AABB> m_BrushAABBs;
    QPoint m_CurMousePos;

    struct SDrawStraight
    {
        SDrawStraight()
            : m_bPressingShift(false)
            , m_StraightDir(0, 0, 0)
            , m_StartingPos(0, 0, 0)
            , m_StartingNormal(0, 0, 0)
        {
        }
        BrushVec3 m_StraightDir;
        BrushVec3 m_StartingPos;
        BrushVec3 m_StartingNormal;
        bool m_bPressingShift;
    };
    SDrawStraight m_DS;
};
