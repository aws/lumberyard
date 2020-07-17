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

#include <PhysX/ConfigurationBus.h>
#include <Editor/ConfigurationWindowBus.h>

namespace AzQtComponents
{
    class TabWidget;
}

namespace PhysX
{
    namespace Editor
    {
        class SettingsWidget;
        class CollisionFilteringWidget;
        class PvdWidget;

        /// Widget for editing physx configuration and settings.
        ///
        class ConfigurationWidget
            : public QWidget
            , public ConfigurationWindowRequestBus::Handler
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(ConfigurationWidget, AZ::SystemAllocator, 0);

            explicit ConfigurationWidget(QWidget* parent = nullptr);
            ~ConfigurationWidget() override;

            void SetConfiguration(const PhysX::PhysXConfiguration& physxConfiguration,
                                  const Physics::CollisionConfiguration& collisionConfiguration,
                                  const Physics::WorldConfiguration& worldConfiguration,
                                  const AZ::Data::Asset<Physics::MaterialLibraryAsset>& materialLibrary);

            // ConfigurationWindowRequestBus
            void ShowCollisionLayersTab() override;
            void ShowCollisionGroupsTab() override;
            void ShowGlobalSettingsTab() override;

        signals:
            void onConfigurationChanged(const PhysX::PhysXConfiguration& physxConfiguration,
                                        const Physics::CollisionConfiguration& collisionConfiguration,
                                        const Physics::WorldConfiguration& worldConfiguration,
                                        const AZ::Data::Asset<Physics::MaterialLibraryAsset>& materialLibrary);

        private:
            Physics::CollisionConfiguration m_collisionConfiguration;
            Physics::WorldConfiguration m_worldConfiguration;
            AZ::Data::Asset<Physics::MaterialLibraryAsset> m_materialLibrary 
                = AZ::Data::AssetLoadBehavior::NoLoad;
            PhysX::PhysXConfiguration m_physxConfiguration;

            AzQtComponents::TabWidget* m_tabs;
            SettingsWidget* m_settings;
            CollisionFilteringWidget* m_collisionFiltering;
            PvdWidget* m_pvd;
        };
    } // namespace Editor
} // namespace PhysX
