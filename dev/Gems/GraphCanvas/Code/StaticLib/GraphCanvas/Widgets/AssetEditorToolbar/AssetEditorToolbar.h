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

#include <QWidget>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>

namespace Ui
{
    class AssetEditorToolbar;
}

namespace GraphCanvas
{
    class AssetEditorToolbar
        : public QWidget
        , public AssetEditorNotificationBus::Handler
        , public SceneNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AssetEditorToolbar, AZ::SystemAllocator, 0);

        AssetEditorToolbar(EditorId editorId);
        ~AssetEditorToolbar() = default;

        void AddCustomAction(QToolButton* toolButton);
        
        // AssetEditorNotificationBus
        void OnActiveGraphChanged(const GraphId& graphId) override;
        ////
        
        // SceneNotificationBus
        void OnSelectionChanged() override;
        ////

    public Q_SLOTS:
    
        // Alignment Tooling
        void AlignSelectedTop(bool checked);
        void AlignSelectedBottom(bool checked);
        
        void AlignSelectedLeft(bool checked);
        void AlignSelectedRight(bool checked);

        // Organization Tooling
        void OrganizeTopLeft(bool checked);
        void OrganizeCentered(bool checked);
        void OrganizeBottomRight(bool checked);
        
    private:

        void AlignSelected(const AlignConfig& alignConfig);
        void OrganizeSelected(const AlignConfig& alignConfig);
        
        void UpdateButtonStates();
        
        EditorId m_editorId;
        GraphId m_activeGraphId;
    
        AZStd::unique_ptr<Ui::AssetEditorToolbar> m_ui;
    };
}