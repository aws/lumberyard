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

// Description : Shape containers.


#include "StdAfx.h"
#include "Shape.h"
#include "ShapeContainer.h"
#include "IRenderer.h"
#include "IRenderAuxGeom.h"
#include "AIQuadTree.h"
#include "AILog.h"

// Constructor
CAIShapeContainer::CAIShapeContainer()
    : m_quadTree(0)
{
}

// Destructor
CAIShapeContainer::~CAIShapeContainer()
{
    Clear();
}

// Clear all shapes and supporting structures.
void CAIShapeContainer::Clear()
{
    for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
    {
        delete m_shapes[i];
    }
    m_shapes.clear();
    m_nameToShape.clear();
    delete m_quadTree;
    m_quadTree = 0;
}

// Replace the container with content from specified container.
void CAIShapeContainer::Copy(const CAIShapeContainer& cont)
{
    Clear();
    Insert(cont);
}

// Insert the contents of the specified container.
void CAIShapeContainer::Insert(const CAIShapeContainer& cont)
{
    for (unsigned i = 0, ni = cont.GetShapes().size(); i < ni; ++i)
    {
        CAIShape* pShape = cont.GetShapes()[i];
        InsertShape(pShape->DuplicateData());
    }
}

// Insert new shape, the container will delete the shape when the container cleared.
void CAIShapeContainer::InsertShape(CAIShape* pShape)
{
    m_shapes.push_back(pShape);
    if (!m_nameToShape.insert(NameToShapeMap::iterator::value_type(pShape->GetName(), pShape)).second)
    {
        AIError("CAIShapeContainer::InsertShape Duplicate AI shape (auto-forbidden area?) name %s", pShape->GetName().c_str());
    }
}

// Deletes shape from container and removes any references to it.
void CAIShapeContainer::DeleteShape(const string& name)
{
    NameToShapeMap::iterator it = m_nameToShape.find(name);
    if (it == m_nameToShape.end())
    {
        return;
    }
    CAIShape* pShape = it->second;
    m_nameToShape.erase(it);
    for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
    {
        if (m_shapes[i] == pShape)
        {
            m_shapes[i] = m_shapes.back();
            m_shapes.pop_back();
        }
    }
    delete pShape;
}

// Deletes shape from container and removes any references to it.
void CAIShapeContainer::DeleteShape(CAIShape* pShape)
{
    DetachShape(pShape);
    delete pShape;
}

// Detaches specified shape from the container.
void CAIShapeContainer::DetachShape(CAIShape* pShape)
{
    for (NameToShapeMap::iterator it = m_nameToShape.begin(), end = m_nameToShape.end(); it != end; ++it)
    {
        if (it->second == pShape)
        {
            m_nameToShape.erase(it);
            break;
        }
    }

    for (unsigned i = 0; i < m_shapes.size(); ++i)
    {
        if (m_shapes[i] == pShape)
        {
            m_shapes[i] = m_shapes.back();
            m_shapes.pop_back();
        }
    }
}

// Finds shape by name.
CAIShape* CAIShapeContainer::FindShape(const string& name)
{
    NameToShapeMap::iterator it = m_nameToShape.find(name);
    if (it == m_nameToShape.end())
    {
        return 0;
    }
    return it->second;
}

// Builds quadtree structure to speed up spatial queries.
void CAIShapeContainer::BuildQuadTree(int maxElementsPerCell, float minCellSize)
{
    if (m_quadTree)
    {
        m_quadTree->Clear(true);
    }
    else
    {
        m_quadTree = new ShapeQuadTree;
    }
    for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
    {
        m_quadTree->AddElement(SQuadTreeAIShapeElement<CAIShape>(m_shapes[i]));
    }
    m_quadTree->BuildQuadTree(maxElementsPerCell, minCellSize);
}

// Builds bin sorting structures for the shapes to speed up spatial queries.
void CAIShapeContainer::BuildBins()
{
    for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
    {
        m_shapes[i]->BuildBins();
    }
}

// Clears the quadtree.
void CAIShapeContainer::ClearQuadTree()
{
    if (m_quadTree)
    {
        m_quadTree->Clear();
    }
}

// Returns all shapes that overlap the specified AABB.
void CAIShapeContainer::GetShapesOverlappingAABB(ShapeVector& elements, const AABB& aabb)
{
    if (m_quadTree)
    {
        static QuadTreeAIShapeVector el;
        el.resize(0);
        m_quadTree->GetElements(el, aabb);
        elements.resize(el.size());
        for (unsigned i = 0, ni = el.size(); i < ni; ++i)
        {
            elements[i] = el[i]->m_pShape;
        }
    }
    else
    {
        elements.clear();
        for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
        {
            if (Overlap::AABB_AABB2D(aabb, m_shapes[i]->GetAABB()))
            {
                elements.push_back(m_shapes[i]);
            }
        }
    }
}

// Returns true if specified point is inside any of the shapes in the container.
bool CAIShapeContainer::IsPointInside(const Vec3& pt, const CAIShape** outShape) const
{
    if (m_quadTree)
    {
        static QuadTreeAIShapeVector elements;
        elements.resize(0);
        m_quadTree->GetElements(elements, pt);
        for (unsigned i = 0, ni = elements.size(); i < ni; ++i)
        {
            if (elements[i]->m_pShape->IsPointInside(pt))
            {
                if (outShape)
                {
                    *outShape = elements[i]->m_pShape;
                }
                return true;
            }
        }
    }
    else
    {
        for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
        {
            if (m_shapes[i]->IsPointInside(pt))
            {
                if (outShape)
                {
                    *outShape = m_shapes[i];
                }
                return true;
            }
        }
    }
    return false;
}

// Counts and returns number of shapes that contain the specified point.
unsigned CAIShapeContainer::IsPointInsideCount(const Vec3& pt, const CAIShape** outShape) const
{
    unsigned hits = 0;
    if (m_quadTree)
    {
        static QuadTreeAIShapeVector elements;
        elements.resize(0);
        m_quadTree->GetElements(elements, pt);
        for (unsigned i = 0, ni = elements.size(); i < ni; ++i)
        {
            if (elements[i]->m_pShape->IsPointInside(pt))
            {
                if (outShape)
                {
                    *outShape = elements[i]->m_pShape;
                }
                hits++;
            }
        }
    }
    else
    {
        for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
        {
            if (m_shapes[i]->IsPointInside(pt))
            {
                if (outShape)
                {
                    *outShape = m_shapes[i];
                }
                hits++;
            }
        }
    }
    return hits;
}

// Returns true if the specified point lies on any edges within specified tolerance of any of the shapes in the container.
bool CAIShapeContainer::IsPointOnEdge(const Vec3& pt, float tol, const CAIShape** outShape) const
{
    if (m_quadTree)
    {
        AABB aabb(AABB::RESET);
        aabb.Add(pt, tol);
        static QuadTreeAIShapeVector elements;
        elements.resize(0);
        m_quadTree->GetElements(elements, aabb);
        for (unsigned i = 0, ni = elements.size(); i < ni; ++i)
        {
            if (elements[i]->m_pShape->IsPointOnEdge(pt, tol))
            {
                if (outShape)
                {
                    *outShape = elements[i]->m_pShape;
                }
                return true;
            }
        }
    }
    else
    {
        for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
        {
            if (m_shapes[i]->IsPointOnEdge(pt, tol))
            {
                if (outShape)
                {
                    *outShape = m_shapes[i];
                }
                return true;
            }
        }
    }

    return false;
}

// Returns true if the linesegment intersects with any of the shapes in hte containers.
// Finds nearest hit.
bool CAIShapeContainer::IntersectLineSeg(const Vec3& start, const Vec3& end, Vec3& outClosestPoint,
    Vec3* outNormal, bool bForceNormalOutwards, const string* nameToSkip) const
{
    bool hit = false;
    float tmin = 1.0f;

    if (m_quadTree)
    {
        AABB aabb(AABB::RESET);
        aabb.Add(start);
        aabb.Add(end);
        static QuadTreeAIShapeVector elements;
        elements.resize(0);
        m_quadTree->GetElements(elements, aabb);

        for (unsigned i = 0, ni = elements.size(); i < ni; ++i)
        {
            if (nameToSkip && elements[i]->m_pShape->GetName() == *nameToSkip)
            {
                continue;
            }
            float t = 1.0f;
            Vec3 normal;
            if (elements[i]->m_pShape->IntersectLineSeg(start, end, t, 0, outNormal ? &normal : 0, bForceNormalOutwards))
            {
                hit = true;
                if (t < tmin)
                {
                    tmin = t;
                    if (outNormal)
                    {
                        *outNormal = normal;
                    }
                }
            }
        }
    }
    else
    {
        for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
        {
            if (nameToSkip && m_shapes[i]->GetName() == *nameToSkip)
            {
                continue;
            }
            float t = 1.0f;
            Vec3 normal;
            if (m_shapes[i]->IntersectLineSeg(start, end, t, 0, outNormal ? &normal : 0, bForceNormalOutwards))
            {
                hit = true;
                if (t < tmin)
                {
                    tmin = t;
                    if (outNormal)
                    {
                        *outNormal = normal;
                    }
                }
            }
        }
    }

    outClosestPoint = start + tmin * (end - start);

    return hit;
}

// Returns memory used by the container including the shapes.
size_t CAIShapeContainer::MemStats() const
{
    size_t size = sizeof(*this);
    size += m_shapes.capacity() * sizeof(CAIShape*);
    for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
    {
        size += m_shapes[i]->MemStats();
    }
    for (NameToShapeMap::const_iterator it = m_nameToShape.begin(); it != m_nameToShape.end(); ++it)
    {
        size += (it->first).capacity();
    }
    // TODO: MemStats for the quadtree.
    //      if (m_quadTree)
    //          size += m_quadTree->MemStats();
    return size;
}


/*
// Constructor
CAISpecialAreaContainer::CAISpecialAreaContainer()
{
}

// Destructor
CAISpecialAreaContainer::~CAISpecialAreaContainer()
{
}

// Clear all shapes and supporting structures.
void CAISpecialAreaContainer::Clear()
{
}

// Insert new shape, the container will delete the shape when the container cleared.
void CAISpecialAreaContainer::InsertShape(CAISpecialArea* pShape)
{
    m_shapes.push_back(pShape);
    if (!m_nameToShape.insert(NameToShapeMap::iterator::value_type(pShape->GetName(), pShape)).second)
        AIError("CAIShapeContainer::InsertShape Duplicate AI shape (auto-forbidden area?) name %s", pShape->GetName().c_str());
}

// Deletes shape from container and removes any references to it.
void CAISpecialAreaContainer::DeleteShape(const string& name)
{
    NameToShapeMap::iterator it = m_nameToShape.find(name);
    if (it == m_nameToShape.end())
        return;
    CAIShape* pShape = it->second;
    m_nameToShape.erase(it);
    for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
    {
        if (m_shapes[i] == pShape)
        {
            m_shapes[i] = m_shapes.back();
            m_shapes.pop_back();
        }
    }
    delete pShape;
}

// Deletes shape from container and removes any references to it.
void CAISpecialAreaContainer::DeleteShape(CAISpecialArea* pShape)
{
    for (NameToShapeMap::iterator it = m_nameToShape.begin(), end = m_nameToShape.end(); it != end; ++it)
    {
        if (it->second == pShape)
        {
            m_nameToShape.erase(it);
            break;
        }
    }

    for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
    {
        if (m_shapes[i] == pShape)
        {
            m_shapes[i] = m_shapes.back();
            m_shapes.pop_back();
        }
    }

    delete pShape;
}

// Finds shape by name.
CAISpecialArea* CAISpecialAreaContainer::FindShape(const string& name)
{
    NameToShapeMap::iterator it = m_nameToShape.find(name);
    if (it == m_nameToShape.end())
        return 0;
    return it->second;
}

// Builds quad tree structure to speed up spatial queries.
void CAISpecialAreaContainer::BuildQuadTree(int maxElementsPerCell, float minCellSize)
{
    if (m_quadTree)
        m_quadTree->Clear(true);
    else
        m_quadTree = new ShapeQuadTree;
    for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
        m_quadTree->AddElement(SQuadTreeAIShapeElement<CAISpecialArea>(m_shapes[i]));
    m_quadTree->BuildQuadTree(maxElementsPerCell, minCellSize);
}

// Builds bin sorting structures for the shapes to speed up spatial queries.
void CAISpecialAreaContainer::BuildBins()
{
    for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
        m_shapes[i]->BuildBins();
}

// Returns true if specified point is inside any of the shapes in the container.
bool CAISpecialAreaContainer::IsPointInside(const Vec3& pt, bool checkHeight, const CAISpecialArea** outShape) const
{
    if (m_quadTree)
    {
        static QuadTreeAISpecialAreaVector elements;
        elements.resize(0);
        m_quadTree->GetElements(elements, pt);
        for (unsigned i = 0, ni = elements.size(); i < ni; ++i)
        {
            if (elements[i]->m_pShape->IsPointInside(pt, checkHeight))
            {
                if (outShape) *outShape = elements[i]->m_pShape;
                return true;
            }
        }
    }
    else
    {
        for (unsigned i = 0, ni = m_shapes.size(); i < ni; ++i)
        {
            if (m_shapes[i]->IsPointInside(pt, checkHeight))
            {
                if (outShape) *outShape = m_shapes[i];
                return true;
            }
        }
    }
    return false;
}

// Returns memory used by the container including the shapes.
size_t CAISpecialAreaContainer::MemStats() const
{
    return 0;
}
*/