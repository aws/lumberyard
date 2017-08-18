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

#include "StdAfx.h"
#include "BSPTree2D.h"

namespace CD
{
    class BSPTree2DNode
        : public CRefCountBase
    {
    public:

        BSPTree2DNode();
        ~BSPTree2DNode();
        CD::EPointPosEnum IsVertexIn(const BrushVec3& vertex) const;
        CD::EIntersectionType HasIntersection(const BrushEdge3D& edge) const;
        bool IsInside(const BrushEdge3D& edge, bool bCheckCoDiff) const;
        bool IsOnEdge(const BrushEdge3D& edge) const;
        void GetPartitions(const BrushEdge3D& inEdge, BSPTree2D::SOutputEdges& outEdges) const;
        void SetPlane(const BrushPlane& plane){ m_Plane = plane; }
        const BrushPlane& GetPlane() const { return m_Plane; }
        const BrushEdge3D::List& GetCoincidentEdgeList() const{ return m_coEdge3DList;  }
        void AddCoincidentEdge(const BrushEdge3D& edge){  m_coEdge3DList.push_back(edge); }
        void SetPositiveChild(BSPTree2DNode* pTree){  m_PosChild = pTree; }
        void SetNegativeChild(BSPTree2DNode* pTree){ m_NegChild = pTree; }
        BSPTree2DNode* GetPositiveChild() const{ return m_PosChild; }
        BSPTree2DNode* GetNegativeChild() const{ return m_NegChild; }
        int GetCoincidentSize() const{ return m_coEdge3DList.size(); }
        const BrushEdge3D& GetCoincidentEdge(int n) const{ return m_coEdge3DList[n]; }
        BrushLine GetLine() const { return BrushLine(m_Plane.W2P(m_coEdge3DList[0].m_v[0]), m_Plane.W2P(m_coEdge3DList[0].m_v[1]));  }
        bool HasNegativeNode() const
        {
            if (m_NegChild)
            {
                return true;
            }
            if (m_PosChild)
            {
                return m_PosChild->HasNegativeNode();
            }
            return false;
        }

    private:
        void GetPositivePartitions(const BrushEdge3D& inEdge, BSPTree2D::SOutputEdges& outEdges) const;
        void GetNegativePartitions(const BrushEdge3D& inEdge, BSPTree2D::SOutputEdges& outEdges) const;

    private:

        BrushPlane m_Plane;
        BrushEdge3D::List m_coEdge3DList;
        BSPTree2DNode* m_PosChild;
        BSPTree2DNode* m_NegChild;
    };

    BSPTree2DNode::BSPTree2DNode()
        : m_PosChild(NULL)
        , m_NegChild(NULL)
    {
    }

    BSPTree2DNode::~BSPTree2DNode()
    {
        if (m_PosChild)
        {
            delete m_PosChild;
        }
        if (m_NegChild)
        {
            delete m_NegChild;
        }
    }

    CD::EPointPosEnum BSPTree2DNode::IsVertexIn(const BrushVec3& vertex) const
    {
        BrushFloat distance(GetLine().Distance(m_Plane.W2P(vertex)));

        if (distance > kDesignerEpsilon)
        {
            if (m_PosChild)
            {
                return m_PosChild->IsVertexIn(vertex);
            }
            else
            {
                return CD::ePP_OUTSIDE;
            }
        }
        else if (distance < -kDesignerEpsilon)
        {
            if (m_NegChild)
            {
                return m_NegChild->IsVertexIn(vertex);
            }
            else
            {
                return CD::ePP_INSIDE;
            }
        }
        else
        {
            if (m_PosChild || m_NegChild)
            {
                for (int i = 0, iCoEdgeSize(m_coEdge3DList.size()); i < iCoEdgeSize; ++i)
                {
                    BrushVec3 v;
                    if (CD::IsEquivalent(m_coEdge3DList[i].m_v[0], vertex) || CD::IsEquivalent(m_coEdge3DList[i].m_v[1], vertex))
                    {
                        return CD::ePP_BORDER;
                    }
                }

                if (m_PosChild && m_PosChild->IsVertexIn(vertex) != CD::ePP_OUTSIDE ||
                    m_NegChild && m_NegChild->IsVertexIn(vertex) != CD::ePP_OUTSIDE)
                {
                    return CD::ePP_BORDER;
                }
                return CD::ePP_OUTSIDE;
            }
            return CD::ePP_BORDER;
        }
    }

    CD::EIntersectionType BSPTree2DNode::HasIntersection(const BrushEdge3D& edge) const
    {
        BSPTree2D::SOutputEdges outEdges;
        GetPartitions(edge, outEdges);
        if (!outEdges.negList.empty())
        {
            return CD::eIT_Intersection;
        }
        if (!outEdges.coSameList.empty() || !outEdges.coDiffList.empty())
        {
            return CD::eIT_JustTouch;
        }
        return CD::eIT_None;
    }

    bool BSPTree2DNode::IsInside(const BrushEdge3D& edge, bool bCheckCoDiff) const
    {
        BSPTree2D::SOutputEdges outEdges;
        GetPartitions(edge, outEdges);
        if (bCheckCoDiff)
        {
            return outEdges.posList.empty() && outEdges.coDiffList.empty();
        }
        return outEdges.posList.empty();
    }

    bool BSPTree2DNode::IsOnEdge(const BrushEdge3D& edge) const
    {
        BSPTree2D::SOutputEdges outEdges;
        GetPartitions(edge, outEdges);
        return outEdges.posList.empty() && outEdges.negList.empty() && (!outEdges.coSameList.empty() || !outEdges.coDiffList.empty());
    }

    void BSPTree2DNode::GetPartitions(const BrushEdge3D& inEdge, BSPTree2D::SOutputEdges& outEdges) const
    {
        BrushEdge3D posEdge;
        BrushEdge3D negEdge;

        DESIGNER_ASSERT(!m_coEdge3DList.empty());
        if (m_coEdge3DList.empty())
        {
            return;
        }

        ESplitResult type = CD::Split(m_Plane, GetLine(), m_coEdge3DList, inEdge, posEdge, negEdge);

        if (type == eSR_CROSS)
        {
            GetPositivePartitions(posEdge, outEdges);
            GetNegativePartitions(negEdge, outEdges);
        }
        else if (type == eSR_POSITIVE)
        {
            GetPositivePartitions(posEdge, outEdges);
        }
        else if (type == eSR_NEGATIVE)
        {
            GetNegativePartitions(negEdge, outEdges);
        }
        else
        {
            BrushEdge3D::List intersectionEdges;
            std::vector<BrushLine3D> intersectionLines;

            for (int i = 0, iSize(m_coEdge3DList.size()); i < iSize; ++i)
            {
                BrushEdge3D outE;
                EOperationResult intersectionResult(CD::IntersectEdge3D(m_coEdge3DList[i], inEdge, outE));
                if (intersectionResult == eOR_One)
                {
                    if (!outE.IsPoint())
                    {
                        intersectionEdges.push_back(outE);
                        intersectionLines.push_back(BrushLine3D(m_coEdge3DList[i].m_v[0], m_coEdge3DList[i].m_v[1]));
                    }
                }
            }

            BrushEdge3D::List subtractedEdges;
            if (!intersectionEdges.empty())
            {
                for (int i = 0, iSize(intersectionEdges.size()); i < iSize; ++i)
                {
                    if (intersectionEdges[i].IsSameFacing(intersectionLines[i]))
                    {
                        outEdges.coSameList.push_back(intersectionEdges[i]);
                    }
                    else
                    {
                        outEdges.coDiffList.push_back(intersectionEdges[i]);
                    }
                }

                std::vector<BrushEdge3D> resultEdges[2];
                int nTurn = 0;
                resultEdges[nTurn].push_back(inEdge);
                for (int i = 0, iSize(intersectionEdges.size()); i < iSize; ++i)
                {
                    BrushEdge3D e[2];
                    for (int a = 0; a < resultEdges[nTurn].size(); ++a)
                    {
                        EOperationResult res(CD::SubtractEdge3D(resultEdges[nTurn][a], intersectionEdges[i], e));
                        if (res != eOR_Invalid)
                        {
                            for (int k = 0; k < (int)res; ++k)
                            {
                                if (!e[k].IsPoint())
                                {
                                    resultEdges[1 - nTurn].push_back(e[k]);
                                }
                            }
                        }
                    }
                    resultEdges[nTurn].clear();
                    nTurn = 1 - nTurn;
                }

                for (int i = 0, iSize(resultEdges[nTurn].size()); i < iSize; ++i)
                {
                    BrushFloat d0 = GetLine().Distance(m_Plane.W2P(resultEdges[nTurn][i].m_v[0]));
                    BrushFloat d1 = GetLine().Distance(m_Plane.W2P(resultEdges[nTurn][i].m_v[1]));

                    if (d0 < 0 && d1 <= 0 || d0 <= 0 && d1 < 0)
                    {
                        GetNegativePartitions(resultEdges[nTurn][i], outEdges);
                    }
                    else
                    {
                        GetPositivePartitions(resultEdges[nTurn][i], outEdges);
                    }
                }
            }
            else
            {
                BrushFloat d0 = GetLine().Distance(m_Plane.W2P(inEdge.m_v[0]));
                BrushFloat d1 = GetLine().Distance(m_Plane.W2P(inEdge.m_v[1]));
                if (d0 < 0 && d1 <= 0 || d0 <= 0 && d1 < 0)
                {
                    GetNegativePartitions(inEdge, outEdges);
                }
                else
                {
                    GetPositivePartitions(inEdge, outEdges);
                }
            }
        }
    }

    void BSPTree2DNode::GetPositivePartitions(const BrushEdge3D& inEdge, BSPTree2D::SOutputEdges& outEdges) const
    {
        if (m_PosChild)
        {
            m_PosChild->GetPartitions(inEdge, outEdges);
        }
        else
        {
            outEdges.posList.push_back(inEdge);
        }
    }

    void BSPTree2DNode::GetNegativePartitions(const BrushEdge3D& inEdge, BSPTree2D::SOutputEdges& outEdges) const
    {
        if (m_NegChild)
        {
            m_NegChild->GetPartitions(inEdge, outEdges);
        }
        else
        {
            outEdges.negList.push_back(inEdge);
        }
    }

    BSPTree2D::~BSPTree2D()
    {
        if (m_pRootNode)
        {
            delete m_pRootNode;
        }
    }

    BSPTree2DNode* BSPTree2D::ConstructTree(const BrushPlane& plane, const std::vector<BrushEdge3D>& edgeList)
    {
        DESIGNER_ASSERT(!edgeList.empty());

        if (edgeList.empty())
        {
            return NULL;
        }

        BSPTree2DNode* pTree = new BSPTree2DNode;

        pTree->AddCoincidentEdge(edgeList[0]);
        pTree->SetPlane(plane);

        std::vector<BrushEdge3D> posList, negList;

        for (int i = 1, edgeLineSize(edgeList.size()); i < edgeLineSize; ++i)
        {
            BrushEdge3D posEdge;
            BrushEdge3D negEdge;

            ESplitResult type = CD::Split(pTree->GetPlane(), pTree->GetLine(), pTree->GetCoincidentEdgeList(), edgeList[i], posEdge, negEdge);

            if (type == eSR_CROSS)
            {
                posList.push_back(posEdge);
                negList.push_back(negEdge);
            }
            else if (type == eSR_POSITIVE)
            {
                posList.push_back(posEdge);
            }
            else if (type == eSR_NEGATIVE)
            {
                negList.push_back(negEdge);
            }
            else if (type == eSR_COINCIDENCE)
            {
                if (CD::IsEquivalent(edgeList[i].m_v[0], edgeList[0].m_v[1]) && CD::IsEquivalent(edgeList[i].m_v[1], edgeList[0].m_v[0]))
                {
                    posList.push_back(edgeList[i]);
                }
                else
                {
                    pTree->AddCoincidentEdge(edgeList[i]);
                }
            }
        }

        BSPTree2DNode* childTree = NULL;
        if (!posList.empty())
        {
            childTree = ConstructTree(plane, posList);
            pTree->SetPositiveChild(childTree);
        }

        if (!negList.empty())
        {
            childTree = ConstructTree(plane, negList);
            pTree->SetNegativeChild(childTree);
        }

        return pTree;
    }

    void BSPTree2D::BuildTree(const BrushPlane& plane, const std::vector<BrushEdge3D>& edgeList)
    {
        if (m_pRootNode)
        {
            delete m_pRootNode;
        }
        m_pRootNode = ConstructTree(plane, edgeList);
    }

    void BSPTree2D::GetEdgeList(BSPTree2DNode* pTree, BrushEdge3D::List& outEdgeList)
    {
        if (pTree == NULL)
        {
            return;
        }

        DESIGNER_ASSERT(pTree->GetCoincidentSize() > 0);

        outEdgeList.push_back(pTree->GetCoincidentEdge(0));

        GetEdgeList(pTree->GetPositiveChild(), outEdgeList);
        GetEdgeList(pTree->GetNegativeChild(), outEdgeList);
    }

    CD::EPointPosEnum BSPTree2D::IsVertexIn(const BrushVec3& vertex) const
    {
        return m_pRootNode->IsVertexIn(vertex);
    }

    CD::EIntersectionType BSPTree2D::HasIntersection(const BrushEdge3D& edge) const
    {
        return m_pRootNode->HasIntersection(edge);
    }

    bool BSPTree2D::IsInside(const BrushEdge3D& edge, bool bCheckCoDiff) const
    {
        return m_pRootNode->IsInside(edge, bCheckCoDiff);
    }

    bool BSPTree2D::IsOnEdge(const BrushEdge3D& edge) const
    {
        return m_pRootNode->IsOnEdge(edge);
    }

    void BSPTree2D::GetPartitions(const BrushEdge3D& inEdge, SOutputEdges& outEdges) const
    {
        return m_pRootNode->GetPartitions(inEdge, outEdges);
    }

    bool BSPTree2D::HasNegativeNode() const
    {
        if (m_pRootNode)
        {
            return m_pRootNode->HasNegativeNode();
        }
        return false;
    }
};