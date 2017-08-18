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
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BaseTool.h
//  Created:     8/12/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Core/Plane.h"
#include "Core/Polygon.h"
#include "Core/ModelCompiler.h"
#include "Tools/ToolCommon.h"
#include "IDataBaseManager.h"

class DesignerTool;
struct ITransformManipulator;
struct IDisplayViewport;
class ElementManager;
class IBasePanel;

class BaseTool
    : public CRefCountBase
{
public:

    CD::EDesignerTool Tool() const { return m_Tool; }
    virtual const char* ToolClass() const;
    const char* ToolName() const;

public:

    BaseTool(CD::EDesignerTool tool)
        : m_Tool(tool)
        , m_Plane(BrushPlane(BrushVec3(0, 0, 1), 0))
        , m_pPickedPolygon(NULL)
        , m_pPanel(NULL)
    {
    }

    virtual ~BaseTool() { EndEditParams(); }

    virtual void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point){}
    virtual void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point){}
    virtual void OnLButtonDblClk(CViewport* view, UINT nFlags, const QPoint& point){}
    virtual void OnRButtonDown(CViewport* view, UINT nFlags, const QPoint& point){}
    virtual void OnRButtonUp(CViewport* view, UINT nFlags, const QPoint& point){}
    virtual void OnMButtonDown(CViewport* view, UINT nFlags, const QPoint& point){}
    virtual void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point);
    virtual void OnMouseWheel(CViewport* view, UINT nFlags, const QPoint& point){}

    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual void Display(DisplayContext& dc);
    virtual void OnManipulatorDrag(CViewport* view, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const BrushVec3& value) {}
    virtual void OnManipulatorMouseEvent(CViewport* view, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo) {}
    virtual void BeginEditParams();
    virtual void EndEditParams();

    virtual void Enter();
    virtual void Leave();

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) {}
    virtual void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event) {}

    virtual QString GetStatusText() const;
    virtual void MaterialChanged() {};
    virtual void SetSubMatID(int nSubMatID) {}

    virtual void OnChangeParameter(bool continuous) {}

    virtual bool IsCircleTypeRotateGizmo() { return false; }
    virtual bool EnabledSeamlessSelection() const { return true; }
    virtual bool IsPhaseFirstStepOnPrimitiveCreation() const { return true; }

public:

    CD::Model* GetModel() const;
    CD::ModelCompiler* GetCompiler() const;
    CBaseObject* GetBaseObject() const;
    CD::SMainContext GetMainContext() const;

    const BrushPlane& GetPlane() const{return m_Plane; }
    void SetPlane(const BrushPlane& plane){m_Plane = plane; }

    IBasePanel* GetPanel() const { return m_pPanel; }

    void DisplayDimensionHelper(DisplayContext& dc, int nShelf = -1);
    void DisplayDimensionHelper(DisplayContext& dc, const AABB& aabb);

    bool IsOverDoubleClickTime(UINT time) const{return GetTickCount() - time > GetDoubleClickTime(); }
    bool IsTwoPointEquivalent(const QPoint& p0, const QPoint& p1) const { return std::abs(p0.x() - p1.x()) > 2 || std::abs(p0.y() - p1.y()) > 2 ? false : true; }

    bool IsModelEmpty() const;
    void UpdateAll(int updateType = CD::eUT_All);

protected:

    BrushMatrix34 GetWorldTM() const;

    CD::PolygonPtr GetPickedPolygon() const { return m_pPickedPolygon; }
    void SetPickedPolygon(CD::PolygonPtr pPolygon){ m_pPickedPolygon = pPolygon; }

    void UpdateShelf(int nShelf);

private:

    CD::PolygonPtr m_pPickedPolygon;
    BrushPlane m_Plane;
    CD::EDesignerTool m_Tool;
    IBasePanel* m_pPanel;
};