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

namespace AzToolsFramework
{
    /**
     * Bus for customizing Entity selection logic from within the EditorComponents.
     * Used to provide with custom implementation for Ray intersection tests, specifying AABB, etc.
     */
    class EditorComponentSelectionRequests
        : public AZ::EBusTraits
    {
    public:
        enum BoundingBoxDisplay
        {
            NoBoundingBox = 0,
            BoundingBox = 0x1,
            Default = BoundingBox,
        };

        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::EntityId BusIdType;

        virtual ~EditorComponentSelectionRequests() {}

        /**
         * @brief Returns an AABB that encompasses the object.
         * @return AABB that encompasses the object.
         */
        virtual AZ::Aabb GetEditorSelectionBounds() { return AZ::Aabb(); }

        /**
         * @brief Returns true if editor selection ray intersects with the handler.
         * @return True if the editor selection ray intersects the handler.
         */
        virtual bool EditorSelectionIntersectRay(const AZ::Vector3& /*src*/, const AZ::Vector3& /*dir*/, AZ::VectorFloat& /*distance*/) { return false; }

        /**
         * @brief Returns true if the component overrides EditorSelectionIntersectRay method; Othewise selection will be based only on AABB test.
         * @return True if EditorSelectionIntersectRay method is implemented.
         */
        virtual bool SupportsEditorRayIntersect() { return false; }

        /**
         * @brief Whether or not bounding box of the entity should be drawn.
         * @return Mask of the options of the bounding box display type.
         */
        virtual AZ::u32 GetBoundingBoxDisplayType() { return BoundingBoxDisplay::Default; }
    };
    using EditorComponentSelectionRequestsBus = AZ::EBus<EditorComponentSelectionRequests>;

    /**
     * Bus that provides notifications about selection events of the parent Entity.
     */
    class EditorComponentSelectionNotifications
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::EntityId BusIdType;

        virtual ~EditorComponentSelectionNotifications() {}

        /**
         * @brief Notifies listeners about in-editor selection events (mouse hover, selected, etc.)
         */
        virtual void OnAccentTypeChanged(AzToolsFramework::EntityAccentType /*accent*/) {}
    };
    using EditorComponentSelectionNotificationsBus = AZ::EBus<EditorComponentSelectionNotifications>;
} // namespace AzToolsFramework

