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

#include <AzCore/std/containers/vector.h>
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include "../StandardPluginsConfig.h"
#include <QWidget>


QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)
QT_FORWARD_DECLARE_CLASS(QContextMenuEvent)

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;
    class NodeGraph;


    class NavigateWidget
        : public QWidget
        , protected EMotionFX::AnimGraphNotificationBus::Handler
    {
        MCORE_MEMORYOBJECTCATEGORY(NavigateWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

        friend class AnimGraphPlugin;

    public:
        enum
        {
            COLUMN_NAME             = 0,
            COLUMN_TYPE             = 1
        };

        NavigateWidget(AnimGraphPlugin* plugin, QWidget* parent = nullptr);
        ~NavigateWidget();

        // AnimGraphNotificationBus
        void OnSyncVisualObject(EMotionFX::AnimGraphObject* object) override;

        void RemoveTreeRow(const QString& name);
        void Rename(const char* oldName, const char* newName);

        MCORE_INLINE QTreeWidget* GetTreeWidget()   { return mTreeWidget; }
        MCORE_INLINE QTreeWidgetItem* GetRootItem() { return mRootItem; }

        QTreeWidgetItem* FindItem(const QString& name);
        QTreeWidgetItem* FindItemById(const EMotionFX::AnimGraphNodeId& id) const;

        void ShowGraph(EMotionFX::AnimGraphNode* node, bool updateInterface);
        void ShowGraphByNodeName(const AZStd::string& nodeName, bool updateInterface);

        static QTreeWidgetItem* InsertNode(QTreeWidget* treeWidget, QTreeWidgetItem* parentItem, EMotionFX::AnimGraphNode* node, bool recursive, const AZ::TypeId& visibilityFilterNodeType = AZ::TypeId::CreateNull(), bool showStatesOnly = false, const char* searchFilterString = nullptr);

        // returns the root item
        static QTreeWidgetItem* UpdateTreeWidget(QTreeWidget* treeWidget, EMotionFX::AnimGraph* animGraph, const AZ::TypeId& visibilityFilterNodeType = AZ::TypeId::CreateNull(), bool showStatesOnly = false, const char* searchFilterString = nullptr);

        void UpdateTreeWidget(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* nodeToShow = nullptr);

        EMotionFX::AnimGraphNode* GetSingleSelectedNode();
        MCore::Array<EMotionFX::AnimGraphNode*> GetSelectedNodes(EMotionFX::AnimGraph* animGraph);
        bool GetIsReadyForPaste();

        void SetVisualOptionsNode(EMotionFX::AnimGraphNode* node)      { mVisualOptionsNode = node; }


    public slots:
        void NavigateToNode(QTreeWidgetItem* item, int column);
        void OnSelectionChangedWithoutGraphUpdate();
    public slots:
        void OnSelectionChanged();
        void OnItemClicked(QTreeWidgetItem* item, int column);
        void OnContextMenuRename();
        void OnDeleteSelectedNodes();
        void OnSetAsEntryState();
        void OnAddWildCardTransition();
        void OnVisualizeOptions();
        void OnVisualizeColorChanged(const QColor& color);
        void NavigateToNode();
        void MakeVirtualFinalNode();
        void RestoreVirtualFinalNode();
        void EnableSelected()                               { SetEnabledSelectedNodes(true); }
        void DisableSelected()                              { SetEnabledSelectedNodes(false); }
        void Cut();
        void Copy();
        void Paste();
        void OnTextFilterChanged(const QString& text);

    private:
        QTreeWidget*        mTreeWidget;
        AnimGraphPlugin*    mPlugin;
        QTreeWidgetItem*    mItemClicked;
        QString             mItemClickedName;
        QTreeWidgetItem*    mVisibleItem;
        QTreeWidgetItem*    mRootItem;
        EMotionFX::AnimGraphNode*      mVisualColorNode;
        EMotionFX::AnimGraphNode*      mVisualOptionsNode;
        AzQtComponents::FilteredSearchWidget* m_searchWidget;
        AZStd::string       m_searchStringText;

        void SetEnabledSelectedNodes(bool flag);
        void FillPasteCommandGroup(bool cutMode);
        AZStd::unordered_set<EMotionFX::AnimGraphNodeId> mNodeIdsToCopy;
        AZ::TypeId                                  mCopyParentNodeType;
        bool                                        mCutMode;

        void ChangeVisibleItem(EMotionFX::AnimGraphNode* node);

        void contextMenuEvent(QContextMenuEvent* event);
        void keyReleaseEvent(QKeyEvent* event);
        void dropEvent(QDropEvent* event);
        void dragEnterEvent(QDragEnterEvent* event);
        void dragLeaveEvent(QDragLeaveEvent* event);
        void dragMoveEvent(QDragMoveEvent* event);
    };
} // namespace EMStudio