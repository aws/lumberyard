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

namespace Camera
{
    /**
     * This bus allows you to get and set the current editor viewport camera
     */
    class EditorCameraRequests : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<EditorCameraRequests>;
        virtual ~EditorCameraRequests() = default;

        /**
         * Sets the view from the entity's perspective
         * @param entityId the id of the entity whose perspective is to be used
         */
        virtual void SetViewFromEntityPerspective(const AZ::EntityId& /*entityId*/) {}

        /**
         * Sets the view from the entity's perspective
         * @param entityId the id of the entity whose perspective is to be used
         * @param lockCameraMovement disallow camera movement from user input in the editor render viewport.
         */
        virtual void SetViewAndMovementLockFromEntityPerspective(const AZ::EntityId& /*entityId*/, bool /*lockCameraMovement*/) {}

        /**
         * Gets the id of the current view entity. Invalid EntityId is returned for the default editor camera
         * @return the entityId of the entity currently being used as the view.  The Invalid entity id is returned for the default editor camera
         */
        virtual AZ::EntityId GetCurrentViewEntityId() { return AZ::EntityId(); }
    };

    using EditorCameraRequestBus = AZ::EBus<EditorCameraRequests>;

    /**
     * This is the bus to interface with the camera system component
     */
    class EditorCameraSystemRequests : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<EditorCameraSystemRequests>;
        virtual ~EditorCameraSystemRequests() = default;

        virtual void CreateCameraEntityFromViewport() {}
    };
    using EditorCameraSystemRequestBus = AZ::EBus<EditorCameraSystemRequests>;

    /**
     * Handle this bus to be notified when the current editor viewport entity id changes
     */
    class EditorCameraNotifications : public AZ::EBusTraits
    {
    public:
        virtual ~EditorCameraNotifications() = default;

        /**
         * Handle this message to know when the current viewports view entity has changed
         * @param newViewId the id of the entity the current view has switched to
         */
        virtual void OnViewportViewEntityChanged(const AZ::EntityId& /*newViewId*/) {}
    };

    using EditorCameraNotificationBus = AZ::EBus<EditorCameraNotifications>;

} // namespace Camera