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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzToolsFramework/ToolsComponents/EditorSelectionAccentSystemComponent.h>

class CEntityObject;

namespace AzFramework
{
    struct ViewportInfo;
}

namespace AzToolsFramework
{
    /// Bus for customizing Entity selection logic from within the EditorComponents.
    /// Used to provide with custom implementation for Ray intersection tests, specifying AABB, etc.
    class EditorComponentSelectionRequests
        : public AZ::ComponentBus
    {
    public:
        /// Bounding box (AABB) display state.
        enum BoundingBoxDisplay
        {
            NoBoundingBox = 0, ///< Bounding box is not drawn.
            BoundingBox = 0x1, ///< Bounding box is visible.
            Default = NoBoundingBox, ///< Default - Do not show the bounding box of editor selection.
        };

        AZ_DEPRECATED(,
            "GetEditorSelectionBounds() has been deprecated - Please use "
            "GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& /*viewportInfo*/)")
        virtual AZ::Aabb GetEditorSelectionBounds()
        {
            AZ_Assert(false,
                "GetEditorSelectionBounds() has been deprecated - Please use "
                "GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& /*viewportInfo*/)");
            return AZ::Aabb::CreateNull();
        }

        /// @brief Returns an AABB that encompasses the object.
        /// @return AABB that encompasses the object.
        /// @note ViewportInfo may be necessary if the all or part of the object
        /// stays at a constant size regardless of camera position.
        virtual AZ::Aabb GetEditorSelectionBoundsViewport(
            const AzFramework::ViewportInfo& /*viewportInfo*/)
        {
            AZ_Assert(!SupportsEditorRayIntersect(),
                "Component claims to support ray intersection but GetEditorSelectionBoundsViewport "
                "has not been implemented in the derived class");

            return AZ::Aabb::CreateNull();
        }

        AZ_DEPRECATED(, "EditorSelectionIntersectRay(const AZ::Vector3& /*src*/, const AZ::Vector3& /*dir*/, "
            "AZ::VectorFloat& /*distance*/) has been deprecated - Please use "
            "EditorSelectionIntersectRayViewport(const AzFramework::ViewportInfo& /*viewportInfo*/, "
            "const AZ::Vector3& /*src*/, const AZ::Vector3& /*dir*/, AZ::VectorFloat& /*distance*/)")
        virtual bool EditorSelectionIntersectRay(
            const AZ::Vector3& /*src*/, const AZ::Vector3& /*dir*/, AZ::VectorFloat& /*distance*/)
        {
            AZ_Assert(false,
                "EditorSelectionIntersectRay(const AZ::Vector3& /*src*/, const AZ::Vector3& /*dir*/, "
                "AZ::VectorFloat& /*distance*/) has been deprecated - Please use "
                "EditorSelectionIntersectRayViewport(const AzFramework::ViewportInfo& /*viewportInfo*/, "
                "const AZ::Vector3& /*src*/, const AZ::Vector3& /*dir*/, AZ::VectorFloat& /*distance*/)");
            return false;
        }

        /// @brief Returns true if editor selection ray intersects with the handler.
        /// @return True if the editor selection ray intersects the handler.
        /// @note ViewportInfo may be necessary if the all or part of the object
        /// stays at a constant size regardless of camera position.
        virtual bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& /*viewportInfo*/,
            const AZ::Vector3& /*src*/, const AZ::Vector3& /*dir*/, AZ::VectorFloat& /*distance*/)
        {
            AZ_Assert(!SupportsEditorRayIntersect(),
                "Component claims to support ray intersection but EditorSelectionIntersectRayViewport "
                "has not been implemented in the derived class");

            return false;
        }

        /// @brief Returns true if the component overrides EditorSelectionIntersectRay method,
        /// otherwise selection will be based only on AABB test.
        /// @return True if EditorSelectionIntersectRay method is implemented.
        virtual bool SupportsEditorRayIntersect() { return false; }

        /// @brief Whether or not bounding box of the entity should be drawn.
        /// @return Mask of the options of the bounding box display type.
        virtual AZ::u32 GetBoundingBoxDisplayType() { return BoundingBoxDisplay::Default; }

    protected:
        ~EditorComponentSelectionRequests() = default;
    };

    /// Type to inherit to implement EditorComponentSelectionRequests.
    using EditorComponentSelectionRequestsBus = AZ::EBus<EditorComponentSelectionRequests>;

    /// Interface to access AABB in object's local space (the objects OBB in world space).
    class EditorLocalBoundsRequests
        : public AZ::ComponentBus
    {
    public:
        /// @brief Returns a tight fit AABB aligned to the object (in its local space).
        /// @note You can think of this as the objects OBB in world space.
        virtual AZ::Aabb GetEditorLocalBounds() = 0;

    protected:
        ~EditorLocalBoundsRequests() = default;
    };

    /// Inherit from EditorLocalBoundsRequestBus::Handler to implement the EditorLocalBoundsRequests interface.
    using EditorLocalBoundsRequestBus = AZ::EBus<EditorLocalBoundsRequests>;

    /// Bus that provides notifications about selection events of the parent Entity.
    class EditorComponentSelectionNotifications
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::EntityId BusIdType;

        /// @brief Notifies listeners about in-editor selection events (mouse hover, selected, etc.)
        virtual void OnAccentTypeChanged(EntityAccentType /*accent*/) {}

    protected:
        ~EditorComponentSelectionNotifications() = default;
    };

    /// Type to inherit to implement EditorComponentSelectionNotifications.
    using EditorComponentSelectionNotificationsBus = AZ::EBus<EditorComponentSelectionNotifications>;

    /// Functor used with GetEditorSelectionBounds for EBusReduceResult Aggregator type.
    struct AabbAggregator
    {
        AZ::Aabb operator()(AZ::Aabb lhs, const AZ::Aabb& rhs) const
        {
            lhs.AddAabb(rhs);
            return lhs;
        }
    };
} // namespace AzToolsFramework