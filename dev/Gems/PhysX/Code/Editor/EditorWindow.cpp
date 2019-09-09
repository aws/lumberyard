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

#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <LyViewPaneNames.h>

#include <Editor/ui_EditorWindow.h>
#include <Editor/EditorWindow.h>
#include <Editor/ConfigurationWidget.h>
#include <PhysX/ConfigurationBus.h>

namespace PhysX
{
    namespace Editor
    {

        EditorWindow::EditorWindow(QWidget* parent)
            : QWidget(parent)
            , m_ui(new Ui::EditorWindowClass())
        {
            m_ui->setupUi(this);
            PhysX::Configuration configuration;
            PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);
            m_ui->m_PhysXConfigurationWidget->SetConfiguration(configuration);

            connect(m_ui->m_PhysXConfigurationWidget, &PhysX::Editor::ConfigurationWidget::onConfigurationChanged, 
                this, &EditorWindow::SaveConfiguration);
        }

        void EditorWindow::RegisterViewClass()
        {
            AzToolsFramework::ViewPaneOptions options;
            options.preferedDockingArea = Qt::LeftDockWidgetArea;
            options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
            options.saveKeyName = "PhysXConfiguration";
            options.isPreview = true;
            AzToolsFramework::RegisterViewPane<EditorWindow>(LyViewPane::PhysXConfigurationEditor, LyViewPane::CategoryTools, options);
        }

        void EditorWindow::SaveConfiguration(const PhysX::Configuration& configuration)
        {
            PhysX::ConfigurationRequestBus::Broadcast(&PhysX::ConfigurationRequests::SetConfiguration, configuration);
        }
    }
}
#include <Editor/EditorWindow.moc>
