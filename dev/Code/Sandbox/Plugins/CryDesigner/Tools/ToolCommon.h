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

class DesignerTool;

#include "ToolFactory.h"
#include <QMessageBox>

namespace CD
{
    struct SSelectedInfo
    {
        SSelectedInfo()
        {
            memset(this, 0, sizeof(*this));
        }
        CBaseObject* m_pObj;
        ModelCompiler* m_pCompiler;
        Model* m_pModel;
    };

    enum EMouseAction
    {
        eMouseAction_Nothing,
        eMouseAction_LButtonDown,
        eMouseAction_LButtonUp,
        eMouseAction_LButtonDoubleClick
    };

    struct SLButtonInfo
    {
        SLButtonInfo()
        {
            m_DownPos = QPoint(0, 0);
            m_LastAction = eMouseAction_Nothing;
            m_DownTimeStamp = 0;
        }
        QPoint m_DownPos;
        EMouseAction m_LastAction;
        UINT m_DownTimeStamp;
    };

    enum EPushPull
    {
        ePP_Push,
        ePP_Pull,
        ePP_None,
    };

    enum EButtonStatus
    {
        eButton_Pressed,
        eButton_Unpressed,
    };

    enum EUpdateType
    {
        eUT_Mesh = 1 << 0,
        eUT_Mirror = 1 << 1,
        eUT_CenterPivot = 1 << 2,
        eUT_DataBase = 1 << 3,
        eUT_GameResource = 1 << 4,
        eUT_SyncPrefab = 1 << 5,

        eUT_All = 0xFFFFFFFF,

        eUT_ExceptMesh = eUT_All & (~eUT_Mesh),
        eUT_ExceptMirror = eUT_All & (~eUT_Mirror),
        eUT_ExceptCenterPivot = eUT_All & (~eUT_CenterPivot),
        eUT_ExceptDataBase = eUT_All & (~eUT_DataBase),
        eUT_ExceptGameResource = eUT_All & (~eUT_GameResource),
        eUT_ExceptSyncPrefab = eUT_All & (~eUT_SyncPrefab),
    };

    void UpdateAll(SMainContext& mc, int updateType = eUT_All);

    bool IsCreationTool(EDesignerTool mode);
    bool IsSelectElementMode(EDesignerTool mode);
    bool IsEdgeSelectMode(EDesignerTool mode);
    DesignerTool* GetDesigner();
    void GetSelectedObjectList(std::vector<SSelectedInfo>& selections);
    void Log(const char* format, ...);
    void SyncPrefab(SMainContext& mc);
    void SyncMirror(Model* pModel);
    void RunTool(EDesignerTool tool);
    void UpdateDrawnEdges(CD::SMainContext& mc);
    void MessageBox(const QString& title, const QString& msg);
    BrushMatrix34 GetOffsetTM(ITransformManipulator* pManipulator, const BrushVec3& vOffset, const BrushMatrix34& worldTM);
}