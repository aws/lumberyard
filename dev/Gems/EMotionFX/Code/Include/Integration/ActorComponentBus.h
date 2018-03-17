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
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>

namespace EMotionFX
{
    class ActorInstance;

    namespace Integration
    {
        /**
        * EMotion FX attachment type.
        */
        enum class AttachmentType : AZ::u32
        {
            None = 0,           ///< Do not attach to another actor.
            ActorAttachment,    ///< Attach to another actor as a separately animating attachment.
            SkinAttachment,     ///< Attach to another actor as a skinned attachment (using the same skeleton as the attachment target).
        };

        /**
        * EMotion FX Actor Component Request Bus
        * Used for making requests to EMotion FX Actor Components.
        */
        class ActorComponentRequests
            : public AZ::ComponentBus
        {
        public:

            /// Retrieve component's actor instance.
            /// \return pointer to actor instance.
            virtual EMotionFX::ActorInstance* GetActorInstance() { return nullptr; }

            /// Attach to the specified entity.
            /// \param targetEntityId - Id of the entity to attach to.
            /// \param attachmentType - Desired type of attachment.
            virtual void AttachToEntity(AZ::EntityId /*targetEntityId*/, AttachmentType /*attachmentType*/) {}

            /// Detach from parent entity, if attached.
            virtual void DetachFromEntity() {}

            /// Enables debug-drawing of the actor's root.
            virtual void DebugDrawRoot(bool /*enable*/) {}
        };

        using ActorComponentRequestBus = AZ::EBus<ActorComponentRequests>;

        /**
        * EMotion FX Actor Component Notification Bus
        * Used for monitoring events from actor components.
        */
        class ActorComponentNotifications
            : public AZ::ComponentBus
        {
        public:

            //////////////////////////////////////////////////////////////////////////
            /**
            * Custom connection policy notifies connecting listeners immediately if actor instance is already created.
            */
            template<class Bus>
            struct AssetConnectionPolicy
                : public AZ::EBusConnectionPolicy<Bus>
            {
                static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, const typename Bus::BusIdType& id = 0)
                {
                    AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, id);

                    EMotionFX::ActorInstance* instance = nullptr;
                    ActorComponentRequestBus::EventResult(instance, id, &ActorComponentRequestBus::Events::GetActorInstance);
                    if (instance)
                    {
                        handler->OnActorInstanceCreated(instance);
                    }
                }
            };
            template<typename Bus>
            using ConnectionPolicy = AssetConnectionPolicy<Bus>;
            //////////////////////////////////////////////////////////////////////////

            /// Notifies listeners when the component has created an actor instance.
            /// \param actorInstance - pointer to actor instance
            virtual void OnActorInstanceCreated(EMotionFX::ActorInstance* /*actorInstance*/) {};

            /// Notifies listeners when the component is destroying an actor instance.
            /// \param actorInstance - pointer to actor instance
            virtual void OnActorInstanceDestroyed(EMotionFX::ActorInstance* /*actorInstance*/) {};
        };

        using ActorComponentNotificationBus = AZ::EBus<ActorComponentNotifications>;

        /**
        * EMotion FX Editor Actor Component Request Bus
        * Used for making requests to EMotion FX Actor Components.
        */
        class EditorActorComponentRequests
            : public AZ::ComponentBus
        {
        public:
            virtual const AZ::Data::AssetId& GetActorAssetId() = 0;
            virtual AZ::EntityId GetAttachedToEntityId() const = 0;
        };

        using EditorActorComponentRequestBus = AZ::EBus<EditorActorComponentRequests>;

    } //namespace Integration

} // namespace EMotionFX
