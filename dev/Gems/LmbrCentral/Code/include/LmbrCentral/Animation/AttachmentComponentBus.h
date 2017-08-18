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

namespace AZ
{
    class Transform;
}

namespace LmbrCentral
{
    /*!
     * Messages serviced by the AttachmentComponent.
     * The AttachmentComponent lets an entity "stick" to a
     * particular bone on a target entity.
     */
    class AttachmentComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~AttachmentComponentRequests() = default;

        //! Change attachment target.
        //! The entity will detach from any previous target.
        //!
        //! \param targetId         Attach to this entity.
        //! \param targetBoneName   Attach to this bone on target entity.
        //!                         If targetBone is not found then attach to
        //!                         target entity's transform origin.
        //! \param offset           Attachment's offset from target.
        virtual void Attach(AZ::EntityId targetId, const char* targetBoneName, const AZ::Transform& offset) = 0;

        //! The entity will detach from its target.
        virtual void Detach() = 0;

        //! Update entity's offset from target.
        virtual void SetAttachmentOffset(const AZ::Transform& offset) = 0;
    };
    using AttachmentComponentRequestBus = AZ::EBus<AttachmentComponentRequests>;

    /*!
     * Events emitted by the AttachmentComponent.
     * The AttachmentComponent lets an entity "stick" to a
     * particular bone on a target entity.
     */
    class AttachmentComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual ~AttachmentComponentNotifications() = default;

        //! The entity has attached to the target.
        //! \param targetId The target being attached to.
        virtual void OnAttached(AZ::EntityId /*targetId*/) {};

        //! The entity is detaching from the target.
        //! \param targetId The target being detached from.
        virtual void OnDetached(AZ::EntityId /*targetId*/) {};
    };
    using AttachmentComponentNotificationBus = AZ::EBus<AttachmentComponentNotifications>;
} // namespace LmbrCentral
