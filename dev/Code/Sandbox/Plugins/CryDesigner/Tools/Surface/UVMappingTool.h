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
//  File name:   TextureMappingTool.h
//  Created:     May/6/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/Select/SelectTool.h"

class UVMappingTool;

namespace CD
{
    class Model;

    enum ETexParamFlag
    {
        eTexParam_Offset  = 0x01,
        eTexParam_Scale   = 0x02,
        eTexParam_Rotate  = 0x04,
        eTexParam_All     = 0xFF
    };

    struct STextureMappingParameter
    {
        Vec2 m_UVOffset;
        Vec2 m_ScaleOffset;
        float m_Rotate;
        Vec2 m_TilingXY;

        UVMappingTool* m_pUVMappingTool;

        STextureMappingParameter()
            : m_UVOffset(Vec2(0, 0))
            , m_ScaleOffset(Vec2(1, 1))
            , m_Rotate(0)
            , m_TilingXY(Vec2(1, 1))
            , m_pUVMappingTool(NULL)
        {
        }

        void Serialize(Serialization::IArchive& ar);
    };
}

class UVMappingTool
    : public SelectTool
{
public:

    UVMappingTool(CD::EDesignerTool tool)
        : SelectTool(tool)
    {
        m_TexMappingParameter.m_pUVMappingTool = this;
        m_nPickFlag = CD::ePF_Face;
    }

    void Enter() override;
    void Leave() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    void ApplyTextureInfo(const CD::STexInfo& texInfo, int nModifiedParts);
    void FitTexture(float fTileU, float fTileV);
    void SetSubMatID(int nSubMatID) override;

    void SelectPolygonsByMatID(int matID);
    bool GetTexInfoOfSelectedPolygon(CD::STexInfo& outTexInfo) const;

    void OnManipulatorDrag(CViewport* pView, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const BrushVec3& value) override;
    void OnManipulatorMouseEvent(CViewport* pView, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo) override;

    bool IsCircleTypeRotateGizmo() override { return true; }
    void RecordTextureMappingUndo(const char* sUndoDescription) const;

    void ShowGizmo();

    void OnFitTexture();
    void OnReset();
    void OnSelectPolygons();
    void OnAssignSubMatID();
    void OnCopy();
    void OnPaste();

    void UpdatePanel(const CD::STexInfo& texInfo);
    void OpenUVMappingWnd();

    void Serialize(Serialization::IArchive& ar){m_TexMappingParameter.Serialize(ar); }
    void OnChangeParameter(bool continuous) override;

public:

    static void CloseTextureMappingToolPanel();
    static int m_nDesignerTextureMappingToolPanelId;

private:

    bool QueryPolygon(const BrushVec3& localRaySrc, const BrushVec3& localRayDir, int& nOutPolygonIndex, bool& bOutNew) const;
    void SetTexInfoToPolygon(CD::PolygonPtr pPolygon, const CD::STexInfo& texInfo, int nModifiedParts);
    void MoveSelectedElements();

private:

    struct STextureContext
    {
        void Init()
        {
            m_TexInfos.clear();
        }

        BrushVec3 m_MouseDownPos;
        std::vector< std::pair<CD::PolygonPtr, CD::STexInfo> > m_TexInfos;
    };
    STextureContext m_MouseDownContext;
    CD::STextureMappingParameter m_TexMappingParameter;
    CD::STextureMappingParameter m_PrevMappingParameter;
    CD::STextureMappingParameter m_CopiedTexMappingParameter;
};