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

#include <AzCore/std/containers/vector.h>

namespace AZ
{
    /**
     * Interface provided by a container of vertices of fixed length (example: array or fixed_vector).
     */
    template<typename Vertex>
    class FixedVertices
    {
    public:
        virtual ~FixedVertices() = default;

        /**
         * Get a vertex at a particular index.
         * @param index Index of vertex to access.
         * @param vertex Out parameter of vertex at index.
         * @return Was the vertex at the index provided able to be accessed.
         */
        virtual bool GetVertex(size_t index, Vertex& vertex) const = 0;
        /**
         * Update a vertex at a particular index.
         * @param index Index of vertex to update.
         * @param vertex New vertex position.
         * @return Was the vertex at the index provided able to be updated.
         */
        virtual bool UpdateVertex(size_t index, const Vertex& vertex) = 0;
        /**
         * How many vertices are there.
         */
        virtual size_t Size() const = 0;
    };

    /**
     * Interface provided by a container of vertices of variable length (example: vector or VertexContainer).
     */
    template<typename Vertex>
    class VariableVertices
        : public FixedVertices<Vertex>
    {
    public:
        /**
         * Add vertex at end of container.
         * @param vertex New vertex position.
         */
        virtual void AddVertex(const Vertex& vertex) = 0;
        /**
         * Insert vertex at a particular index.
         * @param index Index of vertex to insert before.
         * @param vertex New vertex position.
         * @return Was the vertex able to be inserted at the provided index.
         */
        virtual bool InsertVertex(size_t index, const Vertex& vertex) = 0;
        /**
         * Insert vertex at a particular index.
         * @param index Index of vertex to remove.
         * @return Was the vertex able to be removed.
         */
        virtual bool RemoveVertex(size_t index) = 0;
        /**
         * Is the container empty.
         */
        virtual bool Empty() const = 0;
    };

    /**
     * All services provided by the Vertex Container.
     * VertexContainerInterface supports all operations of a variable
     * and fixed container as well as the ability to set and clear vertices.
     */
    template<typename Vertex>
    class VertexContainerInterface
        : public VariableVertices<Vertex>
    {
    public:
        /**
         * Set all vertices.
         * @param vertices New vertices to set.
         */
        virtual void SetVertices(const AZStd::vector<Vertex>& vertices) = 0;
        /**
         * Remove all vertices from container.
         */
        virtual void ClearVertices() = 0;
    };

    /**
     * Interface for vertex container notifications.
     */
    template<typename Vertex>
    class VertexContainerNotificationInterface
    {
    public:
        virtual ~VertexContainerNotificationInterface() = default;

        /**
         * Called when a new vertex is added.
         */
        virtual void OnVertexAdded(size_t index) = 0;
        /**
         * Called when a vertex is removed.
         */
        virtual void OnVertexRemoved(size_t index) = 0;
        /**
         * Called when a vertex is updated.
         */
        virtual void OnVertexUpdated(size_t index) = 0;
        /**
         * Called when a new set of vertices is set.
         */
        virtual void OnVerticesSet(const AZStd::vector<Vertex>& vertices) = 0;
        /**
         * Called when all vertices are cleared.
         */
        virtual void OnVerticesCleared() = 0;
    };
}