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
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   CloneTool.h
//  Created:     June/6/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/BaseTool.h"
#include "Objects/DesignerObject.h"

namespace CD
{
    enum EPlacementType
    {
        ePlacementType_Divide,
        ePlacementType_Multiply,
        ePlacementType_None
    };

    static const int kDefaultNumberOfClone = 5;

    struct SCloneParameter
    {
        int m_NumberOfClones;
        CD::EPlacementType m_PlacementType;

        SCloneParameter()
            : m_NumberOfClones(kDefaultNumberOfClone)
            , m_PlacementType(ePlacementType_Divide)
        {
        }

        void Serialize(Serialization::IArchive& ar);
    };

    typedef DesignerObject Clone;
}

class CloneTool
    : public BaseTool
{
public:

    CloneTool(CD::EDesignerTool tool);

    const char* ToolClass() const override;

    void Enter() override;
    void Leave() override;

    void Display(struct DisplayContext& dc) override;
    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    void Serialize(Serialization::IArchive& ar);
    void OnChangeParameter(bool continuous) override;

private:

    void Confirm();
    void FreezeClones();

    void SetPivotToObject(CBaseObject* pObj, const Vec3& pos);
    Vec3 GetCenterBottom(const AABB& aabb) const;
    void UpdateClonePositions();
    void UpdateCloneList();

    void UpdateClonePositionsAlongCircle();
    void UpdateClonePositionsAlongLine();

    typedef _smart_ptr<CD::Clone> ClonePtr;

    std::vector<ClonePtr> m_Clones;
    ClonePtr m_SelectedClone;

    BrushPlane m_Plane;
    Vec3 m_vStartPos;
    Vec3 m_vPickedPos;
    float m_fRadius;
    bool m_bSuspendedUndo;

    Vec3 m_InitObjectWorldPos;

    CD::SCloneParameter m_CloneParameter;
};

class ArrayCloneTool
    : public CloneTool
{
public:
    ArrayCloneTool(CD::EDesignerTool tool)
        : CloneTool(tool)
    {
    }
};