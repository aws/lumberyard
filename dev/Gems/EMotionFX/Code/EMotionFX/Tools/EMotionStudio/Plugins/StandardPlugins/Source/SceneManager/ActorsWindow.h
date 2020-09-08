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

#include <AzCore/std/string/string.h>
#include "../StandardPluginsConfig.h"
#include <QAction>
#include <QTreeWidget>
#include <QWidget>


namespace EMStudio
{
    class SceneManagerPlugin;

    class ActorsWindow
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(ActorsWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        ActorsWindow(SceneManagerPlugin* plugin, QWidget* parent = nullptr);

        void ReInit();
        void UpdateInterface();

    public slots:
        void OnRemoveButtonClicked();
        void OnClearButtonClicked();
        void OnCreateInstanceButtonClicked();
        void OnVisibleChanged(QTreeWidgetItem* item, int column);
        void OnSelectionChanged();
        void OnResetTransformationOfSelectedActorInstances();
        void OnCloneSelected();
        void OnUnhideSelected()                                     { SetVisibilityFlags(true); }
        void OnHideSelected()                                       { SetVisibilityFlags(false); }

    protected:
        void contextMenuEvent(QContextMenuEvent* event) override;

    private:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void SetControlsEnabled();
        uint32 GetIDFromTreeItem(QTreeWidgetItem* item);
        void SetVisibilityFlags(bool isVisible);

    private:
        SceneManagerPlugin* m_plugin = nullptr;
        QTreeWidget* m_treeWidget = nullptr;
        QAction* m_loadActorAction = nullptr;
        QAction* m_mergeActorAction = nullptr;
        QAction* m_createInstanceAction = nullptr;
        QAction* m_saveAction = nullptr;
        AZStd::string m_tempString;
    };
} // namespace EMStudio
