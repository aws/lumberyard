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

#include <AzCore/Component/ComponentBus.h>

namespace AzToolsFramework
{
    /*!
     * Controls whether an entity is locked in the editor
     * Locked entities can be seen but cannot be selected
     */
    class EditorLockComponentRequests
        : public AZ::ComponentBus
    {
    public:
        /// Set whether this entity is locked
        virtual void SetLocked(bool locked) = 0;

        /// Gets whether this entity is locked
        virtual bool GetLocked() = 0;
    };

    /// \ref EditorLockRequests
    using EditorLockComponentRequestBus = AZ::EBus<EditorLockComponentRequests>;

    /**
     * Notifications about whether an Entity is locked in the Editor.
     * See \ref EditorLockRequests.
     */
    class EditorLockComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        /// The entity's current lock state has changed.
        virtual void OnEntityLockChanged(bool /*locked*/) {}
    };

    /// \ref EditorVisibilityNotifications
    using EditorLockComponentNotificationBus = AZ::EBus<EditorLockComponentNotifications>;
}