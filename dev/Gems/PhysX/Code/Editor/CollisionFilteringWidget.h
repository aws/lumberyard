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

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <AzQtComponents/Components/Widgets/SegmentControl.h>
#include <AzFramework/Physics/Collision.h>
#include <QWidget>

namespace PhysX
{
    class Configuration;

    namespace Editor
    {
        class CollisionLayersWidget;
        class CollisionGroupsWidget;
        class DocumentationLinkWidget;

        /// Container widget for wrapping the collision filtering ux
        /// Wraps the CollisionLayers and CollisionGroups widgets
        /// and presents them in a segment control.
        class CollisionFilteringWidget
            : public QWidget
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(CollisionFilteringWidget, AZ::SystemAllocator, 0);

            explicit CollisionFilteringWidget(QWidget* parent = nullptr);

            void SetConfiguration(const Physics::CollisionLayers& layers, const Physics::CollisionGroups& groups);

            void ShowLayersTab();

            void ShowGroupsTab();

        signals:
            void onConfigurationChanged(const Physics::CollisionLayers& layers, const Physics::CollisionGroups& groups);

        private:
            void ConnectSignals();
            AzQtComponents::SegmentControl* m_tabs;
            CollisionLayersWidget* m_collisionLayersWidget;
            CollisionGroupsWidget* m_collisionGroupsWidget;
            DocumentationLinkWidget* m_documentationLinkWidget;
            Physics::CollisionLayers m_layersConfig;
            Physics::CollisionGroups m_groupsConfig;
        };
    }
}