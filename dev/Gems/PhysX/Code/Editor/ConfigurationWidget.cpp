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
#include <PhysX_precompiled.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <QBoxLayout>
#include <Editor/ConfigurationWidget.h>
#include <Editor/SettingsWidget.h>
#include <Editor/CollisionLayersWidget.h>
#include <Editor/CollisionGroupsWidget.h>
#include <Editor/PvdWidget.h>

namespace PhysX
{
    namespace Editor
    {
        ConfigurationWidget::ConfigurationWidget(QWidget* parent)
            : QWidget(parent)
        {
            QVBoxLayout* verticalLayout = new QVBoxLayout(this);
            verticalLayout->setContentsMargins(0, 0, 0, 0);
            verticalLayout->setSpacing(0);

            m_tabs = new QTabWidget(this);

            m_settings = new SettingsWidget();
            m_collisionLayers = new CollisionLayersWidget();
            m_collisionGroups = new CollisionGroupsWidget();
            m_pvd = new PvdWidget();

            m_tabs->addTab(m_settings, "Global Configuration");
            m_tabs->addTab(m_collisionLayers, "Layers");
            m_tabs->addTab(m_collisionGroups, "Collision Groups");
            m_tabs->addTab(m_pvd, "Debugger");

            verticalLayout->addWidget(m_tabs);

            connect(m_settings, &SettingsWidget::onValueChanged,
                this, [this](const PhysX::Settings& settings, const Physics::WorldConfiguration& worldConfiguration,
                             const PhysX::EditorConfiguration& editorConfiguration)
            {
                m_configuration.m_worldConfiguration = worldConfiguration;
                m_configuration.m_editorConfiguration = editorConfiguration;
                emit onConfigurationChanged(m_configuration);
            });

            connect(m_collisionLayers, &CollisionLayersWidget::onValueChanged,
                this, [this](const Physics::CollisionLayers& layers)
            {
                m_configuration.m_collisionLayers = layers;
                m_collisionGroups->SetValue(m_configuration.m_collisionGroups, m_configuration.m_collisionLayers);
                emit onConfigurationChanged(m_configuration);
            });

            connect(m_collisionGroups, &CollisionGroupsWidget::onValueChanged,
                this, [this](const Physics::CollisionGroups& groups)
            {
                m_configuration.m_collisionGroups = groups;
                emit onConfigurationChanged(m_configuration);
            });

            connect(m_pvd, &PvdWidget::onValueChanged,
                this, [this](const PhysX::Settings& settings)
            {
                m_configuration.m_settings = settings;
                emit onConfigurationChanged(m_configuration);
            });
        }

        void ConfigurationWidget::SetConfiguration(const PhysX::Configuration& configuration)
        {
            m_configuration = configuration;
            m_settings->SetValue(m_configuration.m_settings, m_configuration.m_worldConfiguration,
                m_configuration.m_editorConfiguration);
            m_collisionLayers->SetValue(m_configuration.m_collisionLayers);
            m_collisionGroups->SetValue(m_configuration.m_collisionGroups, m_configuration.m_collisionLayers);
            m_pvd->SetValue(m_configuration.m_settings);
        }

        const PhysX::Configuration& ConfigurationWidget::GetConfiguration() const
        {
            return m_configuration;
        }
    } // namespace PhysXConfigurationWidget
} // namespace AzToolsFramework

#include <Editor/ConfigurationWidget.moc>
