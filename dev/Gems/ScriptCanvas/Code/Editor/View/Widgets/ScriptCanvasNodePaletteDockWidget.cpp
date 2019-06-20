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
#include "precompiled.h"

#include <QLineEdit>
#include <QMenu>
#include <QSignalBlocker>
#include <QScrollBar>
#include <qboxlayout.h>
#include <qpainter.h>
#include <qevent.h>
#include <QCoreApplication>
#include <QCompleter>
#include <QHeaderView>
#include <QScopedValueRollback>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetEditor/AssetEditorUtils.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <ScriptEvents/ScriptEventsAsset.h>

#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>

#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/ui_ScriptCanvasNodePaletteToolbar.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Nodes/NodeUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/ScriptEventsNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/GeneralNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/SpecializedNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>

#include <Editor/Settings.h>
#include <Editor/Components/IconComponent.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>
#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Translation/TranslationHelper.h>
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Utils/NodeUtils.h>

#include <Core/Attributes.h>
#include <Libraries/Entity/EntityRef.h>
#include <Libraries/Libraries.h>

#include <Libraries/Core/GetVariable.h>
#include <Libraries/Core/Method.h>
#include <Libraries/Core/SetVariable.h>

#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>

namespace
{
    GraphCanvas::NodePaletteTreeItem* ExternalCreatePaletteRoot(const ScriptCanvasEditor::NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
    {
        ScriptCanvasEditor::Widget::ScriptCanvasRootPaletteTreeItem* root = aznew ScriptCanvasEditor::Widget::ScriptCanvasRootPaletteTreeItem(nodePaletteModel, assetModel);

        {
            GraphCanvas::NodePaletteTreeItem* utilitiesRoot = root->GetCategoryNode("Utilities");

            utilitiesRoot->CreateChildNode<ScriptCanvasEditor::CommentNodePaletteTreeItem>("Comment", QString(ScriptCanvasEditor::IconComponent::LookupClassIcon(AZ::Uuid()).c_str()));

            root->CreateChildNode<ScriptCanvasEditor::LocalVariablesListNodePaletteTreeItem>("Variables");

            // We always want to keep this one around as a place holder
            GraphCanvas::NodePaletteTreeItem* customEventRoot = root->GetCategoryNode("Script Events");
            customEventRoot->SetAllowPruneOnEmpty(false);
        }

        const ScriptCanvasEditor::NodePaletteModel::NodePaletteRegistry& nodeRegistry = nodePaletteModel.GetNodeRegistry();

        for (const auto& registryPair : nodeRegistry)
        {
            const ScriptCanvasEditor::NodePaletteModelInformation* modelInformation = registryPair.second;

            GraphCanvas::GraphCanvasTreeItem* parentItem = root->GetCategoryNode(modelInformation->m_categoryPath.c_str());
            GraphCanvas::NodePaletteTreeItem* createdItem = nullptr;

            if (auto customModelInformation = azrtti_cast<const ScriptCanvasEditor::CustomNodeModelInformation*>(modelInformation))
            {
                createdItem = parentItem->CreateChildNode<ScriptCanvasEditor::CustomNodePaletteTreeItem>(customModelInformation->m_typeId, customModelInformation->m_displayName);
                createdItem->SetToolTip(QString(customModelInformation->m_toolTip.c_str()));
            }
            else if (auto methodNodeModelInformation = azrtti_cast<const ScriptCanvasEditor::MethodNodeModelInformation*>(modelInformation))
            {
                createdItem = parentItem->CreateChildNode<ScriptCanvasEditor::ClassMethodEventPaletteTreeItem>(methodNodeModelInformation->m_classMethod, methodNodeModelInformation->m_metehodName);
            }
            else if (auto ebusHandlerNodeModelInformation = azrtti_cast<const ScriptCanvasEditor::EBusHandlerNodeModelInformation*>(modelInformation))
            {
                createdItem = parentItem->CreateChildNode<ScriptCanvasEditor::EBusHandleEventPaletteTreeItem>(ebusHandlerNodeModelInformation->m_busName, ebusHandlerNodeModelInformation->m_eventName, ebusHandlerNodeModelInformation->m_busId, ebusHandlerNodeModelInformation->m_eventId);
            }
            else if (auto ebusSenderNodeModelInformation = azrtti_cast<const ScriptCanvasEditor::EBusSenderNodeModelInformation*>(modelInformation))
            {
                createdItem = parentItem->CreateChildNode<ScriptCanvasEditor::EBusSendEventPaletteTreeItem>(ebusSenderNodeModelInformation->m_busName, ebusSenderNodeModelInformation->m_eventName, ebusSenderNodeModelInformation->m_busId, ebusSenderNodeModelInformation->m_eventId);
            }


            if (createdItem)
            {
                modelInformation->PopulateTreeItem((*createdItem));
            }
        }

        root->PruneEmptyNodes();

        return root;
    }

} // anonymous namespace.

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        ////////////////////////////////////
        // ScriptCanvasRootPaletteTreeItem
        ////////////////////////////////////

        ScriptCanvasRootPaletteTreeItem::ScriptCanvasRootPaletteTreeItem(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
            : GraphCanvas::NodePaletteTreeItem("root", ScriptCanvasEditor::AssetEditorId)
            , m_nodePaletteModel(nodePaletteModel)
            , m_assetModel(assetModel)
            , m_categorizer(nodePaletteModel)
        {
            if (m_assetModel)
            {
                TraverseTree();

                {
                    auto connection = QObject::connect(m_assetModel, &QAbstractItemModel::rowsInserted, [this](const QModelIndex& parentIndex, int first, int last) { this->OnRowsInserted(parentIndex, first, last); });
                    m_lambdaConnections.emplace_back(connection);
                }

                {
                    auto connection = QObject::connect(m_assetModel, &QAbstractItemModel::rowsAboutToBeRemoved, [this](const QModelIndex& parentIndex, int first, int last) { this->OnRowsAboutToBeRemoved(parentIndex, first, last); });
                    m_lambdaConnections.emplace_back(connection);
                }
            }
        }

        ScriptCanvasRootPaletteTreeItem::~ScriptCanvasRootPaletteTreeItem()
        {
            for (auto connection : m_lambdaConnections)
            {
                QObject::disconnect(connection);
            }
        }

        // Given a category path (e.g. "My/Category") and a parent node, creates the necessary intermediate
        // nodes under the given parent and returns the leaf tree item under the given category path.
        GraphCanvas::NodePaletteTreeItem* ScriptCanvasRootPaletteTreeItem::GetCategoryNode(const char* categoryPath, GraphCanvas::NodePaletteTreeItem* parentRoot)
        {
            if (parentRoot)
            {
                return static_cast<GraphCanvas::NodePaletteTreeItem*>(m_categorizer.GetCategoryNode(categoryPath, parentRoot));
            }
            else
            {
                return static_cast<GraphCanvas::NodePaletteTreeItem*>(m_categorizer.GetCategoryNode(categoryPath, this));
            }
        }

        void ScriptCanvasRootPaletteTreeItem::PruneEmptyNodes()
        {
            m_categorizer.PruneEmptyNodes();
        }

        void ScriptCanvasRootPaletteTreeItem::OnRowsInserted(const QModelIndex& parentIndex, int first, int last)
        {
            for (int i = first; i <= last; ++i)
            {
                QModelIndex modelIndex = m_assetModel->index(i, 0, parentIndex);
                QModelIndex sourceIndex = m_assetModel->mapToSource(modelIndex);

                AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = reinterpret_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());
                ProcessAsset(entry);
            }
        }

        void ScriptCanvasRootPaletteTreeItem::OnRowsAboutToBeRemoved(const QModelIndex& parentIndex, int first, int last)
        {
            for (int i = first; i <= last; ++i)
            {
                QModelIndex modelIndex = m_assetModel->index(first, 0, parentIndex);
                QModelIndex sourceIndex = m_assetModel->mapToSource(modelIndex);

                const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = reinterpret_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

                if (entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
                {
                    const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productEntry = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(entry);

                    const AZ::Data::AssetId& assetId = productEntry->GetAssetId();

                    auto scriptEventElementIter = m_scriptEventElementTreeItems.find(assetId);
                    if (scriptEventElementIter != m_scriptEventElementTreeItems.end())
                    {
                        scriptEventElementIter->second->DetachItem();
                        delete scriptEventElementIter->second;
                        m_scriptEventElementTreeItems.erase(scriptEventElementIter);
                    }
                }
            }

            PruneEmptyNodes();
        }

        void ScriptCanvasRootPaletteTreeItem::TraverseTree(QModelIndex index)
        {
            QModelIndex sourceIndex = m_assetModel->mapToSource(index);
            AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = reinterpret_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

            ProcessAsset(entry);

            int rowCount = m_assetModel->rowCount(index);

            for (int i = 0; i < rowCount; ++i)
            {
                QModelIndex nextIndex = m_assetModel->index(i, 0, index);
                TraverseTree(nextIndex);
            }
        }

        void ScriptCanvasRootPaletteTreeItem::ProcessAsset(AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
        {
            if (entry)
            {
                if (entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
                {
                    const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productEntry = static_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(entry);

                    if (productEntry->GetAssetType() == azrtti_typeid<ScriptEvents::ScriptEventsAsset>())
                    {
                        const AZ::Data::AssetId& assetId = productEntry->GetAssetId();

                        auto elementIter = m_scriptEventElementTreeItems.find(assetId);

                        if (elementIter == m_scriptEventElementTreeItems.end())
                        {
                            const bool loadBlocking = true;
                            auto busAsset = AZ::Data::AssetManager::Instance().GetAsset(assetId, azrtti_typeid<ScriptEvents::ScriptEventsAsset>(), true, &AZ::ObjectStream::AssetFilterDefault, loadBlocking);
                            if (!busAsset)
                            {
                                AZ_Error("NodePalette", false, "Failed getting asset with Id: %s", assetId.ToString<AZStd::string>().c_str());
                                return;
                            }

                            if (busAsset.IsReady())
                            {
                                ScriptEvents::ScriptEventsAsset* data = busAsset.GetAs<ScriptEvents::ScriptEventsAsset>();

                                if (data)
                                {
                                    GraphCanvas::NodePaletteTreeItem* categoryRoot = GetCategoryNode(data->m_definition.GetCategory().c_str());
                                    ScriptEventsPaletteTreeItem* treeItem = categoryRoot->CreateChildNode<ScriptEventsPaletteTreeItem>(busAsset);

                                    if (treeItem)
                                    {
                                        m_scriptEventElementTreeItems[assetId] = treeItem;
                                    }
                                }
                            }
                            else
                            {
                                AZ_TracePrintf("NodePalette", "Could not refresh node palette properly, the asset is not ready.");
                            }
                        }
                    }
                }
            }
        }

        //////////////////////////
        // NodePaletteDockWidget
        //////////////////////////

        NodePaletteDockWidget::NodePaletteDockWidget(const NodePaletteModel& nodePaletteModel, const QString& windowLabel, QWidget* parent, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel, bool inContextMenu)
            : GraphCanvas::NodePaletteDockWidget(ExternalCreatePaletteRoot(nodePaletteModel, assetModel), ScriptCanvasEditor::AssetEditorId, windowLabel, parent, GetMimeType(), inContextMenu, inContextMenu ? "ScriptCanvas" : "ScriptCnavas_ContextMenu")
            , m_assetModel(assetModel)
            , m_nodePaletteModel(nodePaletteModel)
            , m_cyclingIdentifier(0)
            , m_nextCycleAction(nullptr)
            , m_previousCycleAction(nullptr)
            , m_ignoreSelectionChanged(false)
        {
            if (!inContextMenu)
            {
                m_newCustomEvent = new QToolButton(this);
                m_newCustomEvent->setIcon(QIcon(":/ScriptCanvasEditorResources/Resources/add.png"));
                m_newCustomEvent->setToolTip("Click to create a new Custom Event");

                QObject::connect(m_newCustomEvent, &QToolButton::clicked, this, &NodePaletteDockWidget::OnNewCustomEvent);

                AddSearchCustomizationWidget(m_newCustomEvent);

                QObject::connect(this, &GraphCanvas::NodePaletteDockWidget::OnTreeItemDoubleClicked, this, &NodePaletteDockWidget::HandleTreeItemDoubleClicked);

                QTreeView* treeView = GetTreeView();

                {
                    m_nextCycleAction = new QAction(treeView);

                    m_nextCycleAction->setShortcut(QKeySequence(Qt::Key_F8));
                    treeView->addAction(m_nextCycleAction);

                    QObject::connect(m_nextCycleAction, &QAction::triggered, this, &NodePaletteDockWidget::CycleToNextNode);
                }

                {
                    m_previousCycleAction = new QAction(treeView);

                    m_previousCycleAction->setShortcut(QKeySequence(Qt::Key_F7));
                    treeView->addAction(m_previousCycleAction);

                    QObject::connect(m_previousCycleAction, &QAction::triggered, this, &NodePaletteDockWidget::CycleToPreviousNode);
                }

                QObject::connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NodePaletteDockWidget::OnTreeSelectionChanged);
            }

            ConfigureSearchCustomizationMargins(QMargins(3, 0, 0, 0), 0);

            GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);
        }

        void NodePaletteDockWidget::OnNewCustomEvent()
        {
            AzToolsFramework::AssetEditor::AssetEditorRequestsBus::Broadcast(&AzToolsFramework::AssetEditor::AssetEditorRequests::CreateNewAsset, azrtti_typeid<ScriptEvents::ScriptEventsAsset>());
        }

        void ScriptCanvasRootPaletteTreeItem::AssetChanged(AzFramework::AssetSystem::AssetNotificationMessage /*message*/)
        {
            TraverseTree();
        }

        void ScriptCanvasRootPaletteTreeItem::AssetRemoved(AzFramework::AssetSystem::AssetNotificationMessage /*message*/)
        {
            TraverseTree();
        }

        void NodePaletteDockWidget::HandleTreeItemDoubleClicked(GraphCanvas::GraphCanvasTreeItem* treeItem)
        {
            SetCycleTarget(NodeIdentifierFactory::ConstructNodeIdentifier(treeItem));
            CycleToNextNode();
        }

        void NodePaletteDockWidget::OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId)
        {
            GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
            GraphCanvas::SceneNotificationBus::Handler::BusConnect(graphCanvasGraphId);
        }

        void NodePaletteDockWidget::OnSelectionChanged()
        {
            if (m_ignoreSelectionChanged)
            {
                return;
            }

            m_cyclingHelper.Clear();

            GetTreeView()->selectionModel()->clearSelection();
        }

        GraphCanvas::GraphCanvasTreeItem* NodePaletteDockWidget::CreatePaletteRoot() const
        {
            return ExternalCreatePaletteRoot(m_nodePaletteModel, m_assetModel);
        }

        void NodePaletteDockWidget::OnTreeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
        {
            AZStd::unordered_set< ScriptCanvas::VariableId > variableSet;

            GraphCanvas::NodePaletteWidget* paletteWidget = GetNodePaletteWidget();

            QModelIndexList indexList = GetTreeView()->selectionModel()->selectedRows();           

            if (indexList.size() == 1)
            {
                QSortFilterProxyModel* filterModel = static_cast<QSortFilterProxyModel*>(GetTreeView()->model());

                for (const QModelIndex& index : indexList)
                {
                    QModelIndex sourceIndex = filterModel->mapToSource(index);

                    GraphCanvas::NodePaletteTreeItem* nodePaletteItem = static_cast<GraphCanvas::NodePaletteTreeItem*>(sourceIndex.internalPointer());
                    SetCycleTarget(NodeIdentifierFactory::ConstructNodeIdentifier(nodePaletteItem));
                }
            }
            else
            {
                SetCycleTarget(ScriptCanvas::NodeTypeIdentifier());
            }
        }

        void NodePaletteDockWidget::SetCycleTarget(ScriptCanvas::NodeTypeIdentifier cyclingIdentifier)
        {
            if (m_cyclingIdentifier != cyclingIdentifier)
            {
                m_cyclingIdentifier = cyclingIdentifier;
                m_cyclingHelper.Clear();

                if (m_nextCycleAction)
                {
                    bool isValid = m_cyclingIdentifier != ScriptCanvas::NodeTypeIdentifier(0);

                    m_nextCycleAction->setEnabled(isValid);
                    m_previousCycleAction->setEnabled(isValid);
                }
            }
        }

        void NodePaletteDockWidget::CycleToNextNode()
        {
            ConfigureHelper();

            m_cyclingHelper.CycleToNextNode();
        }

        void NodePaletteDockWidget::CycleToPreviousNode()
        {
            ConfigureHelper();

            m_cyclingHelper.CycleToPreviousNode();
        }

        void NodePaletteDockWidget::ConfigureHelper()
        {
            if (!m_cyclingHelper.IsConfigured() && m_cyclingIdentifier != ScriptCanvas::NodeTypeIdentifier(0))
            {
                AZ::EntityId scriptCanvasGraphId;
                GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetActiveScriptCanvasGraphId);

                AZ::EntityId graphCanvasGraphId;
                GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetActiveGraphCanvasGraphId);

                m_cyclingHelper.SetActiveGraph(graphCanvasGraphId);

                AZStd::vector<NodeIdPair> nodePairs;
                EditorGraphRequestBus::EventResult(nodePairs, scriptCanvasGraphId, &EditorGraphRequests::GetNodesOfType, m_cyclingIdentifier);                

                AZStd::vector<GraphCanvas::NodeId> cyclingNodes;
                cyclingNodes.reserve(nodePairs.size());

                for (const auto& nodeIdPair : nodePairs)
                {
                    cyclingNodes.emplace_back(nodeIdPair.m_graphCanvasId);
                }

                m_cyclingHelper.SetNodes(cyclingNodes);

                {
                    // Clean-up Selection to maintain the 'single' selection state throughout the editor
                    QScopedValueRollback<bool> ignoreSelection(m_ignoreSelectionChanged, true);
                    GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);
                }

                EditorGraphRequestBus::Event(scriptCanvasGraphId, &EditorGraphRequests::HighlightNodes, nodePairs);
            }
        }
    }
}

#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.moc>