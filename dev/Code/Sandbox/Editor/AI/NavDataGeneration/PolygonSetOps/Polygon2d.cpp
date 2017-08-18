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
#include "Polygon2d.h"
#include "BspTree2d.h"
#include "../AICollision.h"

Polygon2d::Edge::Edge (int ind0, int ind1)
    : m_vertIndex0 (ind0)
    , m_vertIndex1 (ind1)
{
}

bool Polygon2d::Edge::operator< (const Edge& rhs) const
{
    if (m_vertIndex1 < rhs.m_vertIndex1)
    {
        return true;
    }
    if (m_vertIndex1 == rhs.m_vertIndex1)
    {
        return m_vertIndex0 < rhs.m_vertIndex0;
    }
    return false;
}

bool Polygon2d::Edge::operator== (const Edge& rhs) const
{
    return m_vertIndex0 == rhs.m_vertIndex0 && m_vertIndex1 == rhs.m_vertIndex1;
}


// ---


Polygon2d::Polygon2d ()
    : m_bsp (0)
{
}

Polygon2d::Polygon2d (const Polygon2d& rhs)
    : m_bsp (0)
{
    *this = rhs;
}

Polygon2d::Polygon2d (const LineSegVec& edges)
    : m_bsp (0)
{
    LineSegVec::const_iterator it = edges.begin ();
    LineSegVec::const_iterator end = edges.end ();
    for (; it != end; ++it)
    {
        AddEdge (it->Start (), it->End ());
    }
}

Polygon2d::~Polygon2d ()
{
    delete m_bsp;
}

Polygon2d& Polygon2d::operator= (const Polygon2d& rhs)
{
    if (this == &rhs)
    {
        return *this;
    }

    delete m_bsp;
    m_bsp = (rhs.m_bsp ? rhs.m_bsp->Clone () : 0);

    m_vertices = rhs.m_vertices;
    m_edges = rhs.m_edges;

    return *this;
}

int Polygon2d::NumVertices () const
{
    return m_vertices.Size ();
}

int Polygon2d::AddVertex (const Vector2d& new_vertex)
{
    BidirectionalMap<int, Vector2d>::Iterator it = m_vertices.Find (new_vertex);
    if (it != m_vertices.End ())
    {
        return (*it)->first;
    }

    int index = m_vertices.Size ();
    m_vertices.Insert (index, new_vertex);
    return index;
}

bool Polygon2d::GetVertex (int i, Vector2d& vertex) const
{
    if (i < 0 || i >= m_vertices.Size ())
    {
        return false;
    }

    vertex = m_vertices[i];
    return true;
}

int Polygon2d::NumEdges () const
{
    return m_edges.Size ();
}

int Polygon2d::AddEdge (const Edge& new_edge)
{
    assert (new_edge.m_vertIndex0 != new_edge.m_vertIndex1);

    BidirectionalMap<int, Edge>::Iterator eIt = m_edges.Find (new_edge);
    if (eIt != m_edges.End ())
    {
        return (*eIt)->first;
    }

    int index = m_edges.Size ();
    m_edges.Insert (index, new_edge);
    return index;
}

int Polygon2d::AddEdge (const Vector2d& v0, const Vector2d& v1)
{
    Edge new_edge;
    new_edge.m_vertIndex0 = AddVertex (v0);
    new_edge.m_vertIndex1 = AddVertex (v1);
    return AddEdge (new_edge);
}

bool Polygon2d::GetEdge (int i, Edge& edge) const
{
    if (i < 0 || i >= m_edges.Size ())
    {
        return false;
    }

    edge = m_edges[i];
    return true;
}

void Polygon2d::ComputeBspTree () const
{
    delete m_bsp;

    if (m_edges.Empty ())
    {
        return;
    }

    LineSegVec edges;
    BidirectionalMap <int, Edge>::Iterator it = m_edges.Begin ();
    BidirectionalMap <int, Edge>::Iterator end = m_edges.End ();
    for (; it != end; ++it)
    {
        const Edge& edge = (*it)->second;
        edges.push_back (LineSeg (m_vertices[edge.m_vertIndex0], m_vertices[edge.m_vertIndex1]));
    }
    m_bsp = new BspTree2d (edges);
}

/**
 * Once this function finishes, the splitter contains (pieces of) this
 * polygon's edges that are "inside" the tree stored separately from those
 * that are "outside".
 *
 * It is assumed that the caller primed the splitter with a BSP corresponding
 * to another polygon.  If so then splitter.LeftSegs() contains upon return
 * (parts of) this polygon's edges that are inside the polygon from which
 * the splitter's BSP was constructed.
 */
void Polygon2d::Cut (BspLineSegSplitter& splitter) const
{
    BidirectionalMap<int, Edge>::Iterator it = m_edges.Begin ();
    BidirectionalMap<int, Edge>::Iterator end = m_edges.End ();
    for (; it != end; ++it)
    {
        const Edge& e = (*it)->second;

        Vector2d v0;
        bool result = GetVertex (e.m_vertIndex0, v0);
        assert (result);

        Vector2d v1;
        result = GetVertex (e.m_vertIndex1, v1);
        assert (result);

        splitter.Split (LineSeg (v0, v1));
    }
}

/**
 * Negation is achieved by just flipping edge directions and inverting a cached
 * BSP if any.
 */
Polygon2d Polygon2d::operator~ () const
{
    Polygon2d negated;

    negated.m_vertices = m_vertices;
    BidirectionalMap<int, Edge>::Iterator it = m_edges.Begin ();
    BidirectionalMap<int, Edge>::Iterator end = m_edges.End ();
    for (; it != end; ++it)
    {
        negated.AddEdge (Edge ((*it)->second.m_vertIndex1, (*it)->second.m_vertIndex0));
    }

    negated.m_bsp = m_bsp ? m_bsp->Clone () : 0;
    if (negated.m_bsp)
    {
        negated.m_bsp->Invert();
    }
    return negated;
}

Polygon2d Polygon2d::operator& (const Polygon2d& rhs) const
{
    if (!m_bsp)
    {
        ComputeBspTree ();
    }
    if (!rhs.m_bsp)
    {
        rhs.ComputeBspTree ();
    }

    // cut 'rhs' by 'this' BSP tree
    BspLineSegSplitter lineSegSplitter (m_bsp);
    rhs.Cut (lineSegSplitter);

    // use the same splitter to cut 'this' by 'rhs' BSP tree
    lineSegSplitter.ResetBspTree (rhs.m_bsp);
    Cut (lineSegSplitter);

    // NOTE Jul 5, 2007: <pvl> ComputeBspTree()?
    // if your prayers didn't go unheeded, LeftSegs() now has all edges of the intersection
    return Polygon2d (lineSegSplitter.LeftSegs ());
}

Polygon2d Polygon2d::operator| (const Polygon2d& rhs) const
{
    // union
    return ~(~*this & ~rhs);
}

Polygon2d Polygon2d::operator- (const Polygon2d& rhs) const
{
    // difference
    return *this & ~rhs;
}

Polygon2d Polygon2d::operator^ (const Polygon2d& rhs) const
{
    // exclusive or
    return (*this - rhs) | (rhs - *this);
}


struct SEdgeFinder
{
    SEdgeFinder(const Polygon2d::Edge& edge)
        : edge(edge) {}
    bool operator()(const Polygon2d::Edge& other) {return edge.m_vertIndex1 == other.m_vertIndex0; }
    Polygon2d::Edge edge;
};

//===================================================================
// CalculateContours
//===================================================================
void Polygon2d::CalculateContours(bool removeInterior)
{
    m_contours.clear();

    std::vector<Edge> edgeArrayCopy;
    m_edges.GetSecondaryKeys ().swap (edgeArrayCopy);

    std::vector <std::vector<Edge> > contours;

    while (!edgeArrayCopy.empty())
    {
        contours.push_back(std::vector<Edge>());
        std::vector<Edge>& currentEdgeArray = contours.back();
        currentEdgeArray.push_back(edgeArrayCopy.back());
        edgeArrayCopy.pop_back();

        std::vector<Edge>::iterator it;
        while ((it = std::find_if(edgeArrayCopy.begin(), edgeArrayCopy.end(), SEdgeFinder(currentEdgeArray.back()))) != edgeArrayCopy.end())
        {
            currentEdgeArray.push_back(*it);
            edgeArrayCopy.erase(it);
        }
        if (currentEdgeArray.front().m_vertIndex0 != currentEdgeArray.back().m_vertIndex1)
        {
            int breakMe = 1;
        }
    }

    float epsilon = 0.01f;
    for (unsigned iContour = 0; iContour < contours.size(); ++iContour)
    {
        m_contours.push_back(std::vector<Vector2d>());
        std::vector<Vector2d>& lastContour = m_contours.back();
        const std::vector<Edge>& contour = contours[iContour];
        for (unsigned iEdge = 0; iEdge < contour.size(); ++iEdge)
        {
            const Edge& edge = contour[iEdge];
            Vector2d pt;
            bool result = GetVertex(edge.m_vertIndex0, pt);
            if (result)
            {
                const Vector2d vec2Zero(0, 0);
                const Vector2d& lastPt = lastContour.empty() ? vec2Zero : lastContour.back();
                if (lastContour.empty() || fabs(pt.x - lastPt.x) + fabs(pt.y - lastPt.y) > epsilon)
                {
                    lastContour.push_back(pt);
                }
                else
                {
                    int breakMe = 1;
                }
            }
            else
            {
                int breakMe = 0;
            }
        }
        // need to remove identical points at the start/end (wrapping around)
        while (lastContour.size() > 1 && fabs(lastContour.front().x - lastContour.back().x) + fabs(lastContour.front().y - lastContour.back().y) <= epsilon)
        {
            lastContour.pop_back();
        }

        if (lastContour.size() < 3)
        {
            m_contours.pop_back();
        }
    }

    if (contours.size() > 1)
    {
        int breakMe = 1;
    }

    // if only one contour then it should already be wound correctly (but sometimes not). If there are
    // multiple contours then interior removal requires them to be anti-clockwise wound. If there
    // are multiple contours (rare) then it is probably safer to ensure they're anti-c wound anyway
    for (unsigned iContour = 0; iContour < m_contours.size(); ++iContour)
    {
        std::vector<Vector2d>& contour = m_contours[iContour];
        if (contour.size() > 1000)
        {
            int breakHere = 1;
        }
        EnsureShapeIsWoundAnticlockwise<std::vector<Vector2d>, double>(contour);
    }

    if (removeInterior)
    {
        // assume contours do not overlap! So just check the first point of each is inside any others
        for (int iContour = 0; iContour < m_contours.size(); ++iContour)
        {
            std::vector<Vector2d>& contour = m_contours[iContour];
            assert(contour.size() > 2);
            const Vector2d& pt = contour.front();
            for (int iOther = iContour + 1; iOther < m_contours.size(); ++iOther)
            {
                std::vector<Vector2d>& other = m_contours[iOther];
                assert(other.size() > 2);
                Vector2d& ptOther = other.front();
                if (Overlap::Point_Polygon2D(ptOther, contour))
                {
                    m_contours.erase(m_contours.begin() + iOther);
                    --iOther;
                }
                else if (Overlap::Point_Polygon2D(pt, other))
                {
                    m_contours.erase(m_contours.begin() + iContour);
                    iContour = 0;
                    iOther = 0;
                }
            }
        }
    }
}

//===================================================================
// GetContourQuantity
//===================================================================
unsigned Polygon2d::GetContourQuantity() const
{
    return m_contours.size();
}

//===================================================================
// GetContour
//===================================================================
unsigned Polygon2d::GetContour(unsigned i, const Vector2d** ppPts) const
{
    *ppPts = 0;
    if (i >= m_contours.size())
    {
        return 0;
    }
    if (m_contours[i].empty())
    {
        return 0;
    }

    *ppPts = &m_contours[i][0];
    return m_contours[i].size();
}

//===================================================================
// CollapseVertices
//===================================================================
void Polygon2d::CollapseVertices(double tol)
{
    std::vector<Vector2d> newVArray;
    std::vector<Edge> newEArray;
    m_vertices.GetSecondaryKeys ().swap (newVArray);
    m_edges.GetSecondaryKeys ().swap (newEArray);

    for (unsigned i = 0; i < newVArray.size(); ++i)
    {
        const Vector2d& ptI = newVArray[i];
        for (unsigned j = i + 1; j < newVArray.size(); ++j)
        {
            const Vector2d& ptJ = newVArray[j];
            if (fabs(ptI.x - ptJ.x) + fabs(ptI.y - ptJ.y) < tol)
            {
                newVArray.erase(newVArray.begin() + j);
                for (unsigned iEdge = 0; iEdge < newEArray.size(); ++iEdge)
                {
                    Edge& edge = newEArray[iEdge];
                    if (edge.m_vertIndex0 == j)
                    {
                        edge.m_vertIndex0 = i;
                    }
                    else if (edge.m_vertIndex0 > (int) j)
                    {
                        --edge.m_vertIndex0;
                    }
                    if (edge.m_vertIndex1 == j)
                    {
                        edge.m_vertIndex1 = i;
                    }
                    else if (edge.m_vertIndex1 > (int) j)
                    {
                        --edge.m_vertIndex1;
                    }
                }
                j = i;
            }
        }
    }

    std::sort(newEArray.begin(), newEArray.end());
    newEArray.erase(std::unique(newEArray.begin(), newEArray.end()), newEArray.end());

    m_vertices.Clear ();
    m_edges.Clear ();

    for (unsigned i = 0; i < newVArray.size(); ++i)
    {
        AddVertex(newVArray[i]);
    }
    for (unsigned i = 0; i < newEArray.size(); ++i)
    {
        if (newEArray[i].m_vertIndex0 != newEArray[i].m_vertIndex1)
        {
            AddEdge(newEArray[i]);
        }
    }
}

//===================================================================
// IsWoundAnticlockwise
//===================================================================
bool Polygon2d::IsWoundAnticlockwise() const
{
    double totalCross = 0.0;
    const int nEdges = NumEdges ();
    Edge edge;
    Vector2d pt0, pt1;
    for (int iEdge = 0; iEdge < nEdges; ++iEdge)
    {
        GetEdge(iEdge, edge);
        GetVertex(edge.m_vertIndex0, pt0);
        GetVertex(edge.m_vertIndex1, pt1);
        totalCross += pt0.x * pt1.y - pt1.x * pt0.y;
    }
    return totalCross > 0;
}
