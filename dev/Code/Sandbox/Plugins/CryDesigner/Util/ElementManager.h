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

#include "Core/Polygon.h"
#include "Core/ModelDB.h"

class CBaseObject;

struct SElement
{
    SElement()
        : m_bIsolated(false) {}
    SElement(CBaseObject* pObject, CD::PolygonPtr pPolygon)
        : m_bIsolated(false)
    {
        SetFace(pObject, pPolygon);
    }
    SElement(CBaseObject* pObject, const BrushEdge3D& edge)
        : m_bIsolated(false)
    {
        SetEdge(pObject, edge);
    }
    SElement(CBaseObject* pObject, const BrushVec3& vertex)
        : m_bIsolated(false)
    {
        SetVertex(pObject, vertex);
    }
    SElement GetMirroredElement(const BrushPlane& mirrorPlane) const;
    bool operator == (const SElement& info);
    void Invalidate();
    bool IsEquivalent(const SElement& elementInfo) const;

    bool IsVertex() const {return m_Vertices.size() == 1; }
    bool IsEdge()   const {return m_Vertices.size() == 2; }
    bool IsFace()   const {return m_Vertices.size() >= 3; }

    BrushVec3 GetPos() const
    {
        assert(IsVertex());
        if (!IsVertex())
        {
            return BrushVec3(0, 0, 0);
        }
        return m_Vertices[0];
    }

    BrushEdge3D GetEdge() const
    {
        assert(IsEdge());
        if (!IsEdge())
        {
            return BrushEdge3D(BrushVec3(0, 0, 0), BrushVec3(0, 0, 0));
        }
        return BrushEdge3D(m_Vertices[0], m_Vertices[1]);
    }

    void SetFace(CBaseObject* pObject, CD::PolygonPtr pPolygon)
    {
        m_pPolygon = pPolygon;
        int iVertexSize(pPolygon->GetVertexCount());
        m_Vertices.reserve(iVertexSize);
        for (int i = 0; i < iVertexSize; ++i)
        {
            m_Vertices.push_back(pPolygon->GetPos(i));
        }
        m_pObject = pObject;
    }

    void SetEdge(CBaseObject* pObject, const BrushEdge3D& edge)
    {
        m_pPolygon = NULL;
        m_Vertices.resize(2);
        m_Vertices[0] = edge.m_v[0];
        m_Vertices[1] = edge.m_v[1];
        m_pObject = pObject;
    }

    void SetVertex(CBaseObject* pObject, const BrushVec3& vertex)
    {
        m_pPolygon = NULL;
        m_Vertices.resize(1);
        m_Vertices[0] = vertex;
        m_pObject = pObject;
    }

    ~SElement(){}

    std::vector<BrushVec3> m_Vertices;
    CD::PolygonPtr m_pPolygon;
    _smart_ptr<CBaseObject> m_pObject;
    bool m_bIsolated;
};

class ElementManager
    : public CRefCountBase
{
public:

    void Clear()
    {
        m_Elements.clear();
    }

    int GetCount() const
    {
        return (int)m_Elements.size();
    }

    int GetFaceCount() const;
    int GetEdgeCount() const;
    int GetVertexCount() const;

    ElementManager(){}
    ~ElementManager(){}

    ElementManager(const ElementManager& elementManager)
    {
        operator =(elementManager);
    }

    ElementManager& operator =(const ElementManager& elementManager)
    {
        m_Elements = elementManager.m_Elements;
        return *this;
    }

    const SElement& Get(int nIndex) const { return m_Elements[nIndex]; }
    SElement& operator [] (int nIndex) { return m_Elements[nIndex]; }

    void Set(int nIndex, const SElement& element)
    {
        m_Elements[nIndex] = element;
    }

    void Set(const ElementManager& elements)
    {
        operator = (elements);
    }

    bool IsEmpty() const { return m_Elements.empty(); }

    bool Add(ElementManager& elements);
    bool Add(const SElement& element);

    bool Pick(CBaseObject* pObject, CD::Model* pModel, CViewport* viewport, const QPoint& point, int nFlag, bool bOnlyIncludeCube, BrushVec3* pOutPickedPos);

    void Display(CBaseObject* pObject, DisplayContext& dc) const;
    void DisplayHighlightElements(CBaseObject* pObject, CD::Model* pModel, DisplayContext& dc, int nPickFlag) const;
    void DisplayVertexElements(CBaseObject* pObject, CD::Model* pModel, DisplayContext& dc, int nShelf = -1, std::vector<BrushVec3>* pExcludedVertices = NULL) const;
    void DisplayFaceElements(CBaseObject* pObject, CD::Model* pModel, DisplayContext& dc) const;

    void PickAdjacentCurvedEdges(CBaseObject* pObject, CD::PolygonPtr pPolygon, const BrushEdge3D& edge, std::vector<SElement>* outPickInfo) const;
    void FindElementsInRect(const QRect& rect, CViewport* pView, const Matrix34& worldTM, bool bOnlyUseSelectionCube, std::vector<SElement>& elements) const;

    BrushVec3 GetNormal(CD::Model* pModel) const;
    void Erase(int nElementFlags);
    void Erase(const SElement& element);
    bool Erase(ElementManager& elements);

    bool Has(const SElement& elementInfo) const;
    bool HasVertex(const BrushVec3& vertex) const;
    QString GetElementsInfoText();
    void RemoveInvalidElements();

    void ApplyOffset(const BrushVec3& vOffset);

    CD::ModelDB::QueryResult QueryFromElements(CD::Model* pModel) const;
    bool QueryNearestVertex(CBaseObject* pObject, CD::Model* pModel, CViewport* pView, const QPoint& point, const BrushVec3& rayLocalSrc, const BrushVec3& rayLocalDir, BrushVec3& outPos, BrushVec3* pOutNormal = NULL) const;

private:

    bool HasPolygonSelected(CD::PolygonPtr pPolygon) const;
    CD::PolygonPtr PickPolygonFromRepresentativeBox(CBaseObject* pObject, CD::Model* pModel, CViewport* pView, const QPoint& point, const BrushVec3& rayLocalSrc, const BrushVec3& rayLocalDir, BrushVec3& outPickedPos) const;

    std::vector<SElement> m_Elements;
};

typedef _smart_ptr<ElementManager> DesignerElementsPtr;