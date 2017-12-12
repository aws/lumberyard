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

#pragma once

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    template<typename Vertex>
    /*static*/ void VertexContainer<Vertex>::Reflect(SerializeContext& context)
    {
        context.Class<VertexContainer<Vertex> >()
            ->Field("Vertices", &VertexContainer<Vertex>::m_vertices);

        if (EditContext* editContext = context.GetEditContext())
        {
            editContext->Class<VertexContainer<Vertex> >("Vertices", "Vertex data")
                ->ClassElement(Edit::ClassElements::EditorData, "")
                ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(Edit::Attributes::AutoExpand, true)
                ->Attribute(Edit::Attributes::ContainerCanBeModified, false)
                ->DataElement(0, &VertexContainer<Vertex>::m_vertices, "Vertices", "Vertex data for shapes")
                ->Attribute(Edit::Attributes::AddNotify, &VertexContainer<Vertex>::AddNotify)
                ->Attribute(Edit::Attributes::RemoveNotify, &VertexContainer<Vertex>::RemoveNotify)
                ->Attribute(Edit::Attributes::ChangeNotify, &VertexContainer<Vertex>::UpdateNotify)
                ->Attribute(Edit::Attributes::AutoExpand, true);
        }
    }

    template<typename Vertex>
    VertexContainer<Vertex>::VertexContainer(
        const IndexFunction& addCallback, const IndexFunction& removeCallback,
        const VoidFunction& updateCallback, const VoidFunction& setCallback,
        const VoidFunction& clearCallback)
            : m_addCallback(addCallback)
            , m_removeCallback(removeCallback)
            , m_updateCallback(updateCallback)
            , m_setCallback(setCallback)
            , m_clearCallback(clearCallback)
    {
    }

    template<typename Vertex>
    AZ_FORCE_INLINE void VertexContainer<Vertex>::AddVertex(const Vertex& vertex)
    {
        m_vertices.push_back(vertex);
        
        if (m_addCallback)
        {
            m_addCallback(m_vertices.size() - 1);
        }
    }

    template<typename Vertex>
    AZ_FORCE_INLINE bool VertexContainer<Vertex>::UpdateVertex(size_t index, const Vertex& vertex)
    {
        AZ_Assert(index < Size(), "Invalid vertex index in %s", __FUNCTION__);
        if (index < Size())
        {
            m_vertices[index] = vertex;
            
            if (m_updateCallback)
            {
                m_updateCallback();
            }

            return true;
        }

        return false;
    }

    template<typename Vertex>
    AZ_FORCE_INLINE bool VertexContainer<Vertex>::InsertVertex(size_t index, const Vertex& vertex)
    {
        AZ_Assert(index < Size(), "Invalid vertex index in %s", __FUNCTION__);
        if (index < Size())
        {
            m_vertices.insert(m_vertices.data() + index, vertex);
            
            if (m_addCallback)
            {
                m_addCallback(index);
            }

            return true;
        }

        return false;
    }

    template<typename Vertex>
    AZ_FORCE_INLINE bool VertexContainer<Vertex>::RemoveVertex(size_t index)
    {
        AZ_Assert(index < Size(), "Invalid vertex index in %s", __FUNCTION__);
        if (index < Size())
        {
            m_vertices.erase(m_vertices.data() + index);

            if (m_removeCallback)
            {
                m_removeCallback(index);
            }

            return true;
        }

        return false;
    }

    template<typename Vertex>
    AZ_FORCE_INLINE bool VertexContainer<Vertex>::GetVertex(size_t index, Vertex& vertex) const
    {
        AZ_Assert(index < Size(), "Invalid vertex index in %s", __FUNCTION__);
        if (index < Size())
        {
            vertex = m_vertices[index];
            return true;
        }

        return false;
    }

    template<typename Vertex>
    AZ_FORCE_INLINE bool VertexContainer<Vertex>::GetLastVertex(Vertex& vertex) const
    {
        if (Size() > 0)
        {
            vertex = m_vertices.back();
            return true;
        }

        return false;
    }

    template<typename Vertex>
    template<typename Vertices>
    AZ_FORCE_INLINE void VertexContainer<Vertex>::SetVertices(Vertices&& vertices)
    {
        m_vertices = AZStd::forward<Vertices>(vertices);

        if (m_setCallback)
        {
            m_setCallback();
        }
    }

    template<typename Vertex>
    AZ_FORCE_INLINE void VertexContainer<Vertex>::Clear()
    {
        m_vertices.clear();

        if (m_clearCallback)
        {
            m_clearCallback();
        }
    }

    template<typename Vertex>
    AZ_FORCE_INLINE size_t VertexContainer<Vertex>::Size() const 
    { 
        return m_vertices.size(); 
    }

    template<typename Vertex>
    AZ_FORCE_INLINE const AZStd::vector<Vertex>& VertexContainer<Vertex>::GetVertices() const
    { 
        return m_vertices; 
    }

    template<typename Vertex>
    void VertexContainer<Vertex>::SetCallbacks(const IndexFunction& addCallback, const IndexFunction& removeCallback,
        const VoidFunction& updateCallback, const VoidFunction& setCallback,
        const VoidFunction& clearCallback)
    {
        m_addCallback = addCallback;
        m_removeCallback = removeCallback;
        m_updateCallback = updateCallback;
        m_setCallback = setCallback;
        m_clearCallback = clearCallback;
    }

    template<typename Vertex>
    void VertexContainer<Vertex>::AddNotify()
    {
        const size_t vertexCount = Size();
        if (vertexCount > 0)
        {
            size_t lastVertex = vertexCount - 1;
            if (vertexCount > 1)
            {
                m_vertices[lastVertex] = m_vertices[vertexCount - 2];
            }
            else
            {
                m_vertices[lastVertex] = Vertex::CreateZero();
            }

            if (m_addCallback)
            {
                m_addCallback(lastVertex);
            }
        }
    }

    template<typename Vertex>
    void VertexContainer<Vertex>::RemoveNotify(size_t index) const
    {
        if (m_removeCallback)
        {
            m_removeCallback(index);
        }
    }

    template<typename Vertex>
    void VertexContainer<Vertex>::UpdateNotify() const
    {
        if (m_updateCallback)
        {
            m_updateCallback();
        }
    }
}