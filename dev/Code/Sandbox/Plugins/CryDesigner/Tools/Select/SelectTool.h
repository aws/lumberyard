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
//  File name:   SelectTool.h
//  Created:     July/1/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/BaseTool.h"
#include "Util/ElementManager.h"

class SelectTool
    : public BaseTool
{
public:

    SelectTool(CD::EDesignerTool tool)
        : BaseTool(tool)
        , m_SelectionType(eST_Nothing)
        , m_bHitGizmo(false)
        , m_nPickFlag(0)
    {
        if (CD::IsSelectElementMode(tool))
        {
            if (tool & CD::eDesigner_Vertex)
            {
                m_nPickFlag |= CD::ePF_Vertex;
            }
            if (tool & CD::eDesigner_Edge)
            {
                m_nPickFlag |= CD::ePF_Edge;
            }
            if (tool & CD::eDesigner_Face)
            {
                m_nPickFlag |= CD::ePF_Face;
            }
        }
    }

    virtual ~SelectTool(){}

    virtual void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    virtual void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    virtual void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;

    virtual void Display(DisplayContext& dc) override;
    virtual void Enter() override;

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    void SelectAllElements();

    void AddPickFlag(int nFlag){ m_nPickFlag |= nFlag; }
    void SetPickFlag(int nFlag){ m_nPickFlag = nFlag; }
    void RemovePickFlag(int nFlag){ m_nPickFlag &= ~nFlag; }
    bool CheckPickFlag(int nFlag){ return m_nPickFlag & nFlag ? true : false; }
    int GetPickFlag(){ return m_nPickFlag; }

    QString GetStatusText() const override;

public:

    // first - the index in query result.
    // second - the index in a mark set
    typedef std::pair<int, int> QueryInput;
    typedef std::vector<QueryInput> QueryInputs;
    // first - polygon index, second - query results of a polygon
    typedef std::map<CD::PolygonPtr, QueryInputs> OrganizedQueryResults;

    static OrganizedQueryResults CreateOrganizedResultsAroundPolygonFromQueryResults(const CD::ModelDB::QueryResult& queryResult);

protected:

    enum eSelectionType
    {
        eST_NormalSelection,
        eST_RectangleSelection,
        eST_EraseSelectionInRectangle,
        eST_JustAboutToTransformSelectedElements,
        eST_TransformSelectedElements,
        eST_Nothing
    };
    void UpdateCursor(CViewport* view, bool bPickingElements);

    void SelectElementsInRectangle(CViewport* view, const QPoint& point, bool bOnlyUseSelectionCube);
    void EraseElementsInRectangle(CViewport* view, const QPoint& point, bool bOnlyUseSelectionCube);

    eSelectionType m_SelectionType;
    QPoint m_MouseDownPos;

    ElementManager m_InitialSelectionElementsInRectangleSel;

    int m_nPickFlag;
    bool m_bHitGizmo;
    BrushVec3 m_PickedPosAsLMBDown;
};