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
#include <QToolButton>

#include <GraphCanvas/Widgets/AssetEditorToolbar/AssetEditorToolbar.h>
#include <StaticLib/GraphCanvas/Widgets/AssetEditorToolbar/ui_AssetEditorToolbar.h>

namespace GraphCanvas
{
    ///////////////////////
    // AssetEditorToolbar
    ///////////////////////

    AssetEditorToolbar::AssetEditorToolbar(EditorId editorId)
        : m_editorId(editorId)
        , m_ui(new Ui::AssetEditorToolbar())
    {
        m_ui->setupUi(this);

        AssetEditorNotificationBus::Handler::BusConnect(editorId);

        QObject::connect(m_ui->topAlign, &QToolButton::clicked, this, &AssetEditorToolbar::AlignSelectedTop);
        QObject::connect(m_ui->bottomAlign, &QToolButton::clicked, this, &AssetEditorToolbar::AlignSelectedBottom);

        QObject::connect(m_ui->leftAlign, &QToolButton::clicked, this, &AssetEditorToolbar::AlignSelectedLeft);
        QObject::connect(m_ui->rightAlign, &QToolButton::clicked, this, &AssetEditorToolbar::AlignSelectedRight);

        // Disabling these while they are a bit dodgey
        m_ui->organizationLine->setVisible(false);
        m_ui->organizeTopLeft->setVisible(false);
        m_ui->organizeCentered->setVisible(false);
        m_ui->organizeBottomRight->setVisible(false);

        // Disabling the customization line as a temporary measure
        m_ui->customizationPanel->setVisible(false);

        QObject::connect(m_ui->organizeTopLeft, &QToolButton::clicked, this, &AssetEditorToolbar::OrganizeTopLeft);
        QObject::connect(m_ui->organizeCentered, &QToolButton::clicked, this, &AssetEditorToolbar::OrganizeCentered);
        QObject::connect(m_ui->organizeBottomRight, &QToolButton::clicked, this, &AssetEditorToolbar::OrganizeBottomRight);
    }

    void AssetEditorToolbar::AddCustomAction(QToolButton* action)
    {
        m_ui->customizationPanel->setVisible(true);
        m_ui->customizationPanel->layout()->addWidget(action);
    }

    void AssetEditorToolbar::OnActiveGraphChanged(const GraphId& graphId)
    {
        m_activeGraphId = graphId;

        SceneNotificationBus::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusConnect(m_activeGraphId);

        UpdateButtonStates();
    }

    void AssetEditorToolbar::OnSelectionChanged()
    {
        UpdateButtonStates();
    }

    void AssetEditorToolbar::AlignSelectedTop(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::None;
        config.m_verAlign = GraphUtils::VerticalAlignment::Top;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        AlignSelected(config);
    }

    void AssetEditorToolbar::AlignSelectedBottom(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::None;
        config.m_verAlign = GraphUtils::VerticalAlignment::Bottom;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        AlignSelected(config);
    }

    void AssetEditorToolbar::AlignSelectedLeft(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::Left;
        config.m_verAlign = GraphUtils::VerticalAlignment::None;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        AlignSelected(config);
    }

    void AssetEditorToolbar::AlignSelectedRight(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::Right;
        config.m_verAlign = GraphUtils::VerticalAlignment::None;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        AlignSelected(config);
    }

    void AssetEditorToolbar::OrganizeTopLeft(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::Left;
        config.m_verAlign = GraphUtils::VerticalAlignment::Top;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        OrganizeSelected(config);
    }

    void AssetEditorToolbar::OrganizeCentered(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::Center;
        config.m_verAlign = GraphUtils::VerticalAlignment::Middle;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        OrganizeSelected(config);    
    }

    void AssetEditorToolbar::OrganizeBottomRight(bool)
    {
        AlignConfig config;
        config.m_horAlign = GraphUtils::HorizontalAlignment::Right;
        config.m_verAlign = GraphUtils::VerticalAlignment::Bottom;

        AssetEditorSettingsRequestBus::EventResult(config.m_alignTime, m_editorId, &AssetEditorSettingsRequests::GetAlignmentTime);

        OrganizeSelected(config);
    }

    void AssetEditorToolbar::AlignSelected(const AlignConfig& alignConfig)
    {
        AZStd::vector< NodeId > selectedNodes;
        SceneRequestBus::EventResult(selectedNodes, m_activeGraphId, &SceneRequests::GetSelectedNodes);

        GraphUtils::AlignNodes(selectedNodes, alignConfig);
    }

    void AssetEditorToolbar::OrganizeSelected(const AlignConfig& alignConfig)
    {
        AZStd::vector< NodeId > selectedNodes;
        SceneRequestBus::EventResult(selectedNodes, m_activeGraphId, &SceneRequests::GetSelectedNodes);

        GraphUtils::OrganizeNodes(selectedNodes, alignConfig);
    }

    void AssetEditorToolbar::UpdateButtonStates()
    {
        bool hasSelection = false;
        SceneRequestBus::EventResult(hasSelection, m_activeGraphId, &SceneRequests::HasSelectedItems);

        m_ui->topAlign->setEnabled(hasSelection);
        m_ui->bottomAlign->setEnabled(hasSelection);

        m_ui->leftAlign->setEnabled(hasSelection);
        m_ui->rightAlign->setEnabled(hasSelection);
    }
}

#include <StaticLib/GraphCanvas/Widgets/AssetEditorToolbar/AssetEditorToolbar.moc>