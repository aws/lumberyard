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

#include <AzCore/Component/EntityBus.h>

#include "EditorComponentBase.h"
#include "EditorEntityIconComponentBus.h"
#include "EditorInspectorComponentBus.h"

namespace AzToolsFramework
{
    namespace Components
    {
        /**
        * Entity icons are the visual icon representing an entity in the editor viewport.
        * This component enables customization of the entity icon for the owning entity.
        * If the \ref m_entityIconAssetId is invalid, an icon from one of its components is chosen instead.
        */
        class EditorEntityIconComponent
            : public EditorComponentBase
            , public AZ::EntityBus::Handler
            , public EditorEntityIconComponentRequestBus::Handler
            , public EditorInspectorComponentNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(EditorEntityIconComponent, "{E15D42C2-912D-466F-9547-E7E948CE2D7D}", EditorComponentBase);
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            ~EditorEntityIconComponent() override;

            ////////////////////////////////////////////////////////////////////
            // EditorEntityIconComponentRequestBus
            void SetEntityIconAsset(const AZ::Data::AssetId& assetId) override;
            AZ::Data::AssetId GetEntityIconAssetId() override;
            AZStd::string GetEntityIconPath() override;
            bool IsEntityIconHiddenInViewport() override;
            ////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////
            // EntityBus
            void OnEntityActivated(const AZ::EntityId&) override;
            ////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////
            // EditorInspectorComponentNotificationBus
            void OnComponentOrderChanged() override;
            ////////////////////////////////////////////////////////////////////

        private:
            ////////////////////////////////////////////////////////////////////
            // AZ::Entity
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////

            /**
            * Return the path of the entity icon asset identified by \ref m_entityIconAssetId if it's valid,
            * else return the path of the icon of the first component in this entity's EditorInspector list,
            * otherwise return the path of the default entity icon.
            */
            AZStd::string CalculateEntityIconPath(AZ::ComponentId firstComponentId);
            AZStd::string GetEntityIconAssetPath();
            AZStd::string GetDefaultEntityIconPath(AZ::ComponentId firstComponentId);

            /**
            * Return a boolean indicating if \ref m_firstComponentIdCache has been changed.
            */
            bool UpdateFirstComponentIdCache();

            bool UpdatePreferNoViewportIconFlag();

            AZ::Data::AssetId m_entityIconAssetId = AZ::Data::AssetId();
            AZStd::string m_entityIconPathCache;

            // first component id listed in the EntityInspector, excluding any default component such as TransformComponent
            AZ::ComponentId m_firstComponentIdCache = AZ::InvalidComponentId;

            // indicating if any component of this entity has the PreferNoViewportIcon Edit Attribute
            bool m_preferNoViewportIcon = false;
        };
    }
}
