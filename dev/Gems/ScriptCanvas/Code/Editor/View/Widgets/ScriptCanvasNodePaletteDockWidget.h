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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzFramework/Asset/AssetSystemBus.h>

#include <AzQtComponents/Components/StyledDockWidget.h>

#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeCategorizer.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>

#include <ScriptCanvas/Components/EditorUtils.h>
#include <ScriptCanvas/Core/Core.h>


class QToolButton;

namespace Ui
{
    class ScriptCanvasNodePaletteToolbar;
}

namespace ScriptCanvasEditor
{
    class ScriptEventsPaletteTreeItem;
    class DynamicEBusPaletteTreeItem;
    class NodePaletteModel;

    namespace Widget
    {
        class ScriptCanvasRootPaletteTreeItem
            : public GraphCanvas::NodePaletteTreeItem
            , AzFramework::AssetSystemBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(ScriptCanvasRootPaletteTreeItem, AZ::SystemAllocator, 0);
            ScriptCanvasRootPaletteTreeItem(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel);
            ~ScriptCanvasRootPaletteTreeItem();

            GraphCanvas::NodePaletteTreeItem* GetCategoryNode(const char* categoryPath, GraphCanvas::NodePaletteTreeItem* parentRoot = nullptr);
            void PruneEmptyNodes();

        private:

            void OnRowsInserted(const QModelIndex& parentIndex, int first, int last);
            void OnRowsAboutToBeRemoved(const QModelIndex& parentIndex, int first, int last);

            void TraverseTree(QModelIndex index = QModelIndex());

            void ProcessAsset(AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);

            //! Called by the AssetProcessor when an asset in the cache has been modified.
            void AssetChanged(AzFramework::AssetSystem::AssetNotificationMessage /*message*/) override;
            //! Called by the AssetProcessor when an asset in the cache has been removed.
            void AssetRemoved(AzFramework::AssetSystem::AssetNotificationMessage /*message*/) override;

            const NodePaletteModel& m_nodePaletteModel;
            AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_assetModel;

            GraphCanvas::GraphCanvasTreeCategorizer m_categorizer;

            AZStd::unordered_map< AZ::Data::AssetId, ScriptEventsPaletteTreeItem* > m_scriptEventElementTreeItems;
            AZStd::unordered_map< AZ::Data::AssetId, DynamicEBusPaletteTreeItem* > m_dynamicEbusElementTreeItems;

            AZStd::vector< QMetaObject::Connection > m_lambdaConnections;
        };

        class ScriptCanvasNodePaletteToolbar
            : public QWidget
        {
            Q_OBJECT
        public:

            enum class FilterType
            {
                AllNodes
            };


            ScriptCanvasNodePaletteToolbar(QWidget* parent);

        signals:

            void OnFilterChanged(FilterType newFilter);
            void CreateDynamicEBus();

        private:

            AZStd::unique_ptr< Ui::ScriptCanvasNodePaletteToolbar > m_ui;
        };

        class NodePaletteDockWidget
            : public GraphCanvas::NodePaletteDockWidget
            , public GraphCanvas::AssetEditorNotificationBus::Handler
            , public GraphCanvas::SceneNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(NodePaletteDockWidget, AZ::SystemAllocator, 0);

            static const char* GetMimeType() { return "scriptcanvas/node-palette-mime-event"; }

            NodePaletteDockWidget(const NodePaletteModel& model, const QString& windowLabel, QWidget* parent, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel, bool inContextMenu = false);
            ~NodePaletteDockWidget() = default;

            void OnNewCustomEvent();

            void HandleTreeItemDoubleClicked(GraphCanvas::GraphCanvasTreeItem* treeItem);

            // GraphCanvas::AssetEditorNotificationBus::Handler
            void OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId) override;
            ////

            // GraphCanvas::SceneNotificationBus
            void OnSelectionChanged() override;
            ////

        protected:

            GraphCanvas::GraphCanvasTreeItem* CreatePaletteRoot() const override;

            void OnTreeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

            void SetCycleTarget(ScriptCanvas::NodeTypeIdentifier cyclingIdentifier);
            void CycleToNextNode();
            void CycleToPreviousNode();

        private:

            void ConfigureHelper();

            AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_assetModel;
            const NodePaletteModel& m_nodePaletteModel;

            ScriptCanvasRootPaletteTreeItem* m_paletteRoot;

            QToolButton* m_newCustomEvent;

            ScriptCanvas::NodeTypeIdentifier    m_cyclingIdentifier;
            GraphCanvas::NodeFocusCyclingHelper m_cyclingHelper;

            QAction* m_nextCycleAction;
            QAction* m_previousCycleAction;

            bool     m_ignoreSelectionChanged;
        };
    }    
}
