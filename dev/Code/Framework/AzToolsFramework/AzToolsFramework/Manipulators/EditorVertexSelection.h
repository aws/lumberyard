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

#include <AzCore/Math/VertexContainer.h>
#include <AzCore/Math/VertexContainerInterface.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/HoverSelection.h>
#include <AzToolsFramework/Manipulators/SelectionManipulator.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>

namespace AzToolsFramework
{
    /**
     * Concrete implementation of AZ::VariableVertices backed by an AZ::VertexContainer.
     */
    template<typename Vertex>
    class VariableVerticesVertexContainer
        : public AZ::VariableVertices<Vertex>
    {
    public:
        explicit VariableVerticesVertexContainer(AZ::VertexContainer<Vertex>& vertexContainer)
            : m_vertexContainer(vertexContainer) {}

        bool GetVertex(size_t index, Vertex& vertex) const override { return m_vertexContainer.GetVertex(index, vertex); }
        bool UpdateVertex(size_t index, const Vertex& vertex) override { return m_vertexContainer.UpdateVertex(index, vertex); };
        void AddVertex(const Vertex& vertex) override { m_vertexContainer.AddVertex(vertex); }
        bool InsertVertex(size_t index, const Vertex& vertex) override { return m_vertexContainer.InsertVertex(index, vertex); }
        bool RemoveVertex(size_t index) override { return m_vertexContainer.RemoveVertex(index); }
        size_t Size() const override { return m_vertexContainer.Size();  }
        bool Empty() const override { return m_vertexContainer.Empty(); }

    private:
        AZ::VertexContainer<Vertex>& m_vertexContainer;
    };

    /**
     * Concrete implementation of AZ::FixedVertices backed by an AZStd::fixed_vector.
     */
    template<typename Vertex, size_t Count>
    class FixedVerticesVector
        : public AZ::FixedVertices<Vertex>
    {
    public:
        explicit FixedVerticesVector(AZStd::fixed_vector<Vertex, Count>& fixedVector)
            : m_fixedVector(fixedVector) {}

        bool GetVertex(size_t index, Vertex& vertex) const override { if (index < m_fixedVector.size()) { vertex = m_fixedVector[index]; return true; } return false; }
        void UpdateVertex(size_t index, const Vertex& vertex) override { m_fixedVector[index] = vertex; }
        size_t Size() const override { return m_fixedVector.size();  }

    private:
        AZStd::fixed_vector<Vertex, Count>& m_fixedVector;
    };

    /**
     * Concrete implementation of AZ::FixedVertices backed by an AZStd::array.
     */
    template<typename Vertex, size_t Count>
    class FixedVerticesArray
        : public AZ::FixedVertices<Vertex>
    {
    public:
        explicit FixedVerticesArray(AZStd::array<Vertex, Count>& array)
            : m_array(array) {}

        bool GetVertex(size_t index, Vertex& vertex) const override
        {
            if (index < m_array.size())
            {
                vertex = m_array[index];
                return true;
            }

            return false;
        }

        bool UpdateVertex(size_t index, const Vertex& vertex) override
        {
            if (index < m_array.size())
            {
                m_array[index] = vertex;
                return true;;
            }

            return false;
        }

        size_t Size() const override { return m_array.size(); }

    private:
        AZStd::array<Vertex, Count>& m_array;
    };

    /**
     * EditorVertexSelection provides an interface for a collection of manipulators to expose
     * editing of vertices in a container/collection. EditorVertexSelection is templated on the
     * type of Vertex (Vector2/Vector3) stored in the container.
     * EditorVertexSelectionBase provides common behaviour shared across Fixed and Variable selections.
     */
    template<typename Vertex>
    class EditorVertexSelectionBase
        : private ManipulatorRequestBus::Handler
    {
    public:
        EditorVertexSelectionBase() = default;
        virtual ~EditorVertexSelectionBase() {}

        void Create(AZ::EntityId entityId, ManipulatorManagerId managerId,
            TranslationManipulators::Dimensions dimensions,
            TranslationManipulatorConfiguratorFn translationManipulatorConfigurator);
        void Destroy();

        /**
         * Update manipulators based on local changes to vertex positions.
         */
        void RefreshLocal();
        /**
         * Update manipulators based on changes to the entities transform.
         */
        void RefreshSpace(const AZ::Transform& worldFromLocal);

        void SetBoundsDirty();

        // ManipulatorRequestBus::Handler
        void ClearSelected() override;
        void SetSelectedPosition(const AZ::Vector3& localPosition) override;

        /**
         * Create a translation manipulator for a given vertex.
         */
        void CreateTranslationManipulator(
            AZ::EntityId entityId, ManipulatorManagerId managerId,
            TranslationManipulators::Dimensions dimensions, const Vertex& vertex, size_t index,
            TranslationManipulatorConfiguratorFn translationManipulatorConfigurator);

        AZStd::function<void()> m_onVertexPositionsUpdated = nullptr; ///< Callback for when vertex positions are changed.

        /**
         * Derived VertexContainers must implement these virtual functions to customize behaviour.
         */
        virtual AZ::FixedVertices<Vertex>* GetVertices() const = 0;
        virtual void SetupSelectionManipulator(
            const AZStd::shared_ptr<SelectionManipulator>& selectionManipulator, AZ::EntityId entityId,
            ManipulatorManagerId managerId, TranslationManipulators::Dimensions dimensions, size_t index,
            TranslationManipulatorConfiguratorFn translationManipulatorConfigurator) = 0;
        virtual void OnCreated(AZ::EntityId /*entityId*/) {}
        virtual void OnDestroyed() {}

        // must be set before Create is called
        AZStd::unique_ptr<HoverSelection> m_hoverSelection = nullptr; ///< Interface to hover selection, representing bounds that can be selected.

    protected:
        /**
         * Set selected manipulators and vertices position from offset from starting position when pressed.
         */
        void UpdateManipulatorsAndVerticesFromOffset(
            IndexedTranslationManipulator<Vertex>& translationManipulator, const AZ::Vector3& localOffset);

        /**
         * Set selected manipulators and vertices position from position.
         */
        void UpdateManipulatorsAndVerticesFromPosition(
            IndexedTranslationManipulator<Vertex>& translationManipulator, const AZ::Vector3& localPosition);

        /**
         * Default behaviour when clicking on a selection manipulator (representing a vertex).
         */
        void SelectionManipulatorSelectCallback(
            size_t index, const ViewportInteraction::MouseInteraction& interaction, AZ::EntityId entityId,
            ManipulatorManagerId managerId, TranslationManipulators::Dimensions dimensions,
            TranslationManipulatorConfiguratorFn translationManipulatorConfigurator);

        AZStd::shared_ptr<IndexedTranslationManipulator<Vertex>> m_translationManipulator = nullptr; ///< Manipulator when vertex is selected to translate it.
        AZStd::vector<AZStd::shared_ptr<SelectionManipulator>> m_selectionManipulators; ///< Manipulators for each vertex when entity is selected.

    private:
        AZ_DISABLE_COPY_MOVE(EditorVertexSelectionBase)

        /**
         * Called after manipulator and vertex positions have been updated.
         */
        void ManipulatorsAndVerticesUpdated();

        AZ::EntityId m_entityId; ///< Id of the entity this editor vertex selection was created on.
    };

    /**
     * EditorVertexSelectionFixed provides selection and editing for a fixed length number of
     * vertices. New vertices cannot be inserted/added or removed.
     */
    template<typename Vertex>
    class EditorVertexSelectionFixed
        : public EditorVertexSelectionBase<Vertex>
    {
    public:
        EditorVertexSelectionFixed() = default;

        // EditorVertexSelectionBase
        AZ::FixedVertices<Vertex>* GetVertices() const override { return m_vertices.get(); }
        void SetupSelectionManipulator(
            const AZStd::shared_ptr<SelectionManipulator>& selectionManipulator, AZ::EntityId entityId,
            ManipulatorManagerId managerId, TranslationManipulators::Dimensions dimensions, size_t index,
            TranslationManipulatorConfiguratorFn translationManipulatorConfigurator) override;

        AZStd::unique_ptr<AZ::FixedVertices<Vertex>> m_vertices; ///< Reference to backing store of vertices, set when created.

    private:
        AZ_DISABLE_COPY_MOVE(EditorVertexSelectionFixed)
    };

    /**
     * EditorVertexSelectionVariable provides selection and editing for a variable length number of
     * vertices. New vertices can be inserted/added or removed from the collection.
     */
    template<typename Vertex>
    class EditorVertexSelectionVariable
        : public EditorVertexSelectionBase<Vertex>
        , private DestructiveManipulatorRequestBus::Handler
    {
    public:
        EditorVertexSelectionVariable() = default;

        // DestructiveManipulatorRequestBus::Handler
        void DestroySelected() override;

        // EditorVertexSelectionBase
        AZ::FixedVertices<Vertex>* GetVertices() const override { return m_vertices.get(); }
        void SetupSelectionManipulator(
            const AZStd::shared_ptr<SelectionManipulator>& selectionManipulator, AZ::EntityId entityId,
            ManipulatorManagerId managerId, TranslationManipulators::Dimensions dimensions, size_t index,
            TranslationManipulatorConfiguratorFn translationManipulatorConfigurator) override;
        void OnCreated(AZ::EntityId entityId) override;
        void OnDestroyed() override;

        AZStd::unique_ptr<AZ::VariableVertices<Vertex>> m_vertices; ///< Reference to backing store of vertices, set when created.

    private:
        AZ_DISABLE_COPY_MOVE(EditorVertexSelectionVariable)
    };

    // template helper to map a local/world space position into a vertex container
    // depending on if it is storing Vector2s or Vector3s.
    template<typename Vertex>
    AZ_FORCE_INLINE Vertex AdaptVertexIn(const AZ::Vector3&) { return AZ::Vector3::CreateZero(); }

    template<>
    AZ_FORCE_INLINE AZ::Vector3 AdaptVertexIn<AZ::Vector3>(const AZ::Vector3& vector) { return vector; }

    template<>
    AZ_FORCE_INLINE AZ::Vector2 AdaptVertexIn<AZ::Vector2>(const AZ::Vector3& vector) { return Vector3ToVector2(vector); }

    // template helper to map a vertex (vector) from a vertex container to a local/world space position
    // depending on if the vertex container is storing Vector2s or Vector3s.
    template<typename Vertex>
    AZ_FORCE_INLINE AZ::Vector3 AdaptVertexOut(const Vertex&) { return AZ::Vector3::CreateZero(); }

    template<>
    AZ_FORCE_INLINE AZ::Vector3 AdaptVertexOut<AZ::Vector3>(const AZ::Vector3& vector) { return vector; }

    template<>
    AZ_FORCE_INLINE AZ::Vector3 AdaptVertexOut<AZ::Vector2>(const AZ::Vector2& vector) { return Vector2ToVector3(vector); }

    /**
     * Utility functions for rendering vertex container text indices
     */
    namespace EditorVertexSelectionUtil
    {
        static const float DefaultVertexTextSize = 1.5f;

        static const AZ::Color DefaultVertexTextColor = AZ::Color(1.f, 1.f, 1.f, 1.f);

        static const AZ::Vector3 DefaultVertexTextOffset = AZ::Vector3(0.f, 0.f, -0.1f);

        /**
         * Displays a single vertex container index as text at the given position
         */
        void DisplayVertexContainerIndex(AzFramework::EntityDebugDisplayRequests& displayContext
            , const AZ::Vector3& position
            , size_t index
            , float textSize
        );

        /**
         * Displays all vertex container indices as text at the position of each vertex when selected
         */
        template<typename Vertex>
        void DisplayVertexContainerIndices(AzFramework::EntityDebugDisplayRequests& displayContext
            , const AzToolsFramework::EditorVertexSelectionBase<Vertex>& vertexContainer
            , const AZ::Transform& transform
            , bool isSelected
            , float textSize = DefaultVertexTextSize
            , const AZ::Color& textColor = DefaultVertexTextColor
            , const AZ::Vector3& textOffset = DefaultVertexTextOffset
        )
        {
            if (!isSelected)
            {
                return;
            }

            displayContext.SetColor(textColor);
            if (auto vertices = vertexContainer.GetVertices())
            {
                for (auto i = 0; i < vertices->Size(); ++i)
                {
                    Vertex vertex;
                    if (vertices->GetVertex(i, vertex))
                    {
                        AZ::Vector3 position = AdaptVertexOut(vertex) + textOffset;
                        DisplayVertexContainerIndex(displayContext, transform * position, i, textSize);
                    }
                }
            }
        }
    }
}